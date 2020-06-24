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

#include "entityhandler.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QLoggingCategory>

// TODO(zehnm) set logging category from concrete class
static Q_LOGGING_CATEGORY(CLASS_LC, "yio.intg.webhook");

EntityHandler::EntityHandler(const QString &entityType, const QString &baseUrl, QObject *parent)
    : QObject(parent), m_entityType(entityType), m_baseUrl(baseUrl) {}

int EntityHandler::readEntities(const QVariantList &entityCfgList, const QVariantMap &headers) {
    int count = 0;
    for (QVariant entityCfg : entityCfgList) {
        QVariantMap   entityCfgMap = entityCfg.toMap();
        WebhookEntity entity = {entityCfgMap.value("entity_id").toString(), entityType(),
                                entityCfgMap.value("friendly_name").toString()};

        for (QVariant commandCfg : entityCfgMap.value("commands").toList()) {
            QVariantMap cmdMap = commandCfg.toMap();
            QString     feature = cmdMap.cbegin().key();
            QVariant    featureCfg = cmdMap.cbegin().value();
            if (feature == "STATUS") {
                // TODO(zehnm) handle STATUS
            } else {
                entity.supportedFeatures.append(feature);
            }

            WebhookCommand command;
            command.headers = headers;
            if (featureCfg.type() == QVariant::String) {
                // simple GET command without any options
                command.url = featureCfg.toString();
                command.method = HttpMethod::GET;
            } else {
                // the full package
                QVariantMap attrMap = featureCfg.toMap();

                command.url = attrMap.value("url").toString();
                command.method = stringToEnum<HttpMethod::Enum>(attrMap.value("method").toString(), HttpMethod::GET);

                if (command.method != HttpMethod::GET) {
                    command.body = attrMap.value("body");
                    QVariantMap cmdHeaders = attrMap.value("headers").toMap();
                    // Attention: QMap.unite inserts the same key multiple times instead of replacing it!
                    for (QVariantMap::const_iterator iter = cmdHeaders.cbegin(); iter != cmdHeaders.cend(); ++iter) {
                        command.headers.insert(iter.key(), iter.value());
                    }
                }
            }
            entity.commands.insert(feature, command);
        }

        count++;
        m_webhookEntities.insert(entity.id, entity);
    }

    return count;
}

int EntityHandler::convertBrightnessToPercentage(float value) const {
    // TODO(zehnm) implement scaling option
    return static_cast<int>(round(value / 255 * 100));
}

QString EntityHandler::replace(const QString &text, const QVariantMap &placeholders) const {
    QString newText = text;
    for (QVariantMap::const_iterator iter = placeholders.cbegin(); iter != placeholders.cend(); ++iter) {
        QString marker = QString("${%1}").arg(iter.key());
        if (newText.contains(marker)) {
            newText.replace(marker, iter.value().toString());
        }
    }

    return newText;
}

QUrl EntityHandler::buildUrl(const QVariant &commandUrl, const QVariantMap &placeholders) const {
    QString baseUrl = replace(m_baseUrl, placeholders);
    if (!commandUrl.isValid()) {
        return baseUrl;
    }

    QUrl url = replace(commandUrl.toString(), placeholders);
    if (url.isRelative()) {
        return QUrl(baseUrl).resolved(url);
    }
    return url;
}

WebhookRequest *EntityHandler::createRequest(const QString &commandName, const QString &entityId,
                                             const QVariantMap &placeholders) const {
    //    qCDebug(CLASS_LC) << "REST command:" << m_url << service << entityId << data;

    if (!m_webhookEntities.contains(entityId)) {
        qCWarning(CLASS_LC) << "Entity not found:" << entityId;
        return Q_NULLPTR;
    }

    WebhookEntity entity = m_webhookEntities.value(entityId);
    if (!entity.commands.contains(commandName)) {
        qCWarning(CLASS_LC) << "Command" << commandName << "not defined for entity:" << entityId;
        return Q_NULLPTR;
    }
    WebhookCommand command = entity.commands.value(commandName);

    WebhookRequest *request = new WebhookRequest();
    request->networkRequest.setUrl(buildUrl(command.url, placeholders));
    request->method = command.method;

    if (command.body.isValid()) {
        if (command.body.type() == QVariant::Map) {
            QJsonDocument jsonDoc = QJsonDocument::fromVariant(command.body);
            request->body.append(replace(jsonDoc.toJson(QJsonDocument::Compact), placeholders));
            request->networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        } else {
            request->body.append(replace(command.body.toString(), placeholders));
            request->networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/text");
        }
    }

    for (QString headerName : command.headers.keys()) {
        QString headerValue = replace(command.headers.value(headerName).toString(), placeholders);
        request->networkRequest.setRawHeader(headerName.toUtf8(), headerValue.toUtf8());
    }

    return request;
}
