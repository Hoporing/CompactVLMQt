#pragma once

#include <windows.h>
#include <winhttp.h>
#include <QString>

// Replaced MFC CString, atlstr.h with QString
// Replaced AfxMessageBox with qWarning, TRACE with qDebug
class TranslateServerManager
{
public:
    TranslateServerManager();
    ~TranslateServerManager();

    bool    StartServer();
    void    StopServer();
    QString Translate(const QString &text,
                      const QString &fromLang = "ko",
                      const QString &toLang   = "en");
    bool    IsServerRunning();

private:
    QString GetServerExePath();
    bool    WaitForServerReady(DWORD timeoutMs);
    bool    CheckServerHealth();
    QString SendTranslateRequest(const QString &text,
                                 const QString &fromLang,
                                 const QString &toLang);
    void    KillExistingServers();

    HANDLE  m_hProcess   = NULL;
    HANDLE  m_hJob       = NULL;
    DWORD   m_dwProcessId = 0;
    int     m_nPort      = 15789;
    bool    m_bStarting  = false;
};
