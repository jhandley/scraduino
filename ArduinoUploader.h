#ifndef ARDUINOUPLOADER_H
#define ARDUINOUPLOADER_H

#include <QObject>
#include <QProcess>
#include <QTimer>

class ArduinoUploader : public QObject
{
    Q_OBJECT
public:
    explicit ArduinoUploader(const QString &hexFilePath,
                             const QString &comPort,
                             QObject *parent = 0);
    
signals:

    void complete();
    void error(const QString &error);

public slots:

    void upload();

private slots:
    void checkPort();
    void avrDudeError(QProcess::ProcessError error);
    void avrDudeProcessFinished(int exitCode);

private:

    void findLeonardoPort();
    QStringList listPorts() const;
    void runAvrDude();

    QString hexFilePath_;
    QString comPort_;
    QTimer *portWaitTimer_;
    QStringList portsBefore_;
    int portWaitTimeElapsedMs_;
    QProcess *avrDudeProcess_;
};

#endif // ARDUINOUPLOADER_H
