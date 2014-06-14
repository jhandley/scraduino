#ifndef ARDUINOBUILDER_H
#define ARDUINOBUILDER_H

#include <QObject>
#include <QProcess>

class ArduinoBuilder : public QObject
{
    Q_OBJECT
public:
    explicit ArduinoBuilder(const QString &inoFilePath,
                            const QString &hexFilePath,
                            const QString &intermediateFilePath,
                            QObject *parent = 0);
    
signals:
    void complete();
    void error(const QString &error);

public slots:
    void build();

private slots:

    void processError(QProcess::ProcessError error);
    void processFinished(int exitCode);

private:

    void compile();
    void link();
    void objcopy1();
    void objcopy2();
    void dieLater(const QString &errorMessage);

    QString inoFilePath_;
    QString hexFilePath_;
    QString intermediateFilePath_;

    enum State {
        START,
        COMPILE,
        LINK,
        OBJCOPY1,
        OBJCOPY2,
        DONE,
        ERROR
    };
    State state_;
    QString arduinoSdkPath_;
    QString boardType_;
    QString mcu_;
    QString cpuFreq_;
    QStringList sourceFiles_;
    QList<QProcess*> processes_;
    QString processErrorMessage_;
};

#endif // ARDUINOBUILDER_H
