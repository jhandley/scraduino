#include "Scratchcc.h"
#include <QProcess>
#include <QTemporaryFile>
#include <QDir>
#include <QSettings>

Scratchcc::Scratchcc(const QString &sourceFile, const QString &resultFile, QObject *parent) :
    QObject(parent),
    source_(sourceFile),
    dest_(resultFile)
{
}

void Scratchcc::compile()
{
    QSettings settings;
    QString scratchccPath = QDir::fromNativeSeparators(settings.value("ScratchccPath").toString());
    compileProcess_ = new QProcess(this);
    compileProcess_->setProcessChannelMode(QProcess::ForwardedChannels);
    connect(compileProcess_, SIGNAL(finished(int,QProcess::ExitStatus)), SLOT(processFinished(int,QProcess::ExitStatus)));
    connect(compileProcess_, SIGNAL(error(QProcess::ProcessError)), SLOT(processError(QProcess::ProcessError)));
#ifdef Q_OS_WIN
    QString elixerExecutable = "elixir.bat";
#else
    QString elixerExecutable = "elixir";
#endif

    compileProcess_->start(elixerExecutable, QStringList() << "-pa" << scratchccPath + "/_build/dev/lib/scratchcc/ebin" <<
            "-pa" << scratchccPath + "/_build/dev/lib/jsex/ebin" <<
            "-pa" << scratchccPath + "/_build/dev/lib/jsx/ebin" <<
            "-e" << QString("Scratchcc.doit(\"%1\", \"%2\")").arg(QDir::fromNativeSeparators(source_)).arg(QDir::fromNativeSeparators(dest_)) <<
                           "-e" << "\":init.stop\"");
}

void Scratchcc::processError(QProcess::ProcessError e)
{
    Q_UNUSED(e);
    QString errorMessage = compileProcess_->errorString();
    emit error(errorMessage);
}

void Scratchcc::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitCode == 0 && exitStatus == QProcess::NormalExit) {
        emit complete();
    } else {
        emit error(QString("Compile process failed with code %1").arg(exitCode));
    }
}
