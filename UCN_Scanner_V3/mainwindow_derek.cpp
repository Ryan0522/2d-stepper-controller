#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QtDebug>
#include <QByteArray>
#include <unistd.h>
#include <QtMath>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QThread>
#include <cmath>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    port(new QSerialPort(this))
{
    ui->setupUi(this);
    ui->label_4->setPixmap(QPixmap(":/images/label_4.png"));

    ui->xPosEdit->setText("0.000");
    ui->yPosEdit->setText("0.000");
    ui->stopScan->setEnabled(false);

    ui->sampleSpacing->setText("5");
    ui->sampleTime->setText("5");
    ui->runTimeEnd->setAlignment(Qt::AlignCenter);
    ui->runTimeEnd->setText("--/--, --:--, --");

    scanMonitorTimer = new QTimer(this);
    connect(scanMonitorTimer, &QTimer::timeout, this, &MainWindow::checkArduinoScanComplete);
    connect(ui->sampleSpacing, &QLineEdit::editingFinished, this, &MainWindow::setupScanGrid);

    setupScanGrid();
    init_port();
}

MainWindow::~MainWindow()
{
    delete ui;
    //port->close(); // Close  arduino serial port on exit
}

//**********************
//Begin Josh's additions
//**********************

//***Command value table***//
//Base state / no move == 0//
//X Back == 1; Y Back == 2 //
//X For == 3; Y For == 4   //
//Run Scan == 5; Stop == 6 //
//Update Position == 7;    //
//Return Home == 8;        //
//Total length = 59cm//
//Total width = 28cm  //
//*************************//

void MainWindow::checkArduinoScanComplete()
{
    QByteArray response;
    if (port->waitForReadyRead(200)) {
        readArduino(&response);
        incomingBuffer += response;

        if (ui->debugBox->isChecked())
            qDebug() << "<DEBUG> Arduino Response: " << incomingBuffer;

        QList<QByteArray> parts = incomingBuffer.split('>');
        for (int i = 0; i < parts.size() - 1; ++i) {
            const QByteArray& part = parts[i];
            int startIdx = part.indexOf('<');
            if (startIdx == -1) continue;

            QByteArray message = part.mid(startIdx);

            if (message.contains("<SCAN_DONE")) {
                scanMonitorTimer->stop();
                ui->runScan->setEnabled(true);
                ui->runTimeEnd->setText(("--/--, --:--, --"));
                ui->stopScan->setEnabled(false);
                qDebug() << "Scan complete: received '<SCAN_DONE>' from Arduino.";
                incomingBuffer.clear();
                return;
            }
        }
        incomingBuffer = parts.last();
    }
}

void MainWindow::readArduino(QByteArray* out=nullptr) {
    QByteArray response;
    if (port->waitForReadyRead(500)) {
        response = port->readAll();
        while (port->waitForReadyRead(100)) {
            response += port->readAll();
        }
        response.replace("\r\n", ". ");
    }
    if (out) {
        *out = response;
    } else {
        qDebug() << "Arduino response:" << response;
    }
}

void MainWindow::init_port()
{
    //Set port configuration
    port->setPortName("/dev/ttyACM0");
    port->setBaudRate(QSerialPort::Baud9600);
    port->setFlowControl(QSerialPort::NoFlowControl);
    port->setParity(QSerialPort::NoParity);
    port->setDataBits(QSerialPort::Data8);
    port->setStopBits(QSerialPort::OneStop);

    // Open port
    if (!port->open(QIODevice::ReadWrite)) {
        qDebug() << "Failed to open serial port: " << port->errorString();
        QMessageBox::warning(this, "PORT ERROR", "Arduino port could not be opened!");
    } else {
        qDebug() << "Serial port opened successfully.";
        qDebug() << "Port name:" << port->portName();
        qDebug() << "Baud rate:" << port->baudRate();
    }

    // Disable UI to prevent bugs
    this->setEnabled(false);
    ui->statusBar->showMessage("Initializing Arduino. Please wait...", 4500);
    QTimer::singleShot(4500, this, [=]() {
        this->setEnabled(true);
    });
}

