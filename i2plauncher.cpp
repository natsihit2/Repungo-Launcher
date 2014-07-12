#include "i2plauncher.h"
#include "repugnoapplication.h"
#include <QDirIterator>
#include <QDir>
#include <QFileInfo>
#include <QDebug>


#ifdef WIN32

#include <windows.h>
#endif

#include <stdio.h>

#ifdef WIN32
#include <string.h>
LPWSTR ConvertToLPWSTR( const std::string& s )
{
  LPWSTR ws = new wchar_t[s.size()+1]; // +1 for zero at the end
  copy( s.begin(), s.end(), ws );
  ws[s.size()] = 0; // zero at the end
  return ws;
}
#endif

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
    // Running
    qDebug() << "CMD for I2P is: " << cmd;
    // MARK: When not starting detached a console window on windows won't spawn.
#ifdef WIN32

#if WINVER == 0x0602
// Why?
// http://msdn.microsoft.com/en-us/library/windows/desktop/ms686331(v=vs.85).aspx
// http://msdn.microsoft.com/en-us/library/windows/desktop/aa383745(v=vs.85).aspx#macros_for_conditional_declarations
#include "Processthreadsapi.h"
#endif

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory( &si, sizeof(si) );
    si.cb = sizeof(si);
    ZeroMemory( &pi, sizeof(pi) );

    // Start the child process.
    if( !CreateProcess( NULL,   // No module name (use command line)
        ConvertToLPWSTR(cmd.toStdString()),        // Command line
        NULL,           // Process handle not inheritable
        NULL,           // Thread handle not inheritable
        FALSE,          // Set handle inheritance to FALSE
        CREATE_NO_WINDOW, // Don't show console to the end user.
        NULL,           // Use parent's environment block
        NULL,           // Use parent's starting directory
        &si,            // Pointer to STARTUPINFO structure
        &pi )           // Pointer to PROCESS_INFORMATION structure
    )
    {
        //qDebug() <<  sprintf( "CreateProcess failed (%d).\n", GetLastError()) 
        return;
    }
    WaitForSingleObject( pi.hProcess, INFINITE );

    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
#else

    p.startDetached(cmd);
    p.waitForFinished(-1);
#if QT_VERSION < 0x050300
    qint64 i2pPid = p.pid();
    //ProcessId first added in Qt 5.3
#else
    qint64 i2pPid = p.processId();
#endif
    qDebug() << "I2P should have started now. With process id: " << QString::number(i2pPid);

#endif

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

    QString javaExec = QDir::separator() + QString("bin") + QDir::separator() + "java";
#ifdef WIN32
    javaExec = javaExec + ".exe";
#endif
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

    return compiledString;
}
