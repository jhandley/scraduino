#ifndef SCRATCHTOARDUINO_H
#define SCRATCHTOARDUINO_H

#include <QObject>
#include <QProcess>

class Scratchcc : public QObject
{
    Q_OBJECT
public:
    explicit Scratchcc(const QString &sourceFile, const QString &resultFile, QObject *parent = 0);
    
signals:
    
    void complete(bool ok, const QString &error);

public slots:

    void compile();

private slots:
    void processError(QProcess::ProcessError error);
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:

    QString source_;
    QString dest_;
    QProcess *compileProcess_;
};

#endif // SCRATCHTOARDUINO_H
