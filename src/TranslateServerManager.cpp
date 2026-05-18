#include "TranslateServerManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QByteArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <windows.h>
#include <winhttp.h>
#include <tlhelp32.h>
#include <shlwapi.h>


TranslateServerManager::TranslateServerManager()
{
    m_hJob = CreateJobObject(NULL, NULL);
    if (m_hJob) {
        JOBOBJECT_EXTENDED_LIMIT_INFORMATION jeli = {};
        jeli.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        SetInformationJobObject(m_hJob, JobObjectExtendedLimitInformation,
                                &jeli, sizeof(jeli));
    }
}

TranslateServerManager::~TranslateServerManager()
{
    StopServer();
    if (m_hJob) {
        CloseHandle(m_hJob);
        m_hJob = NULL;
    }
}

bool TranslateServerManager::StartServer()
{
    if (IsServerRunning()) return true;
    if (m_bStarting) return false;

    m_bStarting = true;
    KillExistingServers();

    QString exePath = GetServerExePath();
    if (exePath.isEmpty() || !PathFileExistsW(exePath.toStdWString().c_str())) {
        qWarning() << "[Translate] translate_server.exe not found:" << exePath;
        m_bStarting = false;
        return false;
    }

    STARTUPINFOW si = {};
    si.cb         = sizeof(si);
    si.dwFlags    = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    PROCESS_INFORMATION pi = {};

    // Working directory: folder containing translate_server.exe
    QString workDir = QDir::toNativeSeparators(QFileInfo(exePath).absolutePath());

    std::wstring cmdLine = L"\"" + exePath.toStdWString() + L"\"";
    std::vector<wchar_t> cmdBuf(cmdLine.begin(), cmdLine.end());
    cmdBuf.push_back(0);

    BOOL bOk = CreateProcessW(NULL, cmdBuf.data(), NULL, NULL, FALSE,
                               CREATE_NO_WINDOW, NULL,
                               workDir.toStdWString().c_str(), &si, &pi);
    if (bOk) {
        m_hProcess    = pi.hProcess;
        m_dwProcessId = pi.dwProcessId;
        CloseHandle(pi.hThread);

        if (m_hJob) AssignProcessToJobObject(m_hJob, m_hProcess);

        if (WaitForServerReady(120000)) {
            m_bStarting = false;
            return true;
        } else {
            qWarning() << "[Translate] Server start timeout";
            StopServer();
        }
    } else {
        qWarning() << "[Translate] CreateProcess failed, error:" << GetLastError();
    }

    m_bStarting = false;
    return false;
}

void TranslateServerManager::StopServer()
{
    if (m_hProcess) {
        TerminateProcess(m_hProcess, 0);
        WaitForSingleObject(m_hProcess, 5000);
        CloseHandle(m_hProcess);
        m_hProcess    = NULL;
        m_dwProcessId = 0;
    }
}

QString TranslateServerManager::Translate(const QString &text,
                                           const QString &fromLang,
                                           const QString &toLang)
{
    if (text.isEmpty()) return {};
    if (!IsServerRunning()) {
        if (!StartServer()) {
            qWarning() << "[Translate] Server is not running";
            return {};
        }
    }
    return SendTranslateRequest(text, fromLang, toLang);
}

bool TranslateServerManager::IsServerRunning()
{
    if (!m_hProcess) return false;
    DWORD exitCode = 0;
    if (GetExitCodeProcess(m_hProcess, &exitCode))
        return (exitCode == STILL_ACTIVE);
    return false;
}

QString TranslateServerManager::GetServerExePath()
{
    QString exeDir = QCoreApplication::applicationDirPath();
    // Normalize Windows path separators
    exeDir = QDir::toNativeSeparators(exeDir);
    QString path = exeDir + "\\translate_server.exe";
    if (QFileInfo::exists(path)) return path;
    // Fallback: search relative to source tree
    QString altPath = QDir::toNativeSeparators(exeDir + "/../translate_server.exe");
    if (QFileInfo::exists(altPath)) return altPath;
    return {};
}

bool TranslateServerManager::WaitForServerReady(DWORD timeoutMs)
{
    DWORD elapsed = 0;
    const DWORD interval = 500;
    while (elapsed < timeoutMs) {
        if (CheckServerHealth()) return true;
        Sleep(interval);
        elapsed += interval;
        if (!IsServerRunning()) return false;
    }
    return false;
}

