#ifndef AGENT_EMBEDDED_LLAMA_ENGINE_H
#define AGENT_EMBEDDED_LLAMA_ENGINE_H

#include <QString>

#include <mutex>

struct llama_model;

class AgentEmbeddedLlamaEngine
{
public:
    struct Request
    {
        QString systemPrompt;
        QString userPrompt;
        int maxTokens = 256;
        float temperature = 0.1f;
        bool deterministic = true;
    };

    struct Result
    {
        bool ok = false;
        QString code;
        QString message;
        QString text;
    };

    static AgentEmbeddedLlamaEngine& instance();

    Result generate(const Request& request);

private:
    AgentEmbeddedLlamaEngine() = default;
    ~AgentEmbeddedLlamaEngine();

    AgentEmbeddedLlamaEngine(const AgentEmbeddedLlamaEngine&) = delete;
    AgentEmbeddedLlamaEngine& operator=(const AgentEmbeddedLlamaEngine&) = delete;

    Result ensureModelLoadedLocked();
    Result generateLocked(const Request& request);

private:
    std::mutex m_mutex;
    QString m_loadedModelPath;
    llama_model* m_model = nullptr;
};

#endif // AGENT_EMBEDDED_LLAMA_ENGINE_H