void MainWindow::transmitVal(char cmd, float val1, float val2)
{
    QString valStr = QString::number(val1, 'f', 3);
    QString valStr2 = QString::number(val2, 'f', 3);

    QString packet = QString("<%1,%2,%3>").arg(cmd).arg(valStr).arg(valStr2);
    qDebug() << "Sending command:" << packet;

    if (port->isOpen()) {
        port->write(packet.toUtf8());
    } else {
        QMessageBox::warning(this, "PORT ERROR", "Arduino port is not open!");
    }
    readArduino();
}

// Scanning Grid Setup
void MainWindow::setupScanGrid() {
    double spacing = ui->sampleSpacing->text().toDouble();
    int roundedSpacing = std::max(1, static_cast<int>(std::round(spacing)));
    if (spacing != roundedSpacing) {
        ui->sampleSpacing->setText(QString::number(roundedSpacing));
        QMessageBox::warning(this, "Invalid Spacing", "Spacing must be integer (cm) and ≥ 1!");
    }
    spacing = roundedSpacing;

    // Save previous selected positions
    QVector<QPointF> selectedPositions;
    int oldNumRows = ui->scanGrid->rowCount();
    int oldNumCols = ui->scanGrid->columnCount();
    double oldSpacing = lastSpacing > 0 ? lastSpacing : spacing;

    for (int i = 0; i < oldNumRows; ++i) {
        for (int j = 0; j < oldNumCols; ++j) {
            QTableWidgetItem* item = ui->scanGrid->item(i, j);
            if (item && item->isSelected()) {
                double y_cm = i * oldSpacing;
                double x_cm = j * oldSpacing;
                selectedPositions.append(QPointF(x_cm, y_cm));
            }
        }
    }

    int numCols = std::ceil(28.0 / spacing); // short side (Y)
    int numRows = std::ceil(59.0 / spacing); // long side (X)

    ui->scanGrid->clear();
    ui->scanGrid->setRowCount(numRows);
    ui->scanGrid->setColumnCount(numCols);
    ui->scanGrid->setSelectionMode(QAbstractItemView::ContiguousSelection);
    ui->scanGrid->setSelectionBehavior(QAbstractItemView::SelectItems);
    ui->scanGrid->setEditTriggers(QAbstractItemView::NoEditTriggers);

    ui->scanGrid->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->scanGrid->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->scanGrid->horizontalHeader()->setVisible(false);
    ui->scanGrid->verticalHeader()->setVisible(false);
    ui->scanGrid->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->scanGrid->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    for (int i = 0; i < numRows; ++i) {
        for (int j = 0; j < numCols; ++j) {
            QTableWidgetItem* item = new QTableWidgetItem();
            item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
            ui->scanGrid->setItem(i, j, item);
        }
    }

    if (ui->debugBox->isChecked())
        qDebug() << "<DEBUG> Selected positions: " << selectedPositions;

    // Restore selection
    if (selectedPositions.isEmpty()) {
        ui->scanGrid->selectAll();
    } else {
        double xMin = selectedPositions.first().x();
        double xMax = xMin;
        double yMin = selectedPositions.first().y();
        double yMax = yMin;

        for (const QPointF& pos : selectedPositions) {
            xMin = std::min(xMin, pos.x());
            xMax = std::max(xMax, pos.x());
            yMin = std::min(yMin, pos.y());
            yMax = std::max(yMax, pos.y());
        }

        qDebug() << xMin << "," << xMax << "," << yMin << "," << yMax;

        int rowMin = static_cast<int>(std::round(yMin / spacing));
        int rowMax = static_cast<int>(std::round(yMax / spacing));
        int colMin = static_cast<int>(std::round(xMin / spacing));
        int colMax = static_cast<int>(std::round(xMax / spacing));

        rowMin = std::max(0, rowMin);
        rowMax = std::min(numRows - 1, rowMax);
        colMin = std::max(0, colMin);
        colMax = std::min(numCols - 1, colMax);

        for (int i = rowMin; i <= rowMax; ++i) {
            for (int j = colMin; j <= colMax; ++j) {
                QTableWidgetItem* item = ui->scanGrid->item(i, j);
                if (item) item->setSelected(true);
            }
        }
    }

    connect(ui->scanGrid, &QTableWidget::itemSelectionChanged, this, [this]() {
        for (int i = 0; i < ui->scanGrid->rowCount(); ++i) {
            for (int j = 0; j < ui->scanGrid->columnCount(); ++j) {
                QTableWidgetItem* item = ui->scanGrid->item(i, j);
                if (!item) continue;
                item->setBackground(item->isSelected() ? Qt::green : Qt::white);
            }
        }
    });

    lastSpacing = spacing;
}
void MainWindow::updatePosDisplay()
{
    double xInCm = 0;
    double yInCm = 0;

    xInCm = (currentX / usteps) / (71.0 + (15.0 / 32.0));
    yInCm = (currentY / usteps) / (71.0 + (5.0 / 32.0));

    QString newX = QString::number(xInCm, 'f', 3);
    ui->xPosEdit->setText(newX);

    QString newY = QString::number(yInCm, 'f', 3);
    ui->yPosEdit->setText(newY);

    ui->posUpdate->setEnabled(true);
    ui->returnHome->setEnabled(true);
    ui->runScan->setEnabled(true);
}