bool TranslateServerManager::CheckServerHealth()
{
    HINTERNET hSess = WinHttpOpen(L"CompactVLMQt/1.0",
                                   WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                   WINHTTP_NO_PROXY_NAME,
                                   WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET hConn = hSess ? WinHttpConnect(hSess, L"127.0.0.1", m_nPort, 0) : nullptr;
    HINTERNET hReq  = hConn ? WinHttpOpenRequest(hConn, L"GET", L"/health",
                                                  NULL, WINHTTP_NO_REFERER,
                                                  WINHTTP_DEFAULT_ACCEPT_TYPES, 0) : nullptr;
    bool ok = false;
    if (hReq) {
        DWORD timeout = 1000;
        WinHttpSetOption(hReq, WINHTTP_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
        WinHttpSetOption(hReq, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

        if (WinHttpSendRequest(hReq, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                               WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
            WinHttpReceiveResponse(hReq, NULL))
        {
            DWORD statusCode = 0, size = sizeof(statusCode);
            WinHttpQueryHeaders(hReq,
                WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                NULL, &statusCode, &size, NULL);
            ok = (statusCode == 200);
        }
    }
    if (hReq)  WinHttpCloseHandle(hReq);
    if (hConn) WinHttpCloseHandle(hConn);
    if (hSess) WinHttpCloseHandle(hSess);
    return ok;
}

QString TranslateServerManager::SendTranslateRequest(const QString &text,
                                                      const QString &fromLang,
                                                      const QString &toLang)
{
    // JSON escape processing (using Qt)
    QString escaped = text;
    escaped.replace("\\", "\\\\");
    escaped.replace("\"", "\\\"");
    escaped.replace("\n", "\\n");
    escaped.replace("\r", "\\r");
    escaped.replace("\t", "\\t");

    QByteArray jsonBody = (QString("{\"text\":\"%1\",\"from\":\"%2\",\"to\":\"%3\"}")
                           .arg(escaped, fromLang, toLang)).toUtf8();

    HINTERNET hSess = WinHttpOpen(L"CompactVLMQt/1.0",
                                   WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                   WINHTTP_NO_PROXY_NAME,
                                   WINHTTP_NO_PROXY_BYPASS, 0);
    HINTERNET hConn = hSess ? WinHttpConnect(hSess, L"127.0.0.1", m_nPort, 0) : nullptr;
    HINTERNET hReq  = hConn ? WinHttpOpenRequest(hConn, L"POST", L"/translate",
                                                  NULL, WINHTTP_NO_REFERER,
                                                  WINHTTP_DEFAULT_ACCEPT_TYPES, 0) : nullptr;

    QString result;
    if (hReq) {
        DWORD timeout = 30000;
        WinHttpSetOption(hReq, WINHTTP_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));
        WinHttpSetOption(hReq, WINHTTP_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));

        LPCWSTR headers = L"Content-Type: application/json; charset=utf-8\r\n";
        if (WinHttpSendRequest(hReq, headers, (DWORD)-1,
                               (LPVOID)jsonBody.constData(), jsonBody.size(),
                               jsonBody.size(), 0) &&
            WinHttpReceiveResponse(hReq, NULL))
        {
            QByteArray responseData;
            DWORD dwSize = 0;
            do {
                dwSize = 0;
                if (!WinHttpQueryDataAvailable(hReq, &dwSize) || dwSize == 0) break;
                QByteArray chunk(static_cast<int>(dwSize) + 1, '\0');
                DWORD dwRead = 0;
                WinHttpReadData(hReq, chunk.data(), dwSize, &dwRead);
                chunk.resize(static_cast<int>(dwRead));
                responseData += chunk;
            } while (dwSize > 0);

            // {"result":"..."} — use Qt JSON to handle escaped quotes correctly
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            if (!doc.isNull() && doc.isObject()) {
                result = doc.object().value("result").toString();
            }
        }
    }

    if (hReq)  WinHttpCloseHandle(hReq);
    if (hConn) WinHttpCloseHandle(hConn);
    if (hSess) WinHttpCloseHandle(hSess);
    return result;
}

void TranslateServerManager::KillExistingServers()
{
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnap == INVALID_HANDLE_VALUE) return;

    PROCESSENTRY32W pe = {};
    pe.dwSize = sizeof(pe);
    if (Process32FirstW(hSnap, &pe)) {
        do {
            if (_wcsicmp(pe.szExeFile, L"translate_server.exe") == 0 &&
                pe.th32ProcessID != m_dwProcessId)
            {
                HANDLE h = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (h) {
                    TerminateProcess(h, 0);
                    CloseHandle(h);
                }
            }
        } while (Process32NextW(hSnap, &pe));
    }
    CloseHandle(hSnap);
}
