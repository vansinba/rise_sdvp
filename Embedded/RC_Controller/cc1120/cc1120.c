#include "cc1120.h"
#include "commands.h"
#include "utils.h"

// Settings
#define CC1120_SPI								SPID1
#define CC1120_PORT_CS							GPIOA
#define CC1120_PIN_CS							4
#define CC1120_PORT_SCK							GPIOA
#define CC1120_PIN_SCK							5
#define CC1120_PORT_MISO						GPIOA
#define CC1120_PIN_MISO							6
#define CC1120_PORT_MOSI						GPIOA
#define CC1120_PIN_MOSI							7
#define CC1120_PORT_RESET						GPIOA
#define CC1120_PIN_RESET						0
#define CC1120_PORT_GPIO0						GPIOA
#define CC1120_PIN_GPIO0						3
#define CC1120_PORT_GPIO2						GPIOA
#define CC1120_PIN_GPIO2						2
#define CC1120_PORT_GPIO3						GPIOA
#define CC1120_PIN_GPIO3						1

#define CC1120_MAX_PAYLOAD						125

// Macros
#define CC1120_READ_BIT							0x80
#define CC1120_WRITE_BIT						0x00
#define CC1120_BURST_BIT						0x40
#define CC1120_IS_EXTENDED(x)					(x & 0x2F00)
#define CC1120_STATUS_STATE_MASK				0x70
#define CC1120_STATUS_STATE_TXFIFO_UNDERFLOW	0x70

// SPI Settings
static const SPIConfig spicfg = {
		NULL,
		CC1120_PORT_CS,
		CC1120_PIN_CS,
		SPI_CR1_BR_1 // 5.25 MHz
};

// Threads
static THD_WORKING_AREA(isr_thread_wa, 2048);
static THD_FUNCTION(isr_thread, arg);
static thread_t *isr_tp;

// Private functions
static int rx_interrupt(void);
static void spi_enable(void);
static void spi_disable(void);
static uint8_t spi_exchange(uint8_t x);

