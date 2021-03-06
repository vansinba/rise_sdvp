/*
	Copyright 2016 Benjamin Vedder	benjamin@vedder.se

	This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "autopilot.h"
#include <math.h>
#include <string.h>
#include "ch.h"
#include "hal.h"
#include "servo_simple.h"
#include "utils.h"
#include "pos.h"
#include "bldc_interface.h"

// Defines
#define AP_HZ						100 // Hz

// Private variables
static THD_WORKING_AREA(ap_thread_wa, 512);
static ROUTE_POINT m_route[AP_ROUTE_SIZE];
static bool m_is_active;
static int m_point_last; // The last point on the route
static int m_point_now; // The first point in the currently considered part of the route
static bool m_has_prev_point;
static float m_override_speed;
static bool m_is_speed_override;
static ROUTE_POINT m_rp_now; // The point in space we are following now
static float m_rad_now;
static ROUTE_POINT m_point_rx_prev;
static bool m_point_rx_prev_set;
static mutex_t m_ap_lock;

// Private functions
static THD_FUNCTION(ap_thread, arg);
static void steering_angle_to_point(
		float current_x,
		float current_y,
		float current_angle,
		float goal_x,
		float goal_y,
		float *steering_angle,
		float *distance);
static bool add_point(ROUTE_POINT *p, bool first);
static void clear_route(void);

void autopilot_init(void) {
	memset(m_route, 0, sizeof(m_route));
	m_is_active = false;
	m_point_now = 0;
	m_point_last = 0;
	m_has_prev_point = false;
	m_override_speed = 0.0;
	m_is_speed_override = false;
	memset(&m_rp_now, 0, sizeof(ROUTE_POINT));
	m_rad_now = -1.0;
	memset(&m_point_rx_prev, 0, sizeof(ROUTE_POINT));
	m_point_rx_prev_set = false;
	chMtxObjectInit(&m_ap_lock);

	chThdCreateStatic(ap_thread_wa, sizeof(ap_thread_wa),
			NORMALPRIO, ap_thread, NULL);
}

/**
 * Add a point to the route
 *
 * @param p
 * The point to add
 *
 * @param first
 * True if this is the first point in the packet. Used to check for duplicate packets.
 *
 * @return
 * True if the point was added, false otherwise.
 */
bool autopilot_add_point(ROUTE_POINT *p, bool first) {
	chMtxLock(&m_ap_lock);

	bool res = add_point(p, first);

	chMtxUnlock(&m_ap_lock);

	return res;
}

void autopilot_remove_last_point(void) {
	chMtxLock(&m_ap_lock);

	if (m_point_last != m_point_now) {
		m_point_last--;
		if (m_point_last < 0) {
			m_point_last = AP_ROUTE_SIZE - 1;
		}
	}

	chMtxUnlock(&m_ap_lock);
}

void autopilot_clear_route(void) {
	chMtxLock(&m_ap_lock);

	clear_route();

	chMtxUnlock(&m_ap_lock);
}

void autopilot_replace_route(ROUTE_POINT *p) {
	chMtxLock(&m_ap_lock);

	if (!m_is_active) {
		clear_route();
		add_point(p, true);
	} else {
		while (m_point_last != m_point_now) {
			m_point_last--;
			if (m_point_last < 0) {
				m_point_last = AP_ROUTE_SIZE - 1;
			}
		}

		m_has_prev_point = false;
		add_point(p, true);
	}

	chMtxUnlock(&m_ap_lock);
}

void autopilot_set_active(bool active) {
	chMtxLock(&m_ap_lock);

	m_is_active = active;

	chMtxUnlock(&m_ap_lock);
}

bool autopilot_is_active(void) {
	return m_is_active;
}

/**
 * Override the speed with a fixed speed instead of using the value defined by
 * the route.
 *
 * @param is_override
 * True for override, false for using the route speed.
 *
 * @param speed
 * The speed to use. Ignored if is_override is false.
 */
void autopilot_set_speed_override(bool is_override, float speed) {
	m_is_speed_override = is_override;
	m_override_speed = speed;
}

/**
 * Set the motor speed.
 *
 * @param speed
 * Speed in m/s.
 */
void autopilot_set_motor_speed(float speed) {
	float rpm = speed / (main_config.car.gear_ratio
			* (2.0 / main_config.car.motor_poles) * (1.0 / 60.0)
			* main_config.car.wheel_diam * M_PI);
	if (!main_config.car.disable_motor) {
		bldc_interface_set_rpm((int)rpm);
	}
}

/**
 * Get steering scale factor based in the current speed.
 *
 * @return
 * Steering scale factor. 1.0 at low speed, decreasing at high speed.
 */
float autopilot_get_steering_scale(void) {
	const float div = 1.0 + fabsf(pos_get_speed()) * 0.05;
	return 1.0 / (div * div);
}

/**
 * Get the current radius for calculating the next goal point.
 *
 * @return
 * The radius in meters.
 */
float autopilot_get_rad_now(void) {
	return m_rad_now;
}

/**
 * Get the current goal point for the autopilot.
 *
 * @rp
 * Pointer to store the goal to.
 */
void autopilot_get_goal_now(ROUTE_POINT *rp) {
	*rp = m_rp_now;
}

static THD_FUNCTION(ap_thread, arg) {
	(void)arg;

	chRegSetThreadName("Autopilot");

	for(;;) {
		chThdSleep(CH_CFG_ST_FREQUENCY / AP_HZ);

		bool route_end = false;

		chMtxLock(&m_ap_lock);

		if (!m_is_active) {
			m_rad_now = -1.0;
			chMtxUnlock(&m_ap_lock);
			continue;
		}

		// the length of the route that is left
		int len = m_point_last;

		// This means that the route has wrapped around
		// (should only happen when ap_repeat_routes == false)
		if (m_point_now > m_point_last) {
			len = AP_ROUTE_SIZE + m_point_last - m_point_now;
		}

		// Time of today according to our clock
		int ms_today = pos_get_ms_today();

		if (len >= 2) {
			POS_STATE p;
			pos_get_pos(&p);

			// Car center
			const float car_cx = p.px;
			const float car_cy = p.py;
			ROUTE_POINT car_pos;
			car_pos.px = car_cx;
			car_pos.py = car_cy;

			// Look 5 points ahead, or less than that if the route is shorter
			int add = 5;
			if (add > len) {
				add = len;
			}

			int start = m_point_now;
			int end = m_point_now + add;

			// Speed-dependent radius
			m_rad_now = main_config.ap_base_rad / autopilot_get_steering_scale();

			ROUTE_POINT rp_now; // The point we should follow now.
			int circle_intersections = 0;
			bool last_point_reached = false;

			// Last point in route
			int last_point_ind = m_point_last - 1;
			if (last_point_ind < 0) {
				last_point_ind += AP_ROUTE_SIZE;
			}

			ROUTE_POINT *rp_last = &m_route[last_point_ind]; // Last point on route
			ROUTE_POINT *rp_ls1 = &m_route[0]; // First point on goal line segment
			ROUTE_POINT *rp_ls2 = &m_route[1]; // Second point on goal line segment

			for (int i = start;i < end;i++) {
				int ind = i; // First point index for this iteration
				int indn = i + 1; // Next point index for this iteration

				// Wrap around
				if (ind >= m_point_last) {
					if (m_point_now <= m_point_last) {
						ind -= m_point_last;
					} else {
						if (ind >= AP_ROUTE_SIZE) {
							ind -= AP_ROUTE_SIZE;
						}
					}
				}

				// Wrap around
				if (indn >= m_point_last) {
					if (m_point_now <= m_point_last) {
						indn -= m_point_last;
					} else {
						if (indn >= AP_ROUTE_SIZE) {
							indn -= AP_ROUTE_SIZE;
						}
					}
				}

				// Check for circle intersection. If there are many intersections
				// found in this loop, the last one will be used.
				ROUTE_POINT int1, int2;
				ROUTE_POINT *p1, *p2;
				p1 = &m_route[ind];
				p2 = &m_route[indn];

				// If the next point has a time before the current point and repeat route is
				// active we have completed a full route. Increase its time by the repetition time.
				if (main_config.ap_repeat_routes && utils_time_before(p2->time, p1->time)) {
					p2->time += main_config.ap_time_add_repeat_ms;
					if (p2->time > MS_PER_DAY) {
						p2->time -= MS_PER_DAY;
					}
				}

				int res = utils_circle_line_int(car_cx, car_cy, m_rad_now, p1, p2, &int1, &int2);

				// One intersection. Use it.
				if (res == 1) {
					circle_intersections++;
					rp_now = int1;
				}

				// Two intersections. Use the point with the most "progress" on the route.
				if (res == 2) {
					circle_intersections += 2;

					if (utils_rp_distance(&int1, p2) < utils_rp_distance(&int2, p2)) {
						rp_now = int1;
					} else {
						rp_now = int2;
					}
				}

				if (res > 0) {
					rp_ls1 = &m_route[ind];
					rp_ls2 = &m_route[indn];
				}

				// If we aren't repeating routes and there is an intersecion on the last
				// line segment, go straight to the last point.
				if (!main_config.ap_repeat_routes) {
					if (indn == last_point_ind && circle_intersections > 0) {
						if (res > 0) {
							last_point_reached = true;
						}
						break;
					}
				}
			}

			// Look for closest points
			ROUTE_POINT closest; // Closest point on route to car
			ROUTE_POINT *closest1 = &m_route[0]; // Start of closest line segment
			ROUTE_POINT *closest2 = &m_route[1]; // End of closest line segment
			int closest1_ind = 0; // Index of the first closest point

			{
				bool closest_set = false;

				for (int i = start;i < end;i++) {
					int ind = i; // First point index for this iteration
					int indn = i + 1; // Next point index for this iteration

					// Wrap around
					if (ind >= m_point_last) {
						if (m_point_now <= m_point_last) {
							ind -= m_point_last;
						} else {
							if (ind >= AP_ROUTE_SIZE) {
								ind -= AP_ROUTE_SIZE;
							}
						}
					}

					// Wrap around
					if (indn >= m_point_last) {
						if (m_point_now <= m_point_last) {
							indn -= m_point_last;
						} else {
							if (indn >= AP_ROUTE_SIZE) {
								indn -= AP_ROUTE_SIZE;
							}
						}
					}

					ROUTE_POINT tmp;
					ROUTE_POINT *p1, *p2;
					p1 = &m_route[ind];
					p2 = &m_route[indn];
					utils_closest_point_line(p1, p2, car_cx, car_cy, &tmp);

					if (!closest_set || utils_rp_distance(&tmp, &car_pos) < utils_rp_distance(&closest, &car_pos)) {
						closest_set = true;
						closest = tmp;
						closest1 = p1;
						closest2 = p2;
						closest1_ind = ind;
					}

					// Do not look past the last point if we aren't repeating routes.
					if (!main_config.ap_repeat_routes) {
						if (indn == last_point_ind) {
							break;
						}
					}
				}
			}

			if (last_point_reached) {
				rp_now = *rp_last;
			} else {
				// Use the closest point on the considered route if no
				// circle intersection is found.
				if (circle_intersections == 0) {
					rp_now = closest;
					rp_ls1 = closest1;
					rp_ls2 = closest2;
				}
			}

			// Check if the end of route is reached
			if (!main_config.ap_repeat_routes &&
					utils_rp_distance(&m_route[last_point_ind], &car_pos) < m_rad_now) {
				route_end = true;
			}

			m_point_now = closest1_ind;
			m_rp_now = rp_now;

			if (!route_end) {
				float distance, steering_angle;
				float servo_pos;
				static int max_steering = 0;

				steering_angle_to_point(p.px, p.py, -p.yaw * M_PI / 180.0, rp_now.px,
						rp_now.py, &steering_angle, &distance);

				// Scale maximum steering by speed
				float max_rad = main_config.car.steering_max_angle_rad * autopilot_get_steering_scale();

				if (steering_angle >= max_rad) {
					steering_angle = max_rad;
					max_steering++;
				} else if (steering_angle <= -max_rad) {
					steering_angle = -max_rad;
					max_steering++;
				} else {
					max_steering = 0;
				}

				servo_pos = steering_angle
						/ ((2.0 * main_config.car.steering_max_angle_rad)
								/ main_config.car.steering_range)
								+ main_config.car.steering_center;

				float speed = 0.0;

				if (main_config.ap_mode_time) {
					if (ms_today >= 0) {
						// Calculate speed such that the route points are reached at their
						// specified time. Notice that the direct distance between the car
						// and the points is used and not the arc that the car drives. This
						// should still work well enough.

						int32_t dist_prev = (int32_t)(utils_rp_distance(&rp_now, rp_ls1) * 1000.0);
						int32_t dist_tot = (int32_t)(utils_rp_distance(&rp_now, rp_ls1) * 1000.0);
						dist_tot += (int32_t)(utils_rp_distance(&rp_now, rp_ls2) * 1000.0);
						int32_t time = utils_map_int(dist_prev, 0, dist_tot, rp_ls1->time, rp_ls2->time);
						float dist_car = utils_rp_distance(&car_pos, &rp_now);

						int32_t t_diff = time - ms_today;
						if (t_diff < 0) {
							t_diff += 24 * 60 * 60 * 1000;
						}

						if (t_diff > 0) {
							speed = dist_car / ((float)t_diff / 1000.0);
						} else {
							speed = 0.0;
						}
					} else {
						speed = 0.0;
					}
				} else {
					// Calculate the speed based on the average speed between the two closest points
					const float dist_prev = utils_rp_distance(&rp_now, closest1);
					const float dist_tot = utils_rp_distance(&rp_now, closest1) + utils_rp_distance(&rp_now, closest2);
					speed = utils_map(dist_prev, 0.0, dist_tot, closest1->speed, closest2->speed);
				}

				if (m_is_speed_override) {
					speed = m_override_speed;
				}

				utils_truncate_number_abs(&speed, main_config.ap_max_speed);

				servo_simple_set_pos_ramp(servo_pos);
				autopilot_set_motor_speed(speed);
			}
		} else {
			route_end = true;
		}

		if (route_end) {
			servo_simple_set_pos_ramp(main_config.car.steering_center);
			if (!main_config.car.disable_motor) {
				bldc_interface_set_current_brake(10.0);
			}
			m_rad_now = -1.0;
		}

		chMtxUnlock(&m_ap_lock);
	}
}

