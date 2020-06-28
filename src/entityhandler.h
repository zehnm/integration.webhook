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
#include <QMap>
#include <QMetaEnum>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include "webhookentity.h"
#include "webhookrequest.h"
#include "yio-interface/entities/entityinterface.h"

class EntityHandler : public QObject {
    Q_OBJECT

 public:
    explicit EntityHandler(const QString& entityType, const QString& baseUrl, QObject* parent = nullptr);

    QString entityType() { return m_entityType; }

    int readEntities(const QVariantList& entityCfgList, const QVariantMap& headers);

    QList<WebhookEntity*> getEntities() const { return m_webhookEntities.values(); }

    virtual WebhookRequest* prepareRequest(const QString& entityId, EntityInterface* entity, int command,
                                           const QVariantMap& placeholders, const QVariant& param) = 0;

    virtual void onReply(int command, EntityInterface* entity, const QVariant& param, const WebhookRequest* request,
                         QNetworkReply* reply) = 0;

 protected:
    QUrl    buildUrl(const QVariant& commandUrl, const QVariantMap& placeholders) const;
    int     convertBrightnessToPercentage(float value) const;
    QString resolveVariables(const QString& text, const QVariantMap& placeholders) const;

    WebhookRequest* createRequest(const QString& commandName, const QString& entityId,
                                  const QVariantMap& placeholders) const;

    int retrieveResponseValues(QNetworkReply* reply, const QMap<QString, QString>& mappings, QVariantMap* values);

    int retrieveResponseValues(const QJsonDocument& jsonDoc, const QMap<QString, QString>& mappings,
                               QVariantMap* values);

    template <class EnumClass>
    EnumClass stringToEnum(const QString& enumString, const EnumClass& defaultValue) const {
        const auto metaEnum = QMetaEnum::fromType<EnumClass>();
        bool       ok;
        const auto value = metaEnum.keyToValue(enumString.toLatin1(), &ok);
        return ok ? static_cast<EnumClass>(value) : defaultValue;
    }

 protected:
    QString m_entityType;
    QString m_baseUrl;

    QMap<QString, WebhookEntity*> m_webhookEntities;
};
