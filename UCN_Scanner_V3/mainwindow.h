#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QDateTime>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QString>
#include <QTimer>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    bool running;
    bool new_run;
    bool sis_is_running;
    bool slow_started;
    int miss_call_sis;
    int runnumber;
    int runlimit;
    double currentX = 0;
    double currentY = 0;
    char command;
    unsigned long long int last_time[20];

    // Josh's public definitions, should match arduino
    double usteps = 32.0;
    double RPM = 60.0;
    double MAX_STEPS_LENGTH = 4214.8215; // 59 cm
    double MAX_STEPS_WIDTH = 1992.375; // 28cm

    FILE *fSIS;
    //std::fstream fVMEpars;
    bool blind;

    QDateTime start_time,stop_time;

    bool SendStart(void *sckt);
    bool SendStop(void *sckt);
    bool SendCheck(void *sckt);
    bool CheckDaqConnections();
    void GetCurrentRunNumber();
    void init_port();
    void transmitVal(char cmd, float val1, float val2);
    void transmitScanPath();
    void setupScanGrid();
    double calcTimeForRegion(int rowMin, int rowMax, int colMin, int colMax);
    void updatePosDisplay();
    void checkArduinoScanComplete();
    double calcTime();
    void readArduino(QByteArray* out);
    void paintGridByState();

    // Realtime update spacing
    double lastSpacing = 5.0;

    // Check scanning ends
    QTimer* scanMonitorTimer;
    QByteArray incomingBuffer;

private:
    Ui::MainWindow *ui;
    void *context;
    void *socket_sis3316;
    void *socket_control;
    void *socket_parser;

    QSerialPort *port;

private slots:
    void on_posUpdate_clicked();

    void on_returnHome_clicked();

    void on_runScan_clicked();

    void on_stopScan_clicked();

    void on_xBack_clicked();

    void on_yBack_clicked();

    void on_xFor_clicked();

    void on_yFor_clicked();

    void on_testSerial_clicked();

    void on_debugBox_toggled(bool checked);

};
#endif // MAINWINDOW_H


