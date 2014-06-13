#include <QStandardPaths>
#include <QSettings>
#include <QMessageBox>
#include <QDir>
#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "ScratchWebApi.h"
#include "Scratchcc.h"
#include "ArduinoBuilder.h"
#include "ArduinoUploader.h"
#include <QSerialPortInfo>
#include "SettingsDialog.h"
#include <QMovie>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    scratchApi_(0),
    scratchcc_(0),
    arduinoBuilder_(0),
    uploader_(0)
{
    initSettings();

    ui->setupUi(this);

    ui->labelBuildError->setText(QString());

    QSettings settings;
    QString lastProjectId = settings.value("lastProjectId", "22951621").toString();
    ui->lineEditProjectId->setText(lastProjectId);

    connect(ui->actionExit, SIGNAL(triggered()), this, SLOT(close()));
    connect(ui->actionSettings, SIGNAL(triggered()), this, SLOT(showSettingsDialog()));

    scratchApi_ = new ScratchWebApi(this);
    connect(scratchApi_, SIGNAL(complete(QString)), SLOT(projectLoaded(QString)));
    connect(scratchApi_, SIGNAL(error(QString)), SLOT(buildError(QString)));

    checkBoardConnection();

    boardConnectionTimer_ = new QTimer(this);
    boardConnectionTimer_->setInterval(3000);
    connect(boardConnectionTimer_, SIGNAL(timeout()), SLOT(checkBoardConnection()));
    boardConnectionTimer_->start();
}

MainWindow::~MainWindow()
{
    delete ui;
    cleanup();
}

void MainWindow::on_pushButtonRun_clicked()
{
    QString projectId = ui->lineEditProjectId->text();
    if (projectId.isEmpty()) {
        QMessageBox msgBox(QMessageBox::Warning, "Error", "Please enter the project Id", QMessageBox::Ok);
        msgBox.exec();
        return;
    }
    QSettings settings;
    settings.setValue("lastProjectId", projectId);

    if (comPort_.isEmpty()) {
        QMessageBox msgBox(QMessageBox::Warning, "Error", "No Arduino connected. Please plug one in.", QMessageBox::Ok);
        msgBox.exec();
        return;
    }

    ui->labelBuildError->setText(QString());
    QMovie *movie = new QMovie(":/assets/animatedprogress.gif");
    movie->setCacheMode(QMovie::CacheAll);
    ui->labelMainIcon->setMovie(movie);
    movie->start();
    ui->labelMainCaption->setText("Getting project from Scratch...");
    ui->pushButtonRun->setEnabled(false);
    scratchApi_->loadProject(projectId);
}

void MainWindow::showSettingsDialog()
{
    SettingsDialog dlg;
    dlg.exec();
}

void MainWindow::projectLoaded(const QString &projectSource)
{
    qDebug("Downloaded project");
    ui->labelMainCaption->setText("Converting to C...");
    QString projectId = ui->lineEditProjectId->text();
    QSettings settings;
    QString boardType = settings.value("board", "leonardo").toString();
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::DataLocation);
    projectPath_ = dataPath + QDir::separator() + "projects" + QDir::separator() + projectId + QDir::separator() + boardType;
    QDir::root().mkpath(projectPath_);

    QString scratchFilePath = projectPath_ + QDir::separator() + "scratch.json";
    QFile scratchFile(scratchFilePath);
    scratchFile.open(QFile::WriteOnly);
    scratchFile.write(projectSource.toUtf8());
    scratchFile.close();

    inoFilePath_ = projectPath_ + QDir::separator() + "scratch.ino";
    scratchcc_ = new Scratchcc(scratchFilePath, inoFilePath_, this);
    connect(scratchcc_, SIGNAL(complete()), SLOT(scratchccComplete()));
    connect(scratchcc_, SIGNAL(error(QString)), SLOT(buildError(QString)));
    scratchcc_->compile();
}

