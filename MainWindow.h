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
class QMovie;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    
private slots:
    void on_pushButtonRun_clicked();
    void showSettingsDialog();
    void projectLoaded(const QString &projectSource);
    void scratchccComplete();
    void buildComplete();
    void uploadComplete();
    void checkBoardConnection();
    void buildError(const QString &error);

private:

    void initSettings();
    void cleanup();

    Ui::MainWindow *ui;
    QString projectPath_;
    QString inoFilePath_;
    QString hexFilePath_;
    QString comPort_;
    ScratchWebApi *scratchApi_;
    Scratchcc *scratchcc_;
    ArduinoBuilder *arduinoBuilder_;
    ArduinoUploader *uploader_;
    QTimer *boardConnectionTimer_;
};

#endif // MAINWINDOW_H
