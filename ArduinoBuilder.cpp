#include "ArduinoBuilder.h"
#include <QSettings>
#include <QProcess>
#include <QFile>
#include <QFileInfo>
#include <QDir>

ArduinoBuilder::ArduinoBuilder(const QString &inoFilePath,
                               const QString &hexFilePath,
                               const QString &intermediateFilePath,
                               QObject *parent) :
    QObject(parent),
    inoFilePath_(inoFilePath),
    hexFilePath_(hexFilePath),
    intermediateFilePath_(intermediateFilePath),
    state_(START)
{
    QSettings settings;
    arduinoSdkPath_ = settings.value("ArduinoSdkPath").toString();
    boardType_ = settings.value("board").toString();
    mcu_ = settings.value("mcu").toString();
    cpuFreq_ = settings.value("CpuFrequency").toString();
}

void ArduinoBuilder::build()
{
    QFile inoFile(inoFilePath_);
    if (!inoFile.open(QFile::ReadOnly)) {
        die(QString("Unable to open source file %1").arg(inoFilePath_));
        return;
    }

    QString cppSourcePath = intermediateFilePath_ + QDir::separator() + QFileInfo(inoFilePath_).baseName() + ".cpp";
    QFile cppSourceFile(cppSourcePath);
    if (!cppSourceFile.open(QFile::WriteOnly)) {
        die(QString("Unable to open file %1 for writing").arg(cppSourcePath));
        return;
    }

    cppSourceFile.write(QString("#include <Arduino.h>\n").toUtf8());
    cppSourceFile.write(inoFile.readAll());
    cppSourceFile.close();
    inoFile.close();

    QStringList copiedIncludeFiles;
    copiedIncludeFiles << ":/protothreads/pt-sem.h"
            << ":/protothreads/lc.h"
            << ":/protothreads/lc-addrlabels.h"
            << ":/protothreads/lc-switch.h"
            << ":/protothreads/pt.h";
    foreach (QString includeFilePath, copiedIncludeFiles) {
        QString destPath = intermediateFilePath_ + QDir::separator() + QFileInfo(includeFilePath).baseName() + ".h";
        QFile::copy(includeFilePath, destPath);
    }

    sourceFiles_ << cppSourcePath <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/main.cpp" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/avr-libc/malloc.c" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/avr-libc/realloc.c" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/WInterrupts.c" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/wiring.c" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/wiring_analog.c" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/wiring_digital.c" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/wiring_pulse.c" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/wiring_shift.c" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/CDC.cpp" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/HardwareSerial.cpp" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/HID.cpp" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/IPAddress.cpp" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/new.cpp" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/Print.cpp" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/Stream.cpp" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/Tone.cpp" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/USBCore.cpp" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/WMath.cpp" <<
                    arduinoSdkPath_ + "/hardware/arduino/cores/arduino/WString.cpp";
    state_ = COMPILE;
    compile();
}

void ArduinoBuilder::processError(QProcess::ProcessError error)
{
    Q_UNUSED(error);
    QProcess *process = qobject_cast<QProcess*>(sender());
    die(process->errorString());
}

void ArduinoBuilder::processFinished(int exitCode)
{
    QProcess *process = qobject_cast<QProcess*>(sender());
    if (exitCode != 0) {
        QByteArray errors = process->readAllStandardError();
        die(QString::fromUtf8(errors));
    }
    processes_.removeOne(process);
    process->deleteLater();

    switch (state_) {
    case START:
        break;
    case COMPILE:
        if (processes_.empty()) {
            state_ = LINK;
            link();
        }
        break;
    case LINK:
        state_ = OBJCOPY1;
        objcopy1();
        break;
    case OBJCOPY1:
        state_ = OBJCOPY2;
        objcopy2();
        break;
    case OBJCOPY2:
        state_ = DONE;
        emit complete();
        break;
    case DONE:
        break;
    case ERROR:
        break;
    }
}

