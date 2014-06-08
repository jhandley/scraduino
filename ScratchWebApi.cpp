#include "ScratchWebApi.h"
#include <QUrl>
#include <QNetworkRequest>
#include <QNetworkReply>

ScratchWebApi::ScratchWebApi(QObject *parent) :
    QObject(parent)
{
    accessManager_ = new QNetworkAccessManager(this);
}

void ScratchWebApi::loadProject(const QString &projectId)
{
    QUrl requestUrl("http://projects.scratch.mit.edu/");
    requestUrl.setPath(QString("/internalapi/project/%1/get/").arg(projectId));

    QNetworkRequest request;
    request.setUrl(requestUrl);
    QNetworkReply *reply = accessManager_->get(request);
    connect(reply, SIGNAL(finished()), SLOT(handleLoadProjectReply()));
}

void ScratchWebApi::handleLoadProjectReply()
{
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!hasError(reply)) {

        QString projectSource = reply->readAll();
        emit projectLoaded(projectSource);

    } else {
        handleError(reply);
    }

    reply->deleteLater();
}

bool ScratchWebApi::hasError(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        return true;
    }

    return false;
}

void ScratchWebApi::handleError(QNetworkReply *reply)
{
    QNetworkReply::NetworkError err = reply->error();
    QString requestPath = reply->request().url().path();
    QString replyBody = QString::fromUtf8(reply->readAll());
    int httpError = reply->attribute( QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qWarning("Network error: requestPath = %s, httpcode=%d, errorMessage=%s, replyBody=%s",
           qPrintable(requestPath),
           httpError,
           qPrintable(reply->errorString()),
           qPrintable(replyBody));

    switch (err) {
        case QNetworkReply::NoError:
            break;

        case QNetworkReply::AuthenticationRequiredError:
            emit error("Authentication failed");
            break;

        case QNetworkReply::ContentOperationNotPermittedError:
            emit error("Content operation not permitted");
            break;

        case QNetworkReply::ConnectionRefusedError:
            emit error("Connection refused");
            break;

        case QNetworkReply::RemoteHostClosedError:
            emit error("Remote host closed");
            break;

        case QNetworkReply::HostNotFoundError:
            emit error("Host not found");
            break;

        case QNetworkReply::TimeoutError:
            emit error("Timeout");
            break;

        case QNetworkReply::OperationCanceledError:
            emit error("Operation canceled");
            break;

        case QNetworkReply::SslHandshakeFailedError:
            emit error("SSL handshake failed");
            break;

        case QNetworkReply::TemporaryNetworkFailureError:
            emit error("Temporary network failure");
            break;

        case QNetworkReply::UnknownNetworkError:
            emit error("Unknown network error");
            break;


        default:
        {
            if (replyBody.isEmpty())
                emit error(reply->errorString());
            else {
                emit error(replyBody);
            }
            break;
        }

    }
}
