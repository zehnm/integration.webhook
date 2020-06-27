/******************************************************************************
 *
 * Copyright (C) 2020 Markus Zehnder <business@markuszehnder.ch>
 *
 * This file is part of the YIO-Remote software project.
 *
 * YIO-Remote software is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * YIO-Remote software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with YIO-Remote software. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *****************************************************************************/

#include "webhook.h"

#include <QNetworkProxy>
#include <QtDebug>

#include "lighthandler.h"
#include "switchhandler.h"

WebhookPlugin::WebhookPlugin() : Plugin("webhook", USE_WORKER_THREAD) {}

Integration *WebhookPlugin::createIntegration(const QVariantMap &config, EntitiesInterface *entities,
                                              NotificationsInterface *notifications, YioAPIInterface *api,
                                              ConfigInterface *configObj) {
    qCInfo(m_logCategory) << "Creating Webhook integration plugin" << PLUGIN_VERSION;

    return new Webhook(config, entities, notifications, api, configObj, this);
}

Webhook::Webhook(const QVariantMap &config, EntitiesInterface *entities, NotificationsInterface *notifications,
                 YioAPIInterface *api, ConfigInterface *configObj, Plugin *plugin)
    : Integration(config, entities, notifications, api, configObj, plugin) {
    if (!config.contains(Integration::OBJ_DATA)) {
        qCCritical(m_logCategory) << "Missing configuration key" << Integration::OBJ_DATA;
        return;
    }

    QVariantMap map = config.value(Integration::OBJ_DATA).toMap();
    QString     baseUrl = map.value("base_url").toString();
    bool        ignoreSsl = map.value(Integration::KEY_DATA_SSL_IGNORE, false).toBool();
    QVariantMap headers = map.value("headers").toMap();

    m_placeholders = map.value("placeholders").toMap();

    m_handlers.insert("light", new LightHandler(baseUrl, this));
    m_handlers.insert("switch", new SwitchHandler(baseUrl, this));

    m_networkManager.setRedirectPolicy(QNetworkRequest::NoLessSafeRedirectPolicy);
    if (ignoreSsl) {
        m_networkManager.setStrictTransportSecurityEnabled(false);
        QObject::connect(&m_networkManager, &QNetworkAccessManager::sslErrors, this, &Webhook::ignoreSslErrors);
    }

    if (map.contains("proxy")) {
        configureProxy(map.value("proxy").toMap());
    }

    qCDebug(m_logCategory) << "Created webhook for:" << baseUrl << "ignoreSSL:" << ignoreSsl;

    QVariantMap entitiesCfg = map.value("entities").toMap();
    for (QVariantMap::const_iterator iter = entitiesCfg.cbegin(); iter != entitiesCfg.cend(); ++iter) {
        EntityHandler *entityHandler = m_handlers.value(iter.key());
        if (entityHandler) {
            entityHandler->readEntities(iter.value().toList(), headers);
        } else {
            qCWarning(m_logCategory) << "TODO implement handler for" << iter.key();
        }
    }

    for (EntityHandler *entityHandler : m_handlers) {
        addAvailableEntities(entityHandler->getEntities());
    }
}

void Webhook::connect() {
    setState(CONNECTING);

    // TODO(zehnm) implement me, e.g. starting timers
    // ...
}

void Webhook::disconnect() {
    // TODO(zehnm) implement me, e.g. stopping timers
    // ...

    setState(DISCONNECTED);
}

void Webhook::enterStandby() {
    // TODO(zehnm) implement me, e.g. suspending timers
    // ...
}

void Webhook::leaveStandby() {
    // TODO(zehnm) implement me, e.g. restarting timers
    // ...
}

void Webhook::addAvailableEntities(const QList<WebhookEntity *> &entities) {
    for (WebhookEntity *e : entities) {
        addAvailableEntity(e->id, e->type, integrationId(), e->friendlyName, e->supportedFeatures);
    }
}

void Webhook::configureProxy(const QVariantMap &proxyCfg) {
    QNetworkProxy::ProxyType proxyType = QNetworkProxy::DefaultProxy;

    QString proxyTypeCfg = proxyCfg.value("type").toString();
    if (proxyTypeCfg == "HttpProxy") {
        proxyType = QNetworkProxy::HttpProxy;
    } else if (proxyTypeCfg == "Socks5Proxy") {
        proxyType = QNetworkProxy::Socks5Proxy;
    } else if (proxyTypeCfg == "NoProxy") {
        proxyType = QNetworkProxy::NoProxy;
    }

    QNetworkProxy networkProxy(proxyType, proxyCfg.value("hostName").toString(), proxyCfg.value("port").toUInt(),
                               proxyCfg.value("user").toString(), proxyCfg.value("password").toString());
    m_networkManager.setProxy(networkProxy);
}

void Webhook::sendCommand(const QString &type, const QString &entityId, int command, const QVariant &param) {
    EntityInterface *entity = m_entities->getEntityInterface(entityId);
    if (!entity) {
        qCWarning(m_logCategory) << "Entity not found:" << entityId;
        return;
    }

    EntityHandler *entityHandler = m_handlers.value(type);
    if (!entityHandler) {
        qCWarning(m_logCategory) << "No EntityHandler found for type:" << entity->type();
        return;
    }

    WebhookRequest *request = entityHandler->prepareRequest(entityId, entity, command, m_placeholders, param);
    if (!request) {
        return;
    }
    Q_ASSERT(request->webhookCommand);

    QNetworkReply *reply;
    switch (request->webhookCommand->method) {
        case HttpMethod::POST:
            reply = m_networkManager.post(request->networkRequest, request->body);
            break;
        case HttpMethod::PUT:
            reply = m_networkManager.put(request->networkRequest, request->body);
            break;
        case HttpMethod::DELETE:
            reply = m_networkManager.deleteResource(request->networkRequest);
            break;
        default:
            reply = m_networkManager.get(request->networkRequest);
    }

    QObject::connect(
        reply, &QNetworkReply::finished, this, [this, entityHandler, command, entity, param, request, reply] {
            request->deleteLater();
            reply->deleteLater();

            if (reply->error() == QNetworkReply::NoError) {
                qCDebug(m_logCategory) << "Request finished successfully:" << request->webhookCommand->method
                                       << reply->url().url();
            } else {
                qCWarning(m_logCategory) << "Request failed:" << request->webhookCommand->method << reply->url().url()
                                         << "/" << reply->error() << "/" << reply->errorString();
            }

            entityHandler->onReply(command, entity, param, request, reply);
        });
}

void Webhook::ignoreSslErrors(QNetworkReply *reply, const QList<QSslError> &errors) {
    qCDebug(m_logCategory) << "Ignoring SSL error:" << errors;
    reply->ignoreSslErrors();
}
