#ifndef CARINTERFACE_H
#define CARINTERFACE_H

#include <QWidget>
#include <QVector>
#include "datatypes.h"
#include "mapwidget.h"

namespace Ui {
class CarInterface;
}

class CarInterface : public QWidget
{
    Q_OBJECT

public:
    explicit CarInterface(QWidget *parent = 0);
    ~CarInterface();
    void setID(int id);
    int getId();
    bool pollData();
    void setOrientation(double roll, double pitch, double yaw);
    void setImuData(IMU_DATA data);
    void setMap(MapWidget *map);

signals:
    void terminalCmd(quint8 id, QString cmd);

private slots:
    void terminalPrint(quint8 id, QString str);

    void on_terminalSendButton_clicked();
    void on_terminalClearButton_clicked();
    void on_yawOffsetSlider_valueChanged(int value);

    void on_idBox_valueChanged(int arg1);

private:
    Ui::CarInterface *ui;
    QVector<double> accelXData;
    QVector<double> accelYData;
    QVector<double> accelZData;
    QVector<double> gyroXData;
    QVector<double> gyroYData;
    QVector<double> gyroZData;
    QVector<double> magXData;
    QVector<double> magYData;
    QVector<double> magZData;
    QVector<double> accelGyroMagXAxis;
    int maxSampleSize;
    MapWidget *mMap;
    int mId;

};

#endif // CARINTERFACE_H