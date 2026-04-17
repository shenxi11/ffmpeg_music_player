#include "AgentEmbeddedLlamaEngine.h"

#include "settings_manager.h"

#include <QByteArray>
#include <QFileInfo>
#include <QList>
#include <QTextStream>

#include <algorithm>
#include <memory>
#include <mutex>
#include <vector>

extern "C" {
#include "ggml-backend.h"
#include "llama.h"
}

namespace {

constexpr int kContextReserveTokens = 128;

using ContextPtr = std::unique_ptr<llama_context, decltype(&llama_free)>;
using SamplerPtr = std::unique_ptr<llama_sampler, decltype(&llama_sampler_free)>;

void initializeLlamaBackend()
{
    static std::once_flag s_backendInitFlag;
    std::call_once(s_backendInitFlag, []() {
        ggml_backend_load_all();
        llama_log_set(
            [](enum ggml_log_level level, const char* text, void*) {
                if (level >= GGML_LOG_LEVEL_ERROR && text) {
                    QTextStream(stderr) << text;
                }
            },
            nullptr);
    });
}

QString fallbackPrompt(const QString& systemPrompt, const QString& userPrompt)
{
    return QStringLiteral("System:\n%1\n\nUser:\n%2\n\nAssistant:\n")
        .arg(systemPrompt.trimmed(), userPrompt.trimmed());
}

QString buildPromptWithTemplate(llama_model* model,
                                const QString& systemPrompt,
                                const QString& userPrompt)
{
    QList<QByteArray> storage;
    storage << systemPrompt.toUtf8() << userPrompt.toUtf8();

    std::vector<llama_chat_message> messages;
    messages.push_back(llama_chat_message{"system", storage[0].constData()});
    messages.push_back(llama_chat_message{"user", storage[1].constData()});

    const char* tmpl = llama_model_chat_template(model, nullptr);
    if (!tmpl || QByteArray(tmpl).trimmed().isEmpty()) {
        return fallbackPrompt(systemPrompt, userPrompt);
    }

    int32_t len =
        llama_chat_apply_template(tmpl, messages.data(), messages.size(), true, nullptr, 0);
    if (len <= 0) {
        return fallbackPrompt(systemPrompt, userPrompt);
    }

    std::vector<char> buffer(static_cast<size_t>(len) + 8);
    len = llama_chat_apply_template(
        tmpl, messages.data(), messages.size(), true, buffer.data(), static_cast<int32_t>(buffer.size()));
    if (len <= 0) {
        return fallbackPrompt(systemPrompt, userPrompt);
    }

    return QString::fromUtf8(buffer.data(), len);
}

std::vector<llama_token> tokenizePrompt(const llama_vocab* vocab, const QString& prompt, QString* error)
{
    const QByteArray utf8 = prompt.toUtf8();
    const int tokenCount =
        -llama_tokenize(vocab, utf8.constData(), utf8.size(), nullptr, 0, true, true);
    if (tokenCount <= 0) {
        if (error) {
            *error = QStringLiteral("无法对当前提示词进行分词。");
        }
        return {};
    }

    std::vector<llama_token> tokens(static_cast<size_t>(tokenCount));
    if (llama_tokenize(vocab,
                       utf8.constData(),
                       utf8.size(),
                       tokens.data(),
                       static_cast<int32_t>(tokens.size()),
                       true,
                       true) < 0) {
        if (error) {
            *error = QStringLiteral("本地模型分词失败。");
        }
        return {};
    }

    return tokens;
}

SamplerPtr createSampler(const AgentEmbeddedLlamaEngine::Request& request)
{
    auto params = llama_sampler_chain_default_params();
    SamplerPtr sampler(llama_sampler_chain_init(params), llama_sampler_free);
    if (!sampler) {
        return sampler;
    }

    if (request.deterministic) {
        llama_sampler_chain_add(sampler.get(), llama_sampler_init_greedy());
        return sampler;
    }

    llama_sampler_chain_add(sampler.get(), llama_sampler_init_min_p(0.05f, 1));
    llama_sampler_chain_add(sampler.get(), llama_sampler_init_temp(std::max(0.1f, request.temperature)));
    llama_sampler_chain_add(sampler.get(), llama_sampler_init_dist(LLAMA_DEFAULT_SEED));
    return sampler;
}

QString tokenToPiece(const llama_vocab* vocab, llama_token token, QString* error)
{
    char buffer[256] = {};
    const int pieceLen = llama_token_to_piece(vocab, token, buffer, sizeof(buffer), 0, true);
    if (pieceLen < 0) {
        if (error) {
            *error = QStringLiteral("本地模型无法将 token 转回文本。");
        }
        return {};
    }
    return QString::fromUtf8(buffer, pieceLen);
}

} // namespace

AgentEmbeddedLlamaEngine& AgentEmbeddedLlamaEngine::instance()
{
    static AgentEmbeddedLlamaEngine s_instance;
    return s_instance;
}

AgentEmbeddedLlamaEngine::~AgentEmbeddedLlamaEngine()
{
    if (m_model) {
        llama_model_free(m_model);
        m_model = nullptr;
    }
}

