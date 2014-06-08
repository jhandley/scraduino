#include "SettingsDialog.h"
#include "ui_SettingsDialog.h"
#include <QSettings>
#include <QFileDialog>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{    
    ui->setupUi(this);
    QSettings settings;
    ui->lineEditArduinoSdkPath->setText(settings.value("ArduinoSdkPath").toString());
    ui->lineEditScratchccPath->setText(settings.value("ScratchccPath").toString());
    ui->comboBoxBoard->setCurrentIndex(ui->comboBoxBoard->findText(settings.value("board").toString()));
    ui->lineEditScratchccPath->setText(settings.value("ScratchccPath").toString());
    ui->lineEditMicro->setText(settings.value("mcu").toString());
    ui->lineEditCpuFrequency->setText(settings.value("CpuFrequency").toString());
}

SettingsDialog::~SettingsDialog()
{
    delete ui;
}

void SettingsDialog::on_pushButtonBrowseArduinoSdkPath_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Locate Arduino SDK",
                                                    ui->lineEditArduinoSdkPath->text(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    ui->lineEditArduinoSdkPath->setText(dir);
}

void SettingsDialog::on_pushButtonBrowseScratchccPath_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this, "Locate Scratchcc",
                                                    ui->lineEditScratchccPath->text(),
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    ui->lineEditScratchccPath->setText(dir);
}

void SettingsDialog::on_buttonBox_accepted()
{
    QSettings settings;
    settings.setValue("ArduinoSdkPath", ui->lineEditArduinoSdkPath->text());
    settings.setValue("ScratchccPath", ui->lineEditScratchccPath->text());
    settings.setValue("board", ui->comboBoxBoard->currentText());
    settings.setValue("mcu", ui->lineEditMicro->text());
    settings.setValue("CpuFrequency", ui->lineEditCpuFrequency->text());
}