static void steering_angle_to_point(
		float current_x,
		float current_y,
		float current_angle,
		float goal_x,
		float goal_y,
		float *steering_angle,
		float *distance) {

	const float D = utils_point_distance(goal_x, goal_y, current_x, current_y);
	*distance = D;
	const float gamma = current_angle - atan2f((goal_y-current_y), (goal_x-current_x));
	const float dx = D * cosf(gamma);
	const float dy = D * sinf(gamma);

	if (dy == 0.0) {
		*steering_angle = 0.0;
		return;
	}

	float circle_radius = -(dx * dx + dy * dy) / (2.0 * dy);

	/*
	 * Add correction if the arc is much longer than the total distance.
	 * TODO: Find a good model.
	 */
	float angle_correction = 1.0 + D * 0.2;
	if (angle_correction > 5.0) {
		angle_correction = 5.0;
	}

	*steering_angle = atanf(main_config.car.axis_distance / circle_radius) * angle_correction;
}

static bool add_point(ROUTE_POINT *p, bool first) {
	if (first && m_point_rx_prev_set &&
			utils_point_distance(m_point_rx_prev.px, m_point_rx_prev.py, p->px, p->py) < 1e-4) {
		return false;
	}

	if (first) {
		m_point_rx_prev = *p;
		m_point_rx_prev_set = true;
	}

	m_route[m_point_last++] = *p;

	if (m_point_last >= AP_ROUTE_SIZE) {
		m_point_last = 0;
	}

	// Make sure that there always is a valid point when looking backwards in the route
	if (!m_has_prev_point) {
		int p_last = m_point_now - 1;
		if (p_last < 0) {
			p_last += AP_ROUTE_SIZE;
		}

		m_route[p_last] = *p;
		m_has_prev_point = true;
	}

	// When repeating routes, the previous point for the first
	// point is the end point of the current route.
	if (main_config.ap_repeat_routes) {
		m_route[AP_ROUTE_SIZE - 1] = *p;
	}

	return true;
}

static void clear_route(void) {
	m_is_active = false;
	m_has_prev_point = false;
	m_point_now = 0;
	m_point_last = 0;
	m_point_rx_prev_set = false;
}
