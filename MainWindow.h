#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class ScratchWebApi;
class Scratchcc;
class ArduinoBuilder;
class ArduinoUploader;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_pushButtonRun_clicked();
    void projectLoaded(const QString &projectSource);
    void projectLoadError(const QString &errorMessage);
    void scratchccComplete(bool ok, const QString &error);
    void buildComplete(bool ok, const QString &error);
    void uploadComplete(bool ok, const QString &error);

private:
    Ui::MainWindow *ui;
    QString projectPath_;
    QString inoFilePath_;
    QString hexFilePath_;
    QString comPort_;
    ScratchWebApi *scratchApi_;
    Scratchcc *scratchcc_;
    ArduinoBuilder *arduinoBuilder_;
    ArduinoUploader *uploader_;
};

#endif // MAINWINDOW_H
