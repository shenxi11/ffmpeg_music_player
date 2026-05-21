#ifndef CURL_NETWORK_TRANSPORT_H
#define CURL_NETWORK_TRANSPORT_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QString>

#include <functional>

/*
模块名: CurlNetworkTransport
功能概述: 提供基于 libcurl 的同步 HTTP 执行器，供统一网络层、下载、
          上传和资源缓存复用。
对外接口: CurlNetworkTransport::perform
依赖关系: libcurl、Qt Core
输入输出: 输入 HTTP 请求参数，输出状态码、响应头、响应体和错误信息。
异常与错误: libcurl 初始化、网络错误、超时和主动中断都会转为 Response。
维护说明: 本类不依赖 Qt Network，可在线程池中执行；回调需保持线程安全。
*/
class CurlNetworkTransport {
  public:
    enum class Error {
        NoError,
        InvalidRequest,
        CurlError,
        Timeout,
        Aborted,
        FileError
    };

    struct MultipartPart {
        QString name;
        QByteArray data;
        QString filePath;
        QString fileName;
        QString contentType;
        bool isFile = false;
    };

    struct Request {
        QString url;
        QString method = QStringLiteral("GET");
        QByteArray body;
        QMap<QString, QString> headers;
        QList<MultipartPart> multipartParts;
        int timeoutMs = 30000;
        bool followRedirects = true;
        int maxRedirects = 5;
        bool enableCompression = true;
        QString userAgent = QStringLiteral("FFmpeg-Music-Player/1.0");
        std::function<bool()> shouldAbort;
        std::function<void(int, const QMap<QString, QString>&)> onHeaders;
        std::function<bool(const QByteArray&)> onData;
        std::function<void(qint64, qint64)> onDownloadProgress;
        std::function<void(qint64, qint64)> onUploadProgress;
    };

    struct Response {
        int statusCode = 0;
        QByteArray body;
        QMap<QString, QString> headers;
        QByteArray rawHeaders;
        Error error = Error::NoError;
        QString errorString;
        bool aborted = false;
        bool timedOut = false;
        qint64 elapsedMs = 0;
    };

    static bool isAvailable();
    static Response perform(const Request& request);
};

#endif // CURL_NETWORK_TRANSPORT_H