double MainWindow::calcTime()
{
    float spacing = 0.0;
    float timing = 0.0;
    double timeOut = 0.0;
    double timeIn = 0.0;
    double timeUp = 0.0;
    double totalTime = 0.0; // total time for scan in minutes (min)
    double minPerRot = 0.0;
    double sec2min = 0.0;
    double cmToStepsWid = 0.0;
    double cmToStepsLen = 0.0;
    double lengthUSteps = 0.0;
    double widthUSteps = 0.0;


    QString sampleTiming = ui->sampleTime->text();
    timing = sampleTiming.toDouble();
    sec2min = timing / 60.0;

    QString sampleSpace = ui->sampleSpacing->text();
    spacing = sampleSpace.toDouble();
    cmToStepsWid = spacing * (71.0 + (5.0 / 32.0));
    cmToStepsLen = spacing * (71.0 + (15.0 / 32.0));
    lengthUSteps = cmToStepsLen * usteps;
    widthUSteps = cmToStepsWid * usteps;

    minPerRot = 1.0 / RPM;

    timeOut = ((qFloor((MAX_STEPS_WIDTH * usteps) / widthUSteps) + 1) * sec2min)
              + ((minPerRot * (widthUSteps / (200 * usteps))) * qFloor((MAX_STEPS_WIDTH * usteps) / widthUSteps));

    timeIn = (((qFloor(((MAX_STEPS_WIDTH * usteps) / widthUSteps)) * widthUSteps) / (200 * usteps)) * minPerRot);

    timeUp = (((MAX_STEPS_LENGTH * usteps) / lengthUSteps) * (lengthUSteps / (200 * usteps)) * minPerRot);

    totalTime = (timeOut + timeIn) * (((MAX_STEPS_LENGTH * usteps) / lengthUSteps) + 1) + timeUp;

    return totalTime;
}

double MainWindow::calcTimeForRegion(int rowMin, int rowMax, int colMin, int colMax)
{
    double spacing = ui->sampleSpacing->text().toDouble();
    double timing = ui->sampleTime->text().toDouble();

    int rows = rowMax - rowMin + 1;
    int cols = colMax - colMin + 1;
    int totalPoints = rows * cols;

    // Time spent at each point
    double totalSampleTime = totalPoints * timing; // in seconds

    // Estimate motor move time (assume 1 step per spacing)
    double lenPerStep = spacing * (71.0 + 15.0 / 32.0);  // X motor
    double widPerStep = spacing * (71.0 + 5.0 / 32.0);   // Y motor

    double stepsX = (cols - 1) * lenPerStep * usteps;
    double stepsY = (rows - 1) * widPerStep * usteps;

    // Use RPM to get time per step (same formula as setup)
    double minPerRot = 1.0 / RPM;
    double timePerStep = minPerRot / (200.0);  // one full step

    double moveTimeSec = (stepsX + stepsY) * timePerStep * 60.0;  // convert to seconds

    double totalTime = totalSampleTime + moveTimeSec;
    return totalTime / 60.0;  // Return in minutes
}


