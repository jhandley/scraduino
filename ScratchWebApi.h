#ifndef SCRATCHWEBAPI_H
#define SCRATCHWEBAPI_H

#include <QObject>
#include <QNetworkAccessManager>

class ScratchWebApi : public QObject
{
    Q_OBJECT
public:
    explicit ScratchWebApi(QObject *parent = 0);
    
signals:
    
    void projectLoaded(const QString& projectText);
    void error(const QString& errorMessage);

public slots:
    
    void loadProject(const QString& projectId);

private slots:
    void handleLoadProjectReply();
    void handleError(QNetworkReply *reply);

private:
    bool hasError(QNetworkReply *reply);

    QNetworkAccessManager *accessManager_;

};

#endif // SCRATCHWEBAPI_H
