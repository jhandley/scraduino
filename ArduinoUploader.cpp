#include "ArduinoUploader.h"
#include <QSerialPortInfo>
#include <QSerialPort>
#include <QSettings>

ArduinoUploader::ArduinoUploader(const QString &hexFilePath, const QString &comPort, QObject *parent) :
    QObject(parent),
    hexFilePath_(hexFilePath),
    comPort_(comPort)
{
}

void ArduinoUploader::upload()
{
    QSettings settings;
    QString boardType = settings.value("board").toString();

    // need to do a little dance for Leonardo and derivatives:
    // open then close the port at the magic baudrate (usually 1200 bps) first
    // to signal to the sketch that it should reset into bootloader. after doing
    // this wait a moment for the bootloader to enumerate. On Windows, also must
    // deal with the fact that the COM port number changes from bootloader to
    // sketch.
    if (boardType == "leonardo") {
        findLeonardoPort();
    } else {
        runAvrDude();
    }
}

void ArduinoUploader::checkPort()
{
    portWaitTimeElapsedMs_ += portWaitTimer_->interval();

    if (portWaitTimeElapsedMs_ > 10000) {
        // give up
        emit complete(false, "Unable to find Leonardo port. Try unplug/replug/reset/prayer.");
        return;
    }

    QStringList now = listPorts();
    QStringList diff = now;
    foreach (QString p, portsBefore_)
        diff.removeAll(p);
    qDebug("Before: {%s}, After {%s}, Diff {%s}",
           qPrintable(portsBefore_.join(" ")), qPrintable(now.join(" ")), qPrintable(diff.join(" ")));
    if (diff.size() > 0) {
        comPort_ = diff.first();
        qDebug("Found Leonardo upload port: %s", qPrintable(comPort_));
        portWaitTimer_->stop();
        portWaitTimer_->disconnect(this);
        portWaitTimer_->deleteLater();
        runAvrDude();
    }

    // Keep track of port that disappears
    portsBefore_ = now;

    // On Windows, it can take a long time for the port to disappear and
    // come back, so use a longer time out before assuming that the selected
    // port is the bootloader (not the sketch).
#ifdef Q_OS_WIN
#define WAIT_FOR_PORT_TO_COME_BACK_MS 5000
#else
#define WAIT_FOR_PORT_TO_COME_BACK_MS 500
#endif

    if (portWaitTimeElapsedMs_ >= WAIT_FOR_PORT_TO_COME_BACK_MS && now.contains(comPort_)) {
        qDebug("Uploading using selected port: %s", qPrintable(comPort_));
        portWaitTimer_->stop();
        portWaitTimer_->disconnect(this);
        portWaitTimer_->deleteLater();
        runAvrDude();
    }
}

void ArduinoUploader::avrDudeError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    emit complete(false, avrDudeProcess_->errorString());
}

void ArduinoUploader::avrDudeProcessFinished(int exitCode)
{
    if (exitCode != 0) {
        QByteArray errors = avrDudeProcess_->readAllStandardError();
        emit complete(false, QString::fromUtf8(errors));
    }
}

void ArduinoUploader::findLeonardoPort()
{
    // Toggle 1200 bps on selected serial port to force board reset.
    portsBefore_ = listPorts();
    if (portsBefore_.contains(comPort_)) {
        qDebug("Forcing reset using 1200bps open/close on port %s", qPrintable(comPort_));
        QSerialPort serialPort(comPort_);
        if (serialPort.open(QIODevice::ReadWrite)) {
            serialPort.setBaudRate(QSerialPort::Baud1200);
            serialPort.setDataBits(QSerialPort::Data8);
            serialPort.setParity(QSerialPort::NoParity);
            serialPort.setStopBits(QSerialPort::OneStop);
            serialPort.close();
        }
    }

    // Wait for a port to appear on the list
    portWaitTimeElapsedMs_ = 0;
    portWaitTimer_ = new QTimer(this);
    portWaitTimer_->setInterval(250);
    connect(portWaitTimer_, SIGNAL(timeout()), SLOT(checkPort()));
    portWaitTimer_->start();
}

QStringList ArduinoUploader::listPorts() const
{
    QStringList ports;
    qDebug("Looking for serial port devices:");
    foreach (const QSerialPortInfo &portInfo, QSerialPortInfo::availablePorts()) {
        qDebug("%s: %s", qPrintable(portInfo.portName()), qPrintable(portInfo.description()));
        ports << portInfo.portName();
    }
    return ports;
}

void ArduinoUploader::runAvrDude()
{
    qDebug("Running AVR dude...");
    QSettings settings;
    QString arduinoSdkPath = settings.value("ArduinoSdkPath").toString();
    QString mcu = settings.value("mcu").toString();

#ifdef Q_OS_WIN
#define SERIAL_PORT_PREFIX "\\\\.\\"
#else
#define SERIAL_PORT_PREFIX ""
#endif

    QString avrDudePath = arduinoSdkPath + "/hardware/tools/avr/bin/avrdude";
    QStringList args;
    args << "-C" + arduinoSdkPath + "/hardware/tools/avr/etc/avrdude.conf"
         << "-v" << "-v" << "-v" << "-v" << "-p" + mcu << "-cavr109"
         << "-P"SERIAL_PORT_PREFIX + comPort_ << "-b57600"
         << "-D" << "-Uflash:w:" + hexFilePath_ + ":i";
    avrDudeProcess_ = new QProcess(this);
    avrDudeProcess_->setProcessChannelMode(QProcess::ForwardedChannels);
    connect(avrDudeProcess_, SIGNAL(finished(int)), SLOT(avrDudeProcessFinished(int)));
    connect(avrDudeProcess_, SIGNAL(error(QProcess::ProcessError)), SLOT(avrDudeError(QProcess::ProcessError)));
    avrDudeProcess_->start(avrDudePath, args);
}