void MainWindow::on_runScan_clicked()
{
    double spacing = ui->sampleSpacing->text().toDouble();
    double timing = ui->sampleTime->text().toDouble();
    scanMonitorTimer->start(100);

    if (spacing <= 0 || spacing > 28.0) {
        QMessageBox::warning(this, "Invalid Spacing", "Sample spacing must be > 0 and < 28.0 cm");
        return;
    }

    // Auto-return to home (0, 0) if needed
    if (currentX != 0 || currentY != 0) {
        transmitVal('8', 0, 0);  // Return home
        currentX = 0;
        currentY = 0;
        ui->xPosEdit->setText("0.000");
        ui->yPosEdit->setText("0.000");
    }

    int rowMin = ui->scanGrid->rowCount(), rowMax = -1;
    int colMin = ui->scanGrid->columnCount(), colMax = -1;
    for (int i = 0; i < ui->scanGrid->rowCount(); ++i) {
        for (int j = 0; j < ui->scanGrid->columnCount(); ++j) {
            QTableWidgetItem* item = ui->scanGrid->item(i, j);
            if (item && item->isSelected()) {
                rowMin = std::min(rowMin, i);
                rowMax = std::max(rowMax, i);
                colMin = std::min(colMin, j);
                colMax = std::max(colMax, j);
            }
        }
    }
    if (rowMin > rowMax || colMin > colMax) {
        QMessageBox::warning(this, "No Region", "No scan region selected.");
        return;
    }

    double runTime = calcTimeForRegion(rowMin, rowMax, colMin, colMax);
    QDateTime now = QDateTime::currentDateTime();
    QDateTime finishTime = now.addSecs(static_cast<int>(runTime * 60));
    ui->runTimeEnd->setText(finishTime.toString("MM/dd, hh:mm, ap"));

    QString packet = QString("<5,%1,%2,%3,%4,%5,%6>")
                         .arg(spacing, 0, 'f', 3)
                         .arg(timing, 0, 'f', 3)
                         .arg(rowMin)
                         .arg(rowMax)
                         .arg(colMin)
                         .arg(colMax);

    port->write(packet.toUtf8());
    qDebug() << "Sent scan region:" << packet;
    readArduino();

    ui->posUpdate->setEnabled(true);
    ui->returnHome->setEnabled(true);
    ui->runScan->setEnabled(false);
    ui->stopScan->setEnabled(true);
}

void MainWindow::on_posUpdate_clicked()
{
    ui->returnHome->setEnabled(true);
    ui->runScan->setEnabled(true);

    double xPosDesire;
    double yPosDesire;
    command = '7';

    QString xPosString = ui->xPosEdit->text();
    QString yPosString = ui->yPosEdit->text();
    xPosDesire = xPosString.toDouble();
    yPosDesire = yPosString.toDouble();
    if ((xPosDesire >= 0) & (yPosDesire >= 0))
    {
        if ((xPosDesire > 59.000) | (yPosDesire > 28.000))
        {
            QMessageBox::warning(this, "Position Update Error", "New position must be less than 59.000 cm for X and 28.000 cm for Y!");
        }
        else
        {
            transmitVal(command, xPosDesire, yPosDesire);

            currentX = xPosDesire * (71.0 + (15.0 / 32.0)) * usteps;
            currentY = yPosDesire * (71.0 + (5.0 / 32.0)) * usteps;
            qDebug("%f usteps", currentX);
            qDebug("%f usteps", currentY);
        }
    }
    else
    {
        QMessageBox::warning(this, "Position Update Error", "New position must be greater than or equal to (0,0)!!");
    }

}

void MainWindow::on_returnHome_clicked()
{
    ui->posUpdate->setEnabled(true);
    ui->returnHome->setEnabled(false);
    ui->runScan->setEnabled(true);
    command = '8';

    transmitVal(command, 0, 0);
    ui->xPosEdit->setText("0.000");
    ui->yPosEdit->setText("0.000");
    currentX = 0.0;
    currentY = 0.0;
}