void cc1120_init(void) {
	palSetPadMode(CC1120_PORT_CS, CC1120_PIN_CS, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
	palSetPadMode(CC1120_PORT_SCK, CC1120_PIN_SCK, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
	palSetPadMode(CC1120_PORT_MISO, CC1120_PIN_MISO, PAL_MODE_ALTERNATE(5));
	palSetPadMode(CC1120_PORT_MOSI, CC1120_PIN_MOSI, PAL_MODE_ALTERNATE(5) | PAL_STM32_OSPEED_HIGHEST);
	palSetPadMode(CC1120_PORT_RESET, CC1120_PIN_RESET, PAL_MODE_OUTPUT_PUSHPULL | PAL_STM32_OSPEED_HIGHEST);
	palSetPadMode(CC1120_PORT_GPIO0, CC1120_PIN_GPIO0, PAL_MODE_INPUT);
	palSetPadMode(CC1120_PORT_GPIO2, CC1120_PIN_GPIO2, PAL_MODE_INPUT);
	palSetPadMode(CC1120_PORT_GPIO3, CC1120_PIN_GPIO3, PAL_MODE_INPUT);

	palSetPad(CC1120_PORT_CS, CC1120_PIN_CS);

	palClearPad(CC1120_PORT_RESET, CC1120_PIN_RESET);
	chThdSleepMilliseconds(100);
	palSetPad(CC1120_PORT_RESET, CC1120_PIN_RESET);
	chThdSleepMilliseconds(100);

	spiStart(&CC1120_SPI, &spicfg);

	// Address Config = No address check
	// Bit Rate = 9.6
	// Carrier Frequency = 452.000000
	// Deviation = 2.395630
	// Device Address = 0
	// Manchester Enable = false
	// Modulation Format = 2-GFSK
	// PA Ramping = true
	// Packet Bit Length = 0
	// Packet Length = 255
	// Packet Length Mode = Variable
	// Performance Mode = High Performance
	// RX Filter BW = 25.000000
	// Symbol rate = 9.6
	// TX Power = 15
	// Whitening = false

	//
	// From TI SmartRF studio 7
	//
//	cc1120_single_write(CC1120_SYNC_CFG1, 0x08);      // Sync Word Detection Configuration Reg. 1
//	cc1120_single_write(CC1120_DEVIATION_M, 0x3A);    // Frequency Deviation Configuration
//	cc1120_single_write(CC1120_MODCFG_DEV_E, 0x0A);   // Modulation Format and Frequency Deviation Configur..
//	cc1120_single_write(CC1120_DCFILT_CFG, 0x04);     // Digital DC Removal Configuration
//	cc1120_single_write(CC1120_PREAMBLE_CFG1, 0x18);  // Preamble Length Configuration Reg. 1
//	cc1120_single_write(CC1120_FREQ_IF_CFG, 0x00);    // RX Mixer Frequency Configuration
//	cc1120_single_write(CC1120_IQIC, 0x00);           // Digital Image Channel Compensation Configuration
//	cc1120_single_write(CC1120_CHAN_BW, 0x08);        // Channel Filter Configuration
//	cc1120_single_write(CC1120_MDMCFG0, 0x05);        // General Modem Parameter Configuration Reg. 0
//	cc1120_single_write(CC1120_SYMBOL_RATE2, 0x73);   // Symbol Rate Configuration Exponent and Mantissa [1..
//	cc1120_single_write(CC1120_AGC_REF, 0x3C);        // AGC Reference Level Configuration
//	cc1120_single_write(CC1120_AGC_CS_THR, 0xEC);     // Carrier Sense Threshold Configuration
//	cc1120_single_write(CC1120_AGC_CFG3, 0x83);       // Automatic Gain Control Configuration Reg. 3
//	cc1120_single_write(CC1120_AGC_CFG2, 0x60);       // Automatic Gain Control Configuration Reg. 2
//	cc1120_single_write(CC1120_AGC_CFG1, 0xA9);       // Automatic Gain Control Configuration Reg. 1
//	cc1120_single_write(CC1120_AGC_CFG0, 0xC0);       // Automatic Gain Control Configuration Reg. 0
//	cc1120_single_write(CC1120_FIFO_CFG, 0x00);       // FIFO Configuration
//	cc1120_single_write(CC1120_FS_CFG, 0x14);         // Frequency Synthesizer Configuration
//	cc1120_single_write(CC1120_PA_CFG0, 0x05);        // Power Amplifier Configuration Reg. 0
//	cc1120_single_write(CC1120_PKT_LEN, 0xFF);        // Packet Length Configuration
//	cc1120_single_write(CC1120_IF_MIX_CFG, 0x00);     // IF Mix Configuration
//	cc1120_single_write(CC1120_TOC_CFG, 0x0A);        // Timing Offset Correction Configuration
//	cc1120_single_write(CC1120_FREQ2, 0x71);          // Frequency Configuration [23:16]
//	cc1120_single_write(CC1120_FS_DIG1, 0x00);        // Frequency Synthesizer Digital Reg. 1
//	cc1120_single_write(CC1120_FS_DIG0, 0x5F);        // Frequency Synthesizer Digital Reg. 0
//	cc1120_single_write(CC1120_FS_CAL1, 0x40);        // Frequency Synthesizer Calibration Reg. 1
//	cc1120_single_write(CC1120_FS_CAL0, 0x0E);        // Frequency Synthesizer Calibration Reg. 0
//	cc1120_single_write(CC1120_FS_DIVTWO, 0x03);      // Frequency Synthesizer Divide by 2
//	cc1120_single_write(CC1120_FS_DSM0, 0x33);        // FS Digital Synthesizer Module Configuration Reg. 0
//	cc1120_single_write(CC1120_FS_DVC0, 0x17);        // Frequency Synthesizer Divider Chain Configuration ..
//	cc1120_single_write(CC1120_FS_PFD, 0x50);         // Frequency Synthesizer Phase Frequency Detector Con..
//	cc1120_single_write(CC1120_FS_PRE, 0x6E);         // Frequency Synthesizer Prescaler Configuration
//	cc1120_single_write(CC1120_FS_REG_DIV_CML, 0x14); // Frequency Synthesizer Divider Regulator Configurat..
//	cc1120_single_write(CC1120_FS_SPARE, 0xAC);       // Frequency Synthesizer Spare
//	cc1120_single_write(CC1120_FS_VCO0, 0xB4);        // FS Voltage Controlled Oscillator Configuration Reg..
//	cc1120_single_write(CC1120_XOSC5, 0x0E);          // Crystal Oscillator Configuration Reg. 5
//	cc1120_single_write(CC1120_XOSC1, 0x03);          // Crystal Oscillator Configuration Reg. 1

	// Address Config = No address check
	// Bit Rate = 1.2
	// Carrier Frequency = 434.000000
	// Deviation = 20.019531
	// Device Address = 0
	// Manchester Enable = false
	// Modulation Format = 2-FSK
	// PA Ramping = true
	// Packet Bit Length = 0
	// Packet Length = 255
	// Packet Length Mode = Variable
	// Performance Mode = High Performance
	// RX Filter BW = 50.000000
	// Symbol rate = 1.2
	// TX Power = 15
	// Whitening = false

	//
	// From TI SmartRF studio 7
	//
	cc1120_single_write(CC1120_SYNC_CFG1, 0x0B);      // Sync Word Detection Configuration Reg. 1
	cc1120_single_write(CC1120_DEVIATION_M, 0x48);    // Frequency Deviation Configuration
	cc1120_single_write(CC1120_MODCFG_DEV_E, 0x05);   // Modulation Format and Frequency Deviation Configur..
	cc1120_single_write(CC1120_DCFILT_CFG, 0x1C);     // Digital DC Removal Configuration
	cc1120_single_write(CC1120_IQIC, 0x00);           // Digital Image Channel Compensation Configuration
	cc1120_single_write(CC1120_CHAN_BW, 0x04);        // Channel Filter Configuration
	cc1120_single_write(CC1120_MDMCFG0, 0x05);        // General Modem Parameter Configuration Reg. 0
	cc1120_single_write(CC1120_AGC_CFG1, 0xA9);       // Automatic Gain Control Configuration Reg. 1
	cc1120_single_write(CC1120_AGC_CFG0, 0xCF);       // Automatic Gain Control Configuration Reg. 0
	cc1120_single_write(CC1120_FIFO_CFG, 0x03);       // FIFO Configuration
	cc1120_single_write(CC1120_FS_CFG, 0x14);         // Frequency Synthesizer Configuration
	cc1120_single_write(CC1120_PKT_LEN, 0xFF);        // Packet Length Configuration
	cc1120_single_write(CC1120_IF_MIX_CFG, 0x00);     // IF Mix Configuration
	cc1120_single_write(CC1120_FREQOFF_CFG, 0x22);    // Frequency Offset Correction Configuration
	cc1120_single_write(CC1120_FREQ2, 0x6C);          // Frequency Configuration [23:16]
	cc1120_single_write(CC1120_FREQ1, 0x80);          // Frequency Configuration [15:8]
	cc1120_single_write(CC1120_FS_DIG1, 0x00);        // Frequency Synthesizer Digital Reg. 1
	cc1120_single_write(CC1120_FS_DIG0, 0x5F);        // Frequency Synthesizer Digital Reg. 0
	cc1120_single_write(CC1120_FS_CAL1, 0x40);        // Frequency Synthesizer Calibration Reg. 1
	cc1120_single_write(CC1120_FS_CAL0, 0x0E);        // Frequency Synthesizer Calibration Reg. 0
	cc1120_single_write(CC1120_FS_DIVTWO, 0x03);      // Frequency Synthesizer Divide by 2
	cc1120_single_write(CC1120_FS_DSM0, 0x33);        // FS Digital Synthesizer Module Configuration Reg. 0
	cc1120_single_write(CC1120_FS_DVC0, 0x17);        // Frequency Synthesizer Divider Chain Configuration ..
	cc1120_single_write(CC1120_FS_PFD, 0x50);         // Frequency Synthesizer Phase Frequency Detector Con..
	cc1120_single_write(CC1120_FS_PRE, 0x6E);         // Frequency Synthesizer Prescaler Configuration
	cc1120_single_write(CC1120_FS_REG_DIV_CML, 0x14); // Frequency Synthesizer Divider Regulator Configurat..
	cc1120_single_write(CC1120_FS_SPARE, 0xAC);       // Frequency Synthesizer Spare
	cc1120_single_write(CC1120_FS_VCO0, 0xB4);        // FS Voltage Controlled Oscillator Configuration Reg..
	cc1120_single_write(CC1120_XOSC5, 0x0E);          // Crystal Oscillator Configuration Reg. 5
	cc1120_single_write(CC1120_XOSC1, 0x03);          // Crystal Oscillator Configuration Reg. 1

	cc1120_single_write(CC1120_IOCFG3, CC1120_SETTING_IOCFG3);
	cc1120_single_write(CC1120_IOCFG2, CC1120_SETTING_IOCFG2);
	cc1120_single_write(CC1120_IOCFG1, CC1120_SETTING_IOCFG1);
	cc1120_single_write(CC1120_IOCFG0, CC1120_SETTING_IOCFG0);
	cc1120_single_write(CC1120_PKT_CFG0, CC1120_SETTING_PKT_CFG0);
	cc1120_single_write(CC1120_RFEND_CFG0, CC1120_SETTING_RFEND_CFG0);
	cc1120_single_write(CC1120_RFEND_CFG1, CC1120_SETTING_RFEND_CFG1);

//	cc1120_single_write(CC1120_SYNC_CFG0, 0x03);

	chThdCreateStatic(isr_thread_wa, sizeof(isr_thread_wa), NORMALPRIO + 2, isr_thread, NULL);
	extChannelEnable(&EXTD1, 3);

	cc1120_off();
	cc1120_strobe(CC1120_SIDLE);
	cc1120_calibrate_manual();
	cc1120_on();
}

uint8_t cc1120_state(void) {
	uint8_t state;
	cc1120_burst_read(CC1120_MARCSTATE, &state, 1);
	return state & 0x1f;
}

char *cc1120_state_name(void) {
	uint8_t state;
	cc1120_burst_read(CC1120_MARCSTATE, &state, 1);
	cc1120_burst_read(CC1120_MARCSTATE, &state, 1);

	state &= 0x1F;

	switch (state) {
		case 0: return "SLEEP"; break;
		case 1: return "IDLE"; break;
		case 2: return "XOFF"; break;
		case 3: return "BIAS_SETTLE_MC"; break;
		case 4: return "REG_SETTLE_MC"; break;
		case 5: return "MANCAL"; break;
		case 6: return "BIAS_SETTLE"; break;
		case 7: return "REG_SETTLE"; break;
		case 8: return "STARTCAL"; break;
		case 9: return "BWBOOST"; break;
		case 10: return "FS_LOCK"; break;
		case 11: return "IFADCON"; break;
		case 12: return "ENDCAL"; break;
		case 13: return "RX"; break;
		case 14: return "RX_END"; break;
		case 15: return "Reserved"; break;
		case 16: return "TXRX_SWITCH"; break;
		case 17: return "RX_FIFO_ERR"; break;
		case 18: return "FSTXON"; break;
		case 19: return "TX"; break;
		case 20: return "TX_END"; break;
		case 21: return "RXTX_SWITCH"; break;
		case 22: return "TX_FIFO_ERR"; break;
		case 23: return "IFADCON_TXRX"; break;

		default:
			return "UNKNOWN";
			break;
	}
}

uint8_t cc1120_strobe(uint8_t strobe) {
	uint8_t ret;

	spiAcquireBus(&CC1120_SPI);
	spi_enable();
	ret = spi_exchange(strobe);
	spi_disable();
	spiReleaseBus(&CC1120_SPI);

	return ret;
}

uint8_t cc1120_single_read(uint16_t addr) {
	uint8_t val;

	spiAcquireBus(&CC1120_SPI);
	spi_enable();

	if(CC1120_IS_EXTENDED(addr)) {
		addr &= ~0x2F00;
		spi_exchange(CC1120_EXTENDED_MEMORY_ACCESS | CC1120_READ_BIT);
		spi_exchange(addr);
	} else {
		spi_exchange(addr | CC1120_READ_BIT);
	}

	val = spi_exchange(0);

	spi_disable();
	spiReleaseBus(&CC1120_SPI);

	return val;
}

uint8_t cc1120_single_write(uint16_t addr, uint8_t val) {
	uint8_t ret;

	spiAcquireBus(&CC1120_SPI);
	spi_enable();

	if(CC1120_IS_EXTENDED(addr)) {
		addr &= ~0x2F00;
		spi_exchange(CC1120_EXTENDED_MEMORY_ACCESS | CC1120_WRITE_BIT);
		spi_exchange(addr);
	} else {
		spi_exchange(addr | CC1120_WRITE_BIT);
	}

	ret = spi_exchange(val);

	spi_disable();
	spiReleaseBus(&CC1120_SPI);

	return ret;
}

void cc1120_burst_read(uint16_t addr, uint8_t *buffer, uint8_t count) {
	spiAcquireBus(&CC1120_SPI);
	spi_enable();

	if(CC1120_IS_EXTENDED(addr)) {
		addr &= ~0x2F00;
		spi_exchange(CC1120_EXTENDED_MEMORY_ACCESS | CC1120_READ_BIT | CC1120_BURST_BIT);
		spi_exchange(addr);
	} else {
		spi_exchange(addr | CC1120_READ_BIT | CC1120_BURST_BIT);
	}

	spiReceive(&CC1120_SPI, count, buffer);

	spi_disable();
	spiReleaseBus(&CC1120_SPI);
}

void cc1120_burst_write(uint16_t addr, uint8_t *buffer, uint8_t count) {
	spiAcquireBus(&CC1120_SPI);
	spi_enable();

	if(CC1120_IS_EXTENDED(addr)) {
		addr &= ~0x2F00;
		spi_exchange(CC1120_EXTENDED_MEMORY_ACCESS | CC1120_WRITE_BIT | CC1120_BURST_BIT);
		spi_exchange(addr);
	} else {
		spi_exchange(addr | CC1120_WRITE_BIT | CC1120_BURST_BIT);
	}

	spiSend(&CC1120_SPI, count, buffer);

	spi_disable();
	spiReleaseBus(&CC1120_SPI);
}

uint8_t cc1120_txbytes(void) {
	uint8_t txbytes1, txbytes2;

	do {
		cc1120_burst_read(CC1120_NUM_TXBYTES, &txbytes1, 1);
		cc1120_burst_read(CC1120_NUM_TXBYTES, &txbytes2, 1);
		if(txbytes1 - 1 == txbytes2 || txbytes1 - 2 == txbytes2) {
			// XXX Workaround for slow CPU/SPI
			return txbytes2;
		}
	} while(txbytes1 != txbytes2);

	return txbytes1;
}

uint8_t cc1120_read_rxbytes(void) {
	uint8_t rxbytes1, rxbytes2;

	do {
		cc1120_burst_read(CC1120_NUM_RXBYTES, &rxbytes1, 1);
		cc1120_burst_read(CC1120_NUM_RXBYTES, &rxbytes2, 1);
		if(rxbytes1 + 1 == rxbytes2 || rxbytes1 + 2 == rxbytes2) {
			// XXX Workaround for slow CPU/SPI
			return rxbytes2;
		}
	} while(rxbytes1 != rxbytes2);

//	uint8_t rxbytes1;
//	cc1120_burst_read(CC1120_NUM_RXBYTES, &rxbytes1, 1);

	return rxbytes1;
}

void cc1120_write_txfifo(uint8_t *data, uint8_t len) {
	uint8_t status;
	int i;

	cc1120_burst_write(CC1120_TXFIFO, &len, 1);

#define FIRST_TX 8

	i = MIN(len, FIRST_TX);
	cc1120_burst_write(CC1120_TXFIFO, data, i);
	cc1120_strobe(CC1120_STX);

	if(len > i) {
		spiAcquireBus(&CC1120_SPI);
		spi_enable();
		spi_exchange(CC1120_TXFIFO | 0x40);

		for(; i < len; i++) {
			status = spi_exchange(data[i]);

			if((status & CC1120_STATUS_STATE_MASK) ==
					CC1120_STATUS_STATE_TXFIFO_UNDERFLOW) {

				spi_disable();
				spiReleaseBus(&CC1120_SPI);

				// TX FIFO underflow, acknowledge it with an SFTX (else the
				// radio becomes completely unresponsive) followed by an SRX,
				// and break the transmission.

				cc1120_strobe(CC1120_SFTX);
				cc1120_strobe(CC1120_SRX);
				spiAcquireBus(&CC1120_SPI);
				break;
			} else if((status & 0x0f) < 2) {
				spi_disable();
				spiReleaseBus(&CC1120_SPI);

				int to = 100;
				while (!(cc1120_txbytes() < 60 || (cc1120_txbytes() & 0x80) != 0) && to > 0) {
					chThdSleepMilliseconds(1);
					to--;
				}

				if(cc1120_txbytes() & 0x80) {
					// TX FIFO underflow.
					cc1120_strobe(CC1120_SFTX);
					cc1120_strobe(CC1120_SRX);
					commands_printf("TX FIFO underflow");
					break;
				}

				spiAcquireBus(&CC1120_SPI);
				spi_enable();
				spi_exchange(CC1120_TXFIFO | 0x40);
			}
		}

		spi_disable();
		spiReleaseBus(&CC1120_SPI);
	}
}

void cc1120_check_txfifo(void) {
	if(cc1120_state() == CC1120_STATUS_STATE_TXFIFO_UNDERFLOW) {
		// Acknowledge TX FIFO underflow.
		cc1120_strobe(CC1120_SFTX);
		cc1120_strobe(CC1120_SRX);
	}
}

void cc1120_flushrx(void) {
	commands_printf("Flush RX");

	if(cc1120_state() == CC1120_STATE_RXFIFO_OVERFLOW) {
		cc1120_strobe(CC1120_SFRX);
	}

	cc1120_strobe(CC1120_SIDLE);

	int to = 100;
	while (!(cc1120_state() == CC1120_STATE_IDLE) && to > 0) {
		to--;
	}

	cc1120_strobe(CC1120_SFRX);
	cc1120_strobe(CC1120_SRX);
}

int cc1120_transmit(uint8_t *data, int len) {
	if(cc1120_state() == CC1120_STATE_RXFIFO_OVERFLOW) {
		cc1120_flushrx();
	}

	if(len > CC1120_MAX_PAYLOAD) {
		commands_printf("cc1120: too big tx %d\n", len);
		return -1;
	}

	cc1120_strobe(CC1120_SIDLE);
	cc1120_write_txfifo(data, len);

	int to = 1000;
	while (!(cc1120_state() == CC1120_STATE_TX) && to > 0) {
		chThdSleepMilliseconds(1);
		to--;
	}

	if (!to) {
		commands_printf("Didn't enter TX");
	}

	if(cc1120_state() != CC1120_STATE_TX) {
		commands_printf("didn't tx (in %d)\n", cc1120_state());
		cc1120_check_txfifo();
		cc1120_flushrx();
		return -2;
	}

	to = 1000;
	while (!(cc1120_state() != CC1120_STATE_TX) && to > 0) {
		chThdSleepMilliseconds(1);
		to--;
	}

	if(cc1120_state() == CC1120_STATE_TX) {
		commands_printf("didn't end tx (in %d, txbytes %d)\n", cc1120_state(), cc1120_txbytes());
		cc1120_check_txfifo();
		cc1120_flushrx();
		return -3;
	}

	cc1120_check_txfifo();

	return 1;
}

int cc1120_on(void) {
	cc1120_flushrx();
	cc1120_strobe(CC1120_SRX);
	return 1;
}

int cc1120_off(void) {
	cc1120_strobe(CC1120_SIDLE);

	int to = 100;
	while (!(cc1120_state() == CC1120_STATE_IDLE) && to > 0) {
		chThdSleepMilliseconds(1);
		to--;
	}

	cc1120_strobe(CC1120_SPWD);

	return 1;
}

void cc1120_ext_cb(EXTDriver *extp, expchannel_t channel) {
	(void)extp;
	(void)channel;

	chSysLockFromISR();
	chEvtSignalI(isr_tp, (eventmask_t) 1);
	chSysUnlockFromISR();
}

static THD_FUNCTION(isr_thread, arg) {
	(void) arg;
	chRegSetThreadName("CC1120 EXTI");

	isr_tp = chThdGetSelfX();

	for (;;) {
		chEvtWaitAny((eventmask_t) 1);

		commands_printf("CC1120 interrupt");
		rx_interrupt();
	}
}

// Private functions

static int rx_interrupt(void) {
	uint8_t rxbytes, s;

	s = cc1120_state();
	if(s == CC1120_STATE_RXFIFO_OVERFLOW) {
		cc1120_burst_read(CC1120_NUM_RXBYTES, &rxbytes, 1);
		commands_printf("irqflush");
		commands_printf("rxbytes 0x%02x", rxbytes);
		cc1120_flushrx();
		return 1;
	}

	if(s == CC1120_STATE_TXFIFO_UNDERFLOW) {
		commands_printf("irqflushtx");
		cc1120_strobe(CC1120_SFTX);
		cc1120_strobe(CC1120_SRX);
		return 1;
	}

	do {
		rxbytes = cc1120_read_rxbytes();

		if(rxbytes == 0) {
			commands_printf("ISR: FIFO empty");
			return 1;
		}

		if(rxbytes > CC1120_MAX_PAYLOAD) {
			commands_printf("rxbytes too large %d", rxbytes);
			cc1120_flushrx();
			return -1;
		}

		static uint8_t tmpbuf[CC1120_MAX_PAYLOAD];
		cc1120_burst_read(CC1120_RXFIFO, tmpbuf, rxbytes);

		// Print bytes for now
		for (int i = 0;i < rxbytes;i++) {
			commands_printf("RX Byte: %d", tmpbuf[i]);
		}

		rxbytes = cc1120_read_rxbytes();
	} while(rxbytes > 1);

	return 1;
}

static void spi_enable(void) {
	palClearPad(CC1120_PORT_CS, CC1120_PIN_CS);

	int timeout = 100;
	while (palReadPad(CC1120_PORT_MISO, CC1120_PIN_MISO)) {
		chThdSleepMilliseconds(1);
		timeout--;
		if (!timeout) {
			commands_printf("SPI Timeout");
			break;
		}
	}
}

static void spi_disable(void) {
	palSetPad(CC1120_PORT_CS, CC1120_PIN_CS);
}

static uint8_t spi_exchange(uint8_t x) {
	uint8_t rx;
	spiExchange(&CC1120_SPI, 1, &x, &rx);
	return rx;
}

// Below code is adapted from TI's CC112x/CC1175 ERRATA
#define VCDAC_START_OFFSET  2
#define FS_VCO2_INDEX       0
#define FS_VCO4_INDEX       1
#define FS_CHP_INDEX        2

void cc1120_calibrate_manual(void) {
	uint8_t original_fs_cal2;
	uint8_t calResults_for_vcdac_start_high[3];
	uint8_t calResults_for_vcdac_start_mid[3];
	uint8_t marcstate;
	uint8_t writeByte;

	// 1) Set VCO cap-array to 0 (FS_VCO2 = 0x00)
	writeByte = 0x00;
	cc1120_burst_write(CC1120_FS_VCO2, &writeByte, 1);

	// 2) Start with high VCDAC (original VCDAC_START + 2):
	cc1120_burst_read(CC1120_FS_CAL2, &original_fs_cal2, 1);
	writeByte = original_fs_cal2 + VCDAC_START_OFFSET;
	cc1120_burst_write(CC1120_FS_CAL2, &writeByte, 1);

	// 3) Calibrate and wait for calibration to be done (radio back in IDLE state)
	cc1120_strobe(CC1120_SCAL);
	do {
		cc1120_burst_read(CC1120_MARCSTATE, &marcstate, 1);
	} while(marcstate != 0x41);

	// 4) Read FS_VCO2, FS_VCO4 and FS_CHP register obtained with high VCDAC_START value
	cc1120_burst_read(CC1120_FS_VCO2,
			&calResults_for_vcdac_start_high[FS_VCO2_INDEX], 1);
	cc1120_burst_read(CC1120_FS_VCO4,
			&calResults_for_vcdac_start_high[FS_VCO4_INDEX], 1);
	cc1120_burst_read(CC1120_FS_CHP, &calResults_for_vcdac_start_high[FS_CHP_INDEX],
			1);

	// 5) Set VCO cap-array to 0 (FS_VCO2 = 0x00)
	writeByte = 0x00;
	cc1120_burst_write(CC1120_FS_VCO2, &writeByte, 1);

	// 6) Continue with mid VCDAC (original VCDAC_START):
	writeByte = original_fs_cal2;
	cc1120_burst_write(CC1120_FS_CAL2, &writeByte, 1);

	// 7) Calibrate and wait for calibration to be done (radio back in IDLE state)
	cc1120_strobe(CC1120_SCAL);
	do {
		cc1120_burst_read(CC1120_MARCSTATE, &marcstate, 1);
	} while(marcstate != 0x41);

	// 8) Read FS_VCO2, FS_VCO4 and FS_CHP register obtained with mid VCDAC_START value
	cc1120_burst_read(CC1120_FS_VCO2, &calResults_for_vcdac_start_mid[FS_VCO2_INDEX],
			1);
	cc1120_burst_read(CC1120_FS_VCO4, &calResults_for_vcdac_start_mid[FS_VCO4_INDEX],
			1);
	cc1120_burst_read(CC1120_FS_CHP, &calResults_for_vcdac_start_mid[FS_CHP_INDEX],
			1);

	// 9) Write back highest FS_VCO2 and corresponding FS_VCO and FS_CHP result
	if(calResults_for_vcdac_start_high[FS_VCO2_INDEX]
									   > calResults_for_vcdac_start_mid[FS_VCO2_INDEX]) {
		writeByte = calResults_for_vcdac_start_high[FS_VCO2_INDEX];
		cc1120_burst_write(CC1120_FS_VCO2, &writeByte, 1);
		writeByte = calResults_for_vcdac_start_high[FS_VCO4_INDEX];
		cc1120_burst_write(CC1120_FS_VCO4, &writeByte, 1);
		writeByte = calResults_for_vcdac_start_high[FS_CHP_INDEX];
		cc1120_burst_write(CC1120_FS_CHP, &writeByte, 1);
	} else {
		writeByte = calResults_for_vcdac_start_mid[FS_VCO2_INDEX];
		cc1120_burst_write(CC1120_FS_VCO2, &writeByte, 1);
		writeByte = calResults_for_vcdac_start_mid[FS_VCO4_INDEX];
		cc1120_burst_write(CC1120_FS_VCO4, &writeByte, 1);
		writeByte = calResults_for_vcdac_start_mid[FS_CHP_INDEX];
		cc1120_burst_write(CC1120_FS_CHP, &writeByte, 1);
	}
}