AgentEmbeddedLlamaEngine::Result AgentEmbeddedLlamaEngine::generate(const Request& request)
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return generateLocked(request);
}

AgentEmbeddedLlamaEngine::Result AgentEmbeddedLlamaEngine::ensureModelLoadedLocked()
{
    initializeLlamaBackend();

    const QString modelPath =
        QFileInfo(SettingsManager::instance().agentLocalModelPath()).absoluteFilePath();
    if (modelPath.trimmed().isEmpty()) {
        return {false,
                QStringLiteral("model_unavailable"),
                QStringLiteral("本地模型文件路径为空，请先在设置页配置模型文件路径。"),
                {}};
    }
    if (!QFileInfo::exists(modelPath)) {
        return {false,
                QStringLiteral("model_unavailable"),
                QStringLiteral("未找到本地模型文件：%1").arg(modelPath),
                {}};
    }

    if (m_model && m_loadedModelPath == modelPath) {
        return {true, {}, {}, {}};
    }

    if (m_model) {
        llama_model_free(m_model);
        m_model = nullptr;
        m_loadedModelPath.clear();
    }

    llama_model_params modelParams = llama_model_default_params();
    modelParams.n_gpu_layers = 0;
    modelParams.use_mmap = true;
    modelParams.use_mlock = false;

    m_model = llama_model_load_from_file(modelPath.toUtf8().constData(), modelParams);
    if (!m_model) {
        return {false,
                QStringLiteral("model_unavailable"),
                QStringLiteral("无法加载本地模型文件：%1").arg(modelPath),
                {}};
    }

    m_loadedModelPath = modelPath;
    return {true, {}, {}, {}};
}

AgentEmbeddedLlamaEngine::Result AgentEmbeddedLlamaEngine::generateLocked(const Request& request)
{
    Result modelReady = ensureModelLoadedLocked();
    if (!modelReady.ok) {
        return modelReady;
    }

    const QString prompt = buildPromptWithTemplate(m_model, request.systemPrompt, request.userPrompt);
    const llama_vocab* vocab = llama_model_get_vocab(m_model);
    QString tokenError;
    std::vector<llama_token> promptTokens = tokenizePrompt(vocab, prompt, &tokenError);
    if (promptTokens.empty()) {
        return {false,
                QStringLiteral("format_error"),
                tokenError.isEmpty() ? QStringLiteral("本地模型提示词分词失败。") : tokenError,
                {}};
    }

    int contextSize = SettingsManager::instance().agentLocalContextSize();
    if (contextSize <= 0) {
        contextSize = 16384;
    }
    const int requiredTokens =
        static_cast<int>(promptTokens.size()) + std::max(1, request.maxTokens) + kContextReserveTokens;
    if (requiredTokens > contextSize) {
        return {false,
                QStringLiteral("context_overflow"),
                QStringLiteral("本地模型上下文不足，当前提示词需要约 %1 token，上下文仅配置为 %2。")
                    .arg(requiredTokens)
                    .arg(contextSize),
                {}};
    }

    llama_context_params contextParams = llama_context_default_params();
    contextParams.n_ctx = static_cast<uint32_t>(contextSize);
    contextParams.n_batch = static_cast<uint32_t>(std::min(contextSize, std::max(32, static_cast<int>(promptTokens.size()))));
    contextParams.n_threads = SettingsManager::instance().agentLocalThreadCount();
    contextParams.n_threads_batch = contextParams.n_threads;
    contextParams.no_perf = true;

    ContextPtr context(llama_init_from_model(m_model, contextParams), llama_free);
    if (!context) {
        return {false,
                QStringLiteral("model_unavailable"),
                QStringLiteral("本地模型上下文初始化失败。"),
                {}};
    }

    SamplerPtr sampler = createSampler(request);
    if (!sampler) {
        return {false,
                QStringLiteral("model_unavailable"),
                QStringLiteral("本地模型采样器初始化失败。"),
                {}};
    }

    llama_batch batch =
        llama_batch_get_one(promptTokens.data(), static_cast<int32_t>(promptTokens.size()));
    if (llama_decode(context.get(), batch) != 0) {
        return {false,
                QStringLiteral("model_unavailable"),
                QStringLiteral("本地模型在处理提示词时失败。"),
                {}};
    }

    QString output;
    output.reserve(request.maxTokens * 2);

    for (int generated = 0; generated < request.maxTokens; ++generated) {
        const llama_token token = llama_sampler_sample(sampler.get(), context.get(), -1);
        if (llama_vocab_is_eog(vocab, token)) {
            break;
        }

        QString pieceError;
        const QString piece = tokenToPiece(vocab, token, &pieceError);
        if (!pieceError.isEmpty()) {
            return {false, QStringLiteral("format_error"), pieceError, {}};
        }
        output += piece;

        llama_batch nextBatch = llama_batch_get_one(const_cast<llama_token*>(&token), 1);
        if (llama_decode(context.get(), nextBatch) != 0) {
            return {false,
                    QStringLiteral("model_unavailable"),
                    QStringLiteral("本地模型在生成响应时失败。"),
                    {}};
        }
    }

    return {true, {}, {}, output.trimmed()};
}
