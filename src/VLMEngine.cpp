#include "VLMEngine.h"

#include <QBuffer>
#include <QByteArray>
#include <QDebug>
#include <windows.h>
#include <shlobj.h>

VLMEngine::VLMEngine(const std::string &ollamaUrl, const std::string &modelVision)
    : m_ollamaUrl(ollamaUrl)
    , m_modelVision(modelVision)
{
}

VLMEngine::~VLMEngine()
{
    if (m_pTranslateServer) {
        m_pTranslateServer->StopServer();
        m_pTranslateServer.reset();
    }
}

bool VLMEngine::Initialize()
{
    try {
        if (!m_pOllama) {
            m_pOllama = std::make_unique<Ollama>(m_ollamaUrl);
            m_pOllama->setReadTimeout(180);
            m_pOllama->setWriteTimeout(180);
        }

        if (!m_pOllama->is_running()) {
            if (!StartOllamaServer()) return false;
        }

        if (!m_pTranslateServer) {
            m_pTranslateServer = std::make_unique<TranslateServerManager>();
            if (!m_pTranslateServer->StartServer()) {
                qWarning() << "[VLM] Translation server not available (translate_server.exe missing?)";
                // Translation server failure does not affect Ollama initialization
            }
        }
        return true;  // Return true if Ollama is OK
    } catch (const std::exception &e) {
        qWarning() << "[VLM] Initialize exception:" << e.what();
        return false;
    } catch (...) {
        qWarning() << "[VLM] Initialize unknown exception";
        return false;
    }
}

bool VLMEngine::StartOllamaServer()
{
    // %LOCALAPPDATA%\Programs\Ollama\ollama.exe serve
    wchar_t localPath[MAX_PATH] = {};
    SHGetSpecialFolderPathW(NULL, localPath, CSIDL_LOCAL_APPDATA, FALSE);
    QString ollamaExe = QString::fromWCharArray(localPath)
                        + "/Programs/Ollama/ollama.exe";

    std::wstring cmdLine = L"\"" + ollamaExe.toStdWString() + L"\" serve";
    std::vector<wchar_t> buf(cmdLine.begin(), cmdLine.end());
    buf.push_back(0);

    STARTUPINFOW si = {};
    si.cb          = sizeof(si);
    si.dwFlags     = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi = {};

    if (CreateProcessW(NULL, buf.data(), NULL, NULL, FALSE,
                       CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);

        for (int i = 0; i < 20; ++i) {
            Sleep(500);
            if (m_pOllama->is_running()) {
                return true;
            }
        }
    }
    qWarning() << "[VLM] Failed to start Ollama";
    return false;
}

VLMResult VLMEngine::SendChat(const QString &text, const QImage &image)
{
    VLMResult result;
    if (!m_pOllama) {
        result.errorMessage = "Ollama not initialized";
        return result;
    }

    try {
        qDebug() << "[1] Korean input:" << text;

        // [1→2] Korean → English translation
        QString translatedPrompt = TranslateText(text, true);
        qDebug() << "[2] English translation:" << translatedPrompt;

        // New image: reset conversation history so previous image descriptions
        // do not contaminate the new image context.
        if (!image.isNull()) {
            m_sessionImage = image;
            m_messageHistory.clear();
        }

        // Encode session image as Base64 and re-attach to every message so
        // follow-up questions can reference the image without re-sending it.
        std::vector<ollama::image> images;
        if (!m_sessionImage.isNull()) {
            std::string b64 = imageToBase64(m_sessionImage);
            images.push_back(ollama::image::from_base64_string(b64));
        }

        // Build history: strip image blobs from prior messages (text only),
        // current message always carries the session image.
        ollama::messages historyToSend;
        for (const auto &msg : m_messageHistory) {
            historyToSend.push_back(ollama::message(
                msg["role"].get<std::string>(),
                msg["content"].get<std::string>()
            ));
        }
        ollama::message userMsg("user", translatedPrompt.toStdString(), images);
        historyToSend.push_back(userMsg);
        m_messageHistory.push_back(userMsg);

        ollama::response resp = m_pOllama->chat(
            m_modelVision, historyToSend,
            nullptr, std::string(""), std::string("0"));

        if (resp.is_valid() && !resp.has_error()) {
            std::string respText = resp.as_simple_string();
            qDebug() << "[3] VLM English response:" << QString::fromStdString(respText);

            ollama::message assistantMsg("assistant", respText);
            m_messageHistory.push_back(assistantMsg);

            // [3→4] English → Korean translation
            QString translated = TranslateText(QString::fromStdString(respText), false);
            qDebug() << "[4] Korean translation:" << translated;

            result.success      = true;
            result.responseText = translated;
        } else {
            result.errorMessage = QString::fromStdString(resp.get_error());
        }
    } catch (const std::exception &e) {
        result.errorMessage = e.what();
    }

    return result;
}

