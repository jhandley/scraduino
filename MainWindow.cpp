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

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QSettings settings;
    QString lastProjectId = settings.value("lastProjectId", "22951621").toString();
    ui->lineEditProjectId->setText(lastProjectId);

    scratchApi_ = new ScratchWebApi(this);
    connect(scratchApi_, SIGNAL(projectLoaded(QString)), SLOT(projectLoaded(QString)));
    connect(scratchApi_, SIGNAL(error(QString)), SLOT(projectLoadError(QString)));
}

MainWindow::~MainWindow()
{
    delete ui;
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

    comPort_.clear();
    qDebug("Looking for serial port devices:");
    foreach (const QSerialPortInfo &portInfo, QSerialPortInfo::availablePorts()) {
        qDebug("%s: %s", qPrintable(portInfo.portName()), qPrintable(portInfo.description()));
        if (portInfo.description().startsWith("Arduino")) {
            comPort_ = portInfo.portName();
            break;
        }
    }
    if (comPort_.isEmpty()) {
        QMessageBox msgBox(QMessageBox::Warning, "Error", "No Arduino connected. Please plug one in.", QMessageBox::Ok);
        msgBox.exec();
        return;
    }
    scratchApi_->loadProject(projectId);
}

void MainWindow::projectLoaded(const QString &projectSource)
{
    qDebug("Downloaded project");
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
    connect(scratchcc_, SIGNAL(complete(bool,QString)), SLOT(scratchccComplete(bool,QString)));
    scratchcc_->compile();
}

void MainWindow::projectLoadError(const QString &errorMessage)
{
    qDebug("%s", qPrintable(errorMessage));
}

void MainWindow::scratchccComplete(bool ok, const QString &error)
{
    if (ok) {
        qDebug("Converted to C");
        hexFilePath_ = projectPath_ + QDir::separator() + "scratch.hex";
        arduinoBuilder_ = new ArduinoBuilder(inoFilePath_, hexFilePath_, projectPath_, this);
        connect(arduinoBuilder_, SIGNAL(complete(bool,QString)), SLOT(buildComplete(bool,QString)));
        arduinoBuilder_->build();
    } else {
        qDebug("Scratchcc error: %s", qPrintable(error));
    }
    scratchcc_->deleteLater();
}

void MainWindow::buildComplete(bool ok, const QString &error)
{
    if (ok) {
        qDebug("Built hex file");
        uploader_ = new ArduinoUploader(hexFilePath_, comPort_, this);
        connect(uploader_, SIGNAL(complete(bool,QString)), SLOT(uploadComplete(bool,QString)));
        uploader_->upload();
    } else {
        qDebug("Build error: %s", qPrintable(error));
    }
}

void MainWindow::uploadComplete(bool ok, const QString &error)
{
    if (ok)
        qDebug("Uploaded");
    else
        qDebug("Upload error: %s", qPrintable(error));
}
