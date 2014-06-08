#include "Scratchcc.h"
#include <QProcess>
#include <QTemporaryFile>
#include <QDir>

Scratchcc::Scratchcc(const QString &sourceFile, const QString &resultFile, QObject *parent) :
    QObject(parent),
    source_(sourceFile),
    dest_(resultFile)
{
}

void Scratchcc::compile()
{
    compileProcess_ = new QProcess(this);
    compileProcess_->setProcessChannelMode(QProcess::ForwardedChannels);
    connect(compileProcess_, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(compileProcess_, SIGNAL(error(QProcess::ProcessError)), SLOT(processError(QProcess::ProcessError)));
    compileProcess_->start("elixir.bat", QStringList() << "-pa" << "C:/Users/Josh/Documents/GitHub/scratchcc/_build/dev/lib/scratchcc/ebin" <<
            "-pa" << "C:/Users/Josh/Documents/GitHub/scratchcc/_build/dev/lib/jsex/ebin" <<
            "-pa" << "C:/Users/Josh/Documents/GitHub/scratchcc/_build/dev/lib/jsx/ebin" <<
            "-e" << QString("Scratchcc.doit(\"%1\", \"%2\")").arg(QDir::fromNativeSeparators(source_)).arg(QDir::fromNativeSeparators(dest_)) <<
                           "-e" << "\":init.stop\"");
}

void Scratchcc::processError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    QString errorMessage = compileProcess_->errorString();
    emit complete(false, errorMessage);
}

void Scratchcc::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        emit complete(true, "");
    } else {
        emit complete(false, QString("Compile process failed with code %1").arg(exitCode));
    }
}