void VLMEngine::ClearHistory()
{
    m_messageHistory.clear();
    m_sessionImage = QImage();
}

bool VLMEngine::IsOllamaRunning() const
{
    if (m_pOllama) {
        try { return m_pOllama->is_running(); } catch (...) {}
    }
    return false;
}

bool VLMEngine::IsTranslateRunning() const
{
    if (m_translatorMode == TranslatorMode::Ollama)
        return IsOllamaRunning();
    return m_pTranslateServer && m_pTranslateServer->IsServerRunning();
}

QString VLMEngine::TranslateText(const QString &text, bool toEnglish)
{
    if (text.isEmpty()) return {};

    if (m_translatorMode == TranslatorMode::Ollama)
        return OllamaTranslate(text, toEnglish);

    // Argos mode
    if (!m_pTranslateServer) return {};
    QString from = toEnglish ? "ko" : "en";
    QString to   = toEnglish ? "en" : "ko";
    QString r    = m_pTranslateServer->Translate(text, from, to);
    return r.isEmpty() ? "Failed to translate" : r;
}

QString VLMEngine::OllamaTranslate(const QString &text, bool toEnglish)
{
    if (!m_pOllama) return {};
    try {
        const std::string sysContent = toEnglish
            ? "You are a professional translator. "
              "Translate the given Korean text into natural English. "
              "Output ONLY the translated text. "
              "Do NOT explain, comment, or provide alternatives."
            : "You are a professional translator. "
              "Translate the given English text into natural Korean. "
              "Output ONLY the translated text. "
              "Do NOT explain, comment, or provide alternatives.";

        ollama::messages msgs;
        msgs.push_back(ollama::message("system", sysContent));
        msgs.push_back(ollama::message("user",   text.toStdString()));

        ollama::response resp = m_pOllama->chat(
            m_translateModel, msgs,
            nullptr, std::string(""), std::string("0"));

        if (resp.is_valid() && !resp.has_error())
            return QString::fromStdString(resp.as_simple_string()).trimmed();

    } catch (const std::exception &e) {
        qWarning() << "[Translate] Ollama translate error:" << e.what();
    }
    return {};
}

void VLMEngine::SetTranslatorMode(TranslatorMode mode)
{
    if (m_translatorMode == mode) return;
    m_translatorMode = mode;

    if (mode == TranslatorMode::Ollama) {
        // Stop argos server to free resources (Python process + RAM)
        if (m_pTranslateServer) {
            m_pTranslateServer->StopServer();
        }
    } else {
        // Start argos server
        if (!m_pTranslateServer)
            m_pTranslateServer = std::make_unique<TranslateServerManager>();
        m_pTranslateServer->StartServer();
    }
}

// ────────────────────────────────────────────────────────────────
// QImage → PNG → Base64 (replaces cv::imencode)
// ────────────────────────────────────────────────────────────────
std::string VLMEngine::imageToBase64(const QImage &image)
{
    QByteArray ba;
    QBuffer buf(&ba);
    buf.open(QIODevice::WriteOnly);
    image.save(&buf, "PNG");
    return ba.toBase64().toStdString();
}