void MainWindow::scratchccComplete()
{
    qDebug("Converted to C");
    ui->labelMainCaption->setText("Compiling C code for Arduino...");

    hexFilePath_ = projectPath_ + QDir::separator() + "scratch.hex";
    arduinoBuilder_ = new ArduinoBuilder(inoFilePath_, hexFilePath_, projectPath_, this);
    connect(arduinoBuilder_, SIGNAL(complete()), SLOT(buildComplete()));
    connect(arduinoBuilder_, SIGNAL(error(QString)), SLOT(buildError(QString)));
    arduinoBuilder_->build();
}

void MainWindow::buildComplete()
{
    qDebug("Built hex file");
    ui->labelMainCaption->setText("Uploading to Arduino...");
    uploader_ = new ArduinoUploader(hexFilePath_, comPort_, this);
    connect(uploader_, SIGNAL(complete()), SLOT(uploadComplete()));
    connect(uploader_, SIGNAL(error(QString)), SLOT(buildError(QString)));
    uploader_->upload();
}

void MainWindow::uploadComplete()
{
    ui->labelMainIcon->setPixmap(QPixmap(":/assets/happy.png"));
    ui->labelMainCaption->setText("It worked!");
    cleanup();
    qDebug("Uploaded");
    ui->pushButtonRun->setEnabled(true);
}

void MainWindow::checkBoardConnection()
{
    QString connectedPortName;
    QString connectedPortDescription;
    foreach (const QSerialPortInfo &portInfo, QSerialPortInfo::availablePorts()) {
        if (portInfo.description().startsWith("Arduino")) {
            connectedPortName = portInfo.portName();
            connectedPortDescription = portInfo.description();
            break;
        }
    }

    if (connectedPortName != comPort_) {
        comPort_ = connectedPortName;
        if (comPort_.isEmpty()) {
            ui->labelBoardConnectionStatus->setText("No Arduino Connected.");
            ui->labelBoardConnectionStatusIcon->setPixmap(QPixmap(":/assets/warning.png"));
        } else {
            ui->labelBoardConnectionStatus->setText(QString("%1 on %2").arg(connectedPortDescription).arg(connectedPortName));
            ui->labelBoardConnectionStatusIcon->setPixmap(QPixmap(":/assets/checkmark.png"));
        }
    }
}

void MainWindow::buildError(const QString &error)
{
    ui->labelMainIcon->setPixmap(QPixmap(":/assets/bug.png"));
    ui->labelMainCaption->setText("Something went wrong.");
    ui->labelBuildError->setText(error);
    ui->pushButtonRun->setEnabled(true);
}

void MainWindow::initSettings()
{
    // set the default values for settings vars in one place
    QSettings settings;
    if (!settings.contains("ArduinoSdkPath"))
#ifdef Q_OS_WIN
        settings.setValue("ArduinoSdkPath", "C:/Program Files/Arduino");
#else
        settings.setValue("ArduinoSdkPath", "/usr/share/arduino");
#endif
    if (!settings.contains("board"))
        settings.setValue("board", "leonardo");
    if (!settings.contains("mcu"))
        settings.setValue("mcu", "atmega32u4");
    if (!settings.contains("CpuFrequency"))
        settings.setValue("CpuFrequency", "16000000");
    if (!settings.contains("ScratchccPath"))
#ifdef Q_OS_WIN
        settings.setValue("ScratchccPath", "C:/Users/Josh/Documents/GitHub/scratchcc");
#else
        settings.setValue("ScratchccPath", "~/scratchcc");
#endif
}

void MainWindow::cleanup()
{
    if (scratchcc_) {
        scratchcc_->deleteLater();
        scratchcc_ = 0;
    }
    if (arduinoBuilder_) {
        arduinoBuilder_->deleteLater();
        arduinoBuilder_ = 0;
    }
    if (uploader_) {
        uploader_->deleteLater();
        uploader_ = 0;
    }
}
