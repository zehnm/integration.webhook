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

#pragma once

#include <QList>
#include <QMap>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QObject>
#include <QSslError>
#include <QString>
#include <QTimer>
#include <QVariantMap>

#include "entityhandler.h"
#include "webhookentity.h"
#include "yio-interface/configinterface.h"
#include "yio-interface/entities/entitiesinterface.h"
#include "yio-interface/entities/entityinterface.h"
#include "yio-interface/notificationsinterface.h"
#include "yio-interface/plugininterface.h"
#include "yio-plugin/integration.h"
#include "yio-plugin/plugin.h"

const bool USE_WORKER_THREAD = false;

class WebhookPlugin : public Plugin {
    Q_OBJECT
    Q_INTERFACES(PluginInterface)
    Q_PLUGIN_METADATA(IID "YIO.PluginInterface" FILE "webhook.json")

 public:
    WebhookPlugin();

    // Plugin interface
 protected:
    Integration* createIntegration(const QVariantMap& config, EntitiesInterface* entities,
                                   NotificationsInterface* notifications, YioAPIInterface* api,
                                   ConfigInterface* configObj) override;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Webhook : public Integration {
    Q_OBJECT

 public:
    Webhook(const QVariantMap& config, EntitiesInterface* entities, NotificationsInterface* notifications,
            YioAPIInterface* api, ConfigInterface* configObj, Plugin* plugin);

    // IntegrationInterface interface
 public:
    void sendCommand(const QString& type, const QString& entityId, int command, const QVariant& param) override;

 public slots:  // NOLINT open issue: https://github.com/cpplint/cpplint/pull/99
    void connect() override;
    void disconnect() override;
    void enterStandby() override;
    void leaveStandby() override;

 private:
    void           addAvailableEntities(const QList<WebhookEntity*>& entities);
    void           configureProxy(const QVariantMap& proxyCfg);
    QNetworkReply* sendWebhookRequest(WebhookRequest* request);

 private slots:  // NOLINT open issue: https://github.com/cpplint/cpplint/pull/99
    void ignoreSslErrors(QNetworkReply* reply, const QList<QSslError>& errors);
    void statusUpdate();

 private:
    QNetworkAccessManager         m_networkManager;
    QMap<QString, EntityHandler*> m_handlers;
    QVariantMap                   m_placeholders;
    QTimer*                       m_statusTimer;
};