void MainWindow::on_stopScan_clicked()
{
    scanMonitorTimer->stop();

    ui->posUpdate->setEnabled(true);
    ui->returnHome->setEnabled(true);
    ui->runScan->setEnabled(true);
    ui->stopScan->setEnabled(false);
    ui->runTimeEnd->setText("--/--, --:--, --");

    command = '6';
    transmitVal(command, 0, 0); // Send stop command

    QByteArray stopResponse;
    readArduino(&stopResponse);

    char status = stopResponse.isEmpty() ? '\0' : stopResponse[0];
    if (status == '9') {
        qDebug() << "Scan successfully stopped.";
    } else if (status == '0') {
        qDebug() << "Scan was never running.";
    } else {
        qDebug() << "Unexpected stop response: " << stopResponse;
    }

    QByteArray posResponse;
    readArduino(&posResponse);

    // Attempt to parse position (format: <X><Y>)
    // Note: method indexOf returns -1 if character not found
    int xStart = posResponse.indexOf('<');
    int xEnd   = posResponse.indexOf('>', xStart);
    int yStart = posResponse.indexOf('<', xEnd);
    int yEnd   = posResponse.indexOf('>', yStart);

    if (xStart != -1 && xEnd != -1 && yStart != -1 && yEnd != -1) {
        // Note: method mid trims out < and >
        QByteArray xStr = posResponse.mid(xStart + 1, xEnd - xStart - 1);
        QByteArray yStr = posResponse.mid(yStart + 1, yEnd - yStart - 1);
        currentX = xStr.toDouble();
        currentY = yStr.toDouble();
        qDebug() << "Parsed X:" << currentX << "Parsed Y:" << currentY;
    } else {
        qDebug() << "Failed to parse position from: " << posResponse;
    }

    updatePosDisplay();
    qDebug() << "Stop procedure complete.";

    QByteArray trailingGarbage;
    readArduino(&trailingGarbage);
    if (!trailingGarbage.isEmpty())
        qDebug() << "Trailing serial data discarded:" << trailingGarbage;

}

void MainWindow::on_xBack_clicked()
{

    command = '1';
    if (currentX > 0)
    {
        transmitVal(command, 0, 0);

        currentX = currentX - usteps;
        updatePosDisplay();
    }
    else
    {
        QMessageBox::warning(this, "Take Step Error", "At min position, can't step back!");
    }
}

void MainWindow::on_yBack_clicked()
{

    command = '2';
    if (currentY > 0)
    {
        transmitVal(command, 0, 0);

        currentY = currentY - usteps;
        updatePosDisplay();
    }
    else
    {
        QMessageBox::warning(this, "Take Step Error", "At min position, can't step back!");
    }
}

void MainWindow::on_xFor_clicked()
{

    command = '3';
    if (currentX < (4214.8215 * usteps))
    {
        transmitVal(command, 0, 0);

        currentX = currentX + usteps;
        updatePosDisplay();
    }
    else
    {
        QMessageBox::warning(this, "Take Step Error", "At max position, can't step forward!");
    }
}

void MainWindow::on_yFor_clicked()
{

    command = '4';
    if (currentY < (1992.375 * usteps))
    {
        transmitVal(command, 0 , 0);

        currentY = currentY + usteps;
        updatePosDisplay();
    }
    else
    {
        QMessageBox::warning(this, "Take Step Error", "At max position, can't step forward!");
    }
}

void MainWindow::on_testSerial_clicked()
{
    if (!port->isOpen()) {
        QMessageBox::warning(this, "PORT ERROR", "Arduino port is not opened!");
        return;
    }

    for (int i = 0; i < 10; ++i){

        // Preparing Messsage
        QString cmd = QString("<T, %1, 0>").arg(i+1);
        QByteArray packet = cmd.toUtf8();

        // Sending Message
        qint64 bytesWritten = port->write(packet);
        if (bytesWritten == -1) {
            qDebug() << "Failed to write to port: " << port->errorString();
            return;
        } else if (bytesWritten != packet.size()) {
            qDebug() << "Only partial data written to port.";
            return;
        } else {
            qDebug() << bytesWritten << "bytes sent: " << packet;
        }

        if (!port->waitForBytesWritten(1000)) {
            qDebug() << "Write timed out:" << port->errorString();
            return;
        }

        QByteArray response;
        readArduino(&response);

        if (response.isEmpty()) {
            qDebug() << "Read timed out or no response received for iteration" << i;
        }

        QThread::msleep(150);
    }
}

void MainWindow::on_debugBox_toggled(bool checked)
{
    if (port->isOpen()) {
        char cmd = '9';
        float val1 = checked;
        float val2 = 0.0;
        transmitVal(cmd, val1, val2);
        qDebug() << "Debug mode toggled. Sent <9," << val1 << "," << val2 << ">";
        readArduino();
    } else {
        QMessageBox::warning(this, "PORT ERROR", "Arduino port is not open!");
    }
}

