#include "entityhandler.h"

class TestEntityHandler;
class EntityHandlerImpl : public EntityHandler {
    Q_OBJECT

 public:
    explicit EntityHandlerImpl(const QString &entityType, const QString &baseUrl, QObject *parent = nullptr)
        : EntityHandler(entityType, baseUrl, parent) {}

    // EntityHandler interface
 public:
    WebhookRequest *prepareRequest(const QString &entityId, EntityInterface *entity, int command,
                                   const QVariantMap &placeholders, const QVariant &param) override {
        Q_UNUSED(entityId)
        Q_UNUSED(entity)
        Q_UNUSED(command)
        Q_UNUSED(placeholders)
        Q_UNUSED(param)
        return Q_NULLPTR;
    }

    void onReply(int command, EntityInterface *entity, const QVariant &param, const WebhookRequest *request,
                 QNetworkReply *reply) override {
        Q_UNUSED(command)
        Q_UNUSED(entity)
        Q_UNUSED(param)
        Q_UNUSED(request)
        Q_UNUSED(reply)
    }

 private:
    friend TestEntityHandler;
};
