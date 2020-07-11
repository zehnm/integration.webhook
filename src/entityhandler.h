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

#include <QJsonDocument>
#include <QList>
#include <QLoggingCategory>
#include <QMap>
#include <QMapIterator>
#include <QMetaEnum>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "webhookentity.h"
#include "webhookrequest.h"
#include "yio-interface/entities/entitiesinterface.h"
#include "yio-interface/entities/entityinterface.h"

class EntityHandler : public QObject {
    Q_OBJECT

 public:
    explicit EntityHandler(const QString& entityType, const QString& baseUrl, QObject* parent = nullptr);

    QString entityType() { return m_entityType; }

    /**
     * @brief Reads all webhook entity definitions from the configuration structure.
     * All valid webhook entities can be retrieved afterwards with getEntities().
     * @param entityCfgList Webhook configuration structure.
     * @param headers Global default http request headers applicable for all command requests.
     * @return Number of created entities
     */
    int readEntities(const QVariantList& entityCfgList, const QVariantMap& headers);

    QList<WebhookEntity*> getEntities() const { return m_webhookEntities.values(); }

    /**
     * @brief Returns a Java-style const iterator of the created webhook entities.
     */
    QMapIterator<QString, WebhookEntity*> entityIter() const;

    /**
     * @brief Optional enitity initialization, called after the created entities have been added to the app.
     * @details This allows for example to set default values stored in the configuration which cannot be retrieved by a
     * status update.
     */
    virtual void initialize(EntitiesInterface* entities);

    /**
     * @brief Returns true if the given entity supports status requests.
     */
    bool hasStatusCommand(const QString& entityId) const;

    /**
     * @brief Creates an internal status update request for the given entity identifier.
     * @return A newly created WebhookRequest object which must be deleted by the caller, or null if the entity does not
     * support status requests.
     */
    WebhookRequest* createStatusRequest(const QString& entityId, const QVariantMap& placeholders) const;

    /**
     * @brief Handles the internal QNetworkReply from the given status request.
     * @param entity The entity interface for retrieving and setting command specific information.
     * @param request The corresponding wehook status request.
     * @param reply The received reply from the webhook status request.
     */
    virtual void statusReply(EntityInterface* entity, const WebhookRequest* request, QNetworkReply* reply);

    /**
     * @brief Creates a webhook request for the given entity command.
     * @param entityId Entity identifier.
     * @param entity The entity interface for retrieving and setting command specific information.
     * @param command Entity specific command sent from the app. See BlindDef, LightDef, SwitchDef, etc. enums
     * @param placeholders Webhook specific variable placeholders.
     * @param param Command specific parameter, provided from the app.
     * @return A newly created WebhookRequest object which must be deleted by the caller, or null if the request could
     * not be created.
     */
    virtual WebhookRequest* createCommandRequest(const QString& entityId, EntityInterface* entity, int command,
                                                 const QVariantMap& placeholders, const QVariant& param) const = 0;

    /**
     * @brief Handles the QNetworkReply from the given webhook request.
     * @param command Original entity specific command sent from the app. See BlindDef, LightDef, SwitchDef, etc. enums
     * @param entity The entity interface for retrieving and setting command specific information.
     * @param param Original command specific parameter, provided from the app.
     * @param request The corresponding wehook command request.
     * @param reply The received reply from the webhook command request.
     */
    virtual void commandReply(int command, EntityInterface* entity, const QVariant& param,
                              const WebhookRequest* request, QNetworkReply* reply) = 0;

 protected:
    virtual const QLoggingCategory& logCategory() const = 0;

    /**
     * @brief Optional hook when a WebhookEntity was read from the configuration to further customize it.
     * @return true to include the entity, false to filter it out
     */
    virtual bool onWebhookEntityRead(const QVariantMap& entityCfgMap, WebhookEntity* entity) {
        Q_UNUSED(entityCfgMap)
        Q_UNUSED(entity)
        return true;
    }

    QUrl    buildUrl(const QVariant& commandUrl, const QVariantMap& placeholders) const;
    int     convertBrightnessToPercentage(float value) const;
    QString resolveVariables(const QString& text, const QVariantMap& placeholders) const;

    WebhookRequest* createRequest(const QString& commandName, const QString& entityId,
                                  const QVariantMap& placeholders) const;

    virtual void handleResponseData(EntityInterface* entity, const WebhookRequest* request, QNetworkReply* reply);

    int retrieveResponseValues(QNetworkReply* reply, const QMap<QString, QString>& mappings, QVariantMap* values);

    int retrieveResponseValues(const QJsonDocument& jsonDoc, const QMap<QString, QString>& mappings,
                               QVariantMap* values);

    virtual void updateEntity(EntityInterface* entity, const QVariantMap& placeholders) = 0;

    template <class EnumClass>
    EnumClass stringToEnum(const QString& enumString, const EnumClass& defaultValue) const {
        const auto metaEnum = QMetaEnum::fromType<EnumClass>();
        bool       ok;
        const auto value = metaEnum.keyToValue(enumString.toLatin1(), &ok);
        return ok ? static_cast<EnumClass>(value) : defaultValue;
    }

 protected:
    /**
     * @brief Internal status command identifier
     */
    static const QString STATUS_COMMAND;

    QString m_entityType;
    QString m_baseUrl;

    QMap<QString, WebhookEntity*> m_webhookEntities;
};
