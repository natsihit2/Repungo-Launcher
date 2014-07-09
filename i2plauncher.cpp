#include "i2plauncher.h"
#include "repugnoapplication.h"
#include <QDirIterator>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <stdio.h>



I2PLauncher::I2PLauncher(QString jrePath, QString i2pPath)
{
    // TODO: Add checks that the directories are correct & exists even they should
    // have been checked by earlier methods in the stack already.
    m_jrePath = jrePath;
    m_i2pPath = i2pPath;
    //runner = new I2PRunner();
}

void I2PLauncher::Run()
{
    /*
    runner->moveToThread(&workerThread);

    // Setting connections between threads
    connect(&workerThread, &QThread::finished, runner, &QObject::deleteLater);
    connect(this, &I2PLauncher::operate, runner, &I2PRunner::runI2P);
    connect(runner, &I2PRunner::resultReady, this, &I2PLauncher::handleResults);

    workerThread.start();*/
    QProcess p;
    QString cmd = I2PLauncher::GenerateLaunchCommand();

    // Environment overriding
    QStringList env = QProcess::systemEnvironment();
    env << RepugnoApplication::applicationDirPath() +QDir::separator()+ "Temp";
    env << "_JAVA_OPTIONS=-Xmx"+QString(DEFAULT_MEMORY)+"M"; // This is required for a JRE to start.
    env << "JAVA_HOME="+m_jrePath;
    qDebug() << "JAVA_HOME="+m_jrePath;
    p.setEnvironment(env);

    // TODO: Add optional?
    QDir *logDir = new QDir(RepugnoApplication::applicationDirPath() +QDir::separator()+"Logs");
    if (!logDir->exists()) logDir->mkdir(logDir->absolutePath());

    p.setStandardErrorFile(logDir->absolutePath()+QDir::separator()+"i2p.stderr.log",QIODevice::Append);
    p.setStandardOutputFile(logDir->absolutePath()+QDir::separator()+"i2p.stdout.log",QIODevice::Append);

    //TODO: Check if location is different from i2psnark.config and change in case it is.

    /*
BOOL WINAPI CreateProcess(
  _In_opt_     LPCTSTR lpApplicationName,
  _Inout_opt_  LPTSTR lpCommandLine,
  _In_opt_     LPSECURITY_ATTRIBUTES lpProcessAttributes,
  _In_opt_     LPSECURITY_ATTRIBUTES lpThreadAttributes,
  _In_         BOOL bInheritHandles,
  _In_         DWORD dwCreationFlags,
  _In_opt_     LPVOID lpEnvironment,
  _In_opt_     LPCTSTR lpCurrentDirectory,
  _In_         LPSTARTUPINFO lpStartupInfo,
  _Out_        LPPROCESS_INFORMATION lpProcessInformation
);
*/

    // Running
    qDebug() << "CMD for I2P is: " << cmd;
    // MARK: When not starting detached a console window on windows won't spawn.
#ifdef WIN32
    //TODO: For fuck sake, find a better way to do it.
    p.startDetached("nircmd.exe execmd CALL "+cmd);
#else
    p.startDetached(cmd);
#endif
    p.waitForFinished(-1);
#if QT_VERSION < 0x050300
    qint64 i2pPid = p.pid();
    //ProcessId first added in Qt 5.3
#else
    qint64 i2pPid = p.processId();
#endif
    qDebug() << "I2P should have started now. With process id: " << QString::number(i2pPid);
}

QString I2PLauncher::GenerateLaunchCommand()
{
    // Collect I2P jar files
    QString classPath = "";
    QString compiledString = "";
    QDirIterator it(m_i2pPath+QDir::separator()+"lib", QDirIterator::NoIteratorFlags);
    while (it.hasNext())
    {
        QString tmp = it.next();
        QFileInfo *fTest = new QFileInfo(tmp);
        if ( fTest->isDir() ) continue; // Only allow jars for now
        classPath +=
#ifdef WIN32
                ";"
#else
                ":"
#endif
                +tmp;
        qDebug() << "[+] Added "<< tmp << " to classpath.";
    }
    qDebug() << "[+] Classpath looks like: " << classPath;

    QString javaExec =
#ifdef WIN32
    "\\bin\\java.exe "
#else
    "/bin/java "
#endif
    ;
    QString i2p_config = QCoreApplication::applicationDirPath() + QDir::separator() + "Config" + QDir::separator() + "i2p";
    QString mainClass = I2PMAINCLASS;
    compiledString += m_jrePath;
    compiledString += javaExec;
    // TODO: Allow alternative java JRE
    compiledString += " -cp ." + classPath;
    compiledString += " -Di2p.dir.base="+m_i2pPath +
            " -Dorg.mortbay.util.FileResource.checkAliases=false -DloggerFilenameOverride="+
            QCoreApplication::applicationDirPath() + QDir::separator() +"log"+ QDir::separator() +"i2p-log-router-@.txt " +
            "-Djava.library.path=."+ QDir::separator()+ QCoreApplication::applicationDirPath() + QDir::separator() +"lib "+
            "-Dorg.mortbay.http.Version.paranoid=true -Di2p.dir.config="+ i2p_config + " " + mainClass;
    qDebug() << "CMD so far: " << compiledString;

    //I2PRunner::i2pCommand = compiledString;
    /*if (Constants::i2pRunCommand == NULL)
    {
        Constants::i2pRunCommand = compiledString;
    }*/
    return compiledString;
}