void ArduinoBuilder::compile()
{
    qDebug("Compiling...");
    QStringList compileFlags;
    compileFlags << "-c" << "-g" << "-Os" << "-Wall" << "-fno-exceptions" << "-ffunction-sections" << "-fdata-sections"
            << "-mmcu="+mcu_ << "-DF_CPU="+cpuFreq_+"L" << "-MMD" << "-DUSB_VID=0x2341" << "-DUSB_PID=0x8036"
            << "-DARDUINO=105" << "-I" + arduinoSdkPath_ + "/hardware/arduino/cores/arduino"
            << "-I" + arduinoSdkPath_ + "/hardware/arduino/variants/" + boardType_;
    QString cCompiler = arduinoSdkPath_ + "/hardware/tools/avr/bin/avr-gcc";
    QString cppCompiler = arduinoSdkPath_ + "/hardware/tools/avr/bin/avr-g++";
    foreach (QString sourceFile, sourceFiles_) {
        QString compiler;
        if (sourceFile.endsWith(".c")) {
            compiler = cCompiler;
        } else if (sourceFile.endsWith(".cpp")) {
            compiler = cppCompiler;
        } else {
            die(QString("Invalid source file extension must be .c or .cpp: %1").arg(sourceFile));
        }
        QStringList args = compileFlags;
        args << sourceFile << "-o" << intermediateFilePath_ + QDir::separator() + QFileInfo(sourceFile).baseName() + ".o";
        qDebug("%s %s", qPrintable(compiler), qPrintable(args.join(' ')));
        QProcess *compileProcess = new QProcess(this);
        connect(compileProcess, SIGNAL(finished(int)), SLOT(processFinished(int)));
        connect(compileProcess, SIGNAL(error(QProcess::ProcessError)), SLOT(processError(QProcess::ProcessError)));
        processes_ << compileProcess;
        compileProcess->start(compiler, args);
    }
}

void ArduinoBuilder::link()
{
    qDebug("Linking...");
    QString linker = arduinoSdkPath_ + "/hardware/tools/avr/bin/avr-gcc";
    QStringList args;
    args << "-Os" << "-Wl,--gc-sections"
         << "-mmcu="+mcu_ << "-lm"
         << "-o" << intermediateFilePath_ + QDir::separator() + QFileInfo(inoFilePath_).baseName() + ".elf";
    foreach (QString sourceFile, sourceFiles_) {
        args << intermediateFilePath_ +  QDir::separator() + QFileInfo(sourceFile).baseName() + ".o";
    }
    qDebug("%s %s", qPrintable(linker), qPrintable(args.join(' ')));
    QProcess *linkerProcess = new QProcess(this);
    connect(linkerProcess, SIGNAL(finished(int)), SLOT(processFinished(int)));
    connect(linkerProcess, SIGNAL(error(QProcess::ProcessError)), SLOT(processError(QProcess::ProcessError)));
    processes_ << linkerProcess;
    linkerProcess->start(linker, args);
}

void ArduinoBuilder::objcopy1()
{
    qDebug("Build hex step 1...");
    QString objcopy = arduinoSdkPath_ + "/hardware/tools/avr/bin/avr-objcopy";
    QStringList args;
    args << "-O" << "ihex" << "-j" << ".eeprom"
         << "--set-section-flags=.eeprom=alloc,load" << "--no-change-warnings"
         << "--change-section-lma" << ".eeprom=0"
         << intermediateFilePath_ + QDir::separator() + QFileInfo(inoFilePath_).baseName() + ".elf"
         << intermediateFilePath_ + QDir::separator() + QFileInfo(inoFilePath_).baseName() + ".eep";
    qDebug("%s %s", qPrintable(objcopy), qPrintable(args.join(' ')));
    QProcess *objcopyProcess = new QProcess(this);
    connect(objcopyProcess, SIGNAL(finished(int)), SLOT(processFinished(int)));
    connect(objcopyProcess, SIGNAL(error(QProcess::ProcessError)), SLOT(processError(QProcess::ProcessError)));
    processes_ << objcopyProcess;
    objcopyProcess->start(objcopy, args);
}

void ArduinoBuilder::objcopy2()
{
    //%ARDUINO_PATH%\hardware\tools\avr\bin\avr-objcopy -O ihex -R .eeprom %1.elf %1.hex
    qDebug("Build hex step 2...");
    QString objcopy = arduinoSdkPath_ + "/hardware/tools/avr/bin/avr-objcopy";
    QStringList args;
    args << "-O" << "ihex" << "-R" << ".eeprom"
         << intermediateFilePath_ + QDir::separator() + QFileInfo(inoFilePath_).baseName() + ".elf"
         << hexFilePath_;
    qDebug("%s %s", qPrintable(objcopy), qPrintable(args.join(' ')));
    QProcess *objcopyProcess = new QProcess(this);
    connect(objcopyProcess, SIGNAL(finished(int)), SLOT(processFinished(int)));
    connect(objcopyProcess, SIGNAL(error(QProcess::ProcessError)), SLOT(processError(QProcess::ProcessError)));
    processes_ << objcopyProcess;
    objcopyProcess->start(objcopy, args);
}

void ArduinoBuilder::die(const QString &errorMessage)
{
    state_ = ERROR;
    foreach (QProcess *process, processes_) {
        process->disconnect(this);
        process->kill();
    }

    emit error(errorMessage);
}
