#pragma once

#include "ollama.hpp"

#include <QImage>
#include <QString>
#include <string>
#include <vector>
#include <memory>

struct VLMResult
{
    bool    success      = false;
    QString responseText;
    QString errorMessage;
};

class VLMEngine
{
public:
    explicit VLMEngine(const std::string &ollamaUrl   = "http://localhost:11434",
                       const std::string &modelVision = "llama3.2-vision");
    ~VLMEngine() = default;

    bool      Initialize();
    VLMResult SendChat(const QString &text, const QImage &image = QImage());
    void      ClearHistory();

    bool        IsOllamaRunning() const;
    std::string GetModelName()    const { return m_modelVision; }

private:
    std::string imageToBase64(const QImage &image);
    QString     TranslateText(const QString &text, bool toEnglish);
    bool        StartOllamaServer();

    std::unique_ptr<Ollama>  m_pOllama;

    std::string      m_ollamaUrl;
    std::string      m_modelVision;
    std::string      m_translateModel = "qwen2.5:3b";
    ollama::messages m_messageHistory;
    QImage           m_sessionImage;
};
