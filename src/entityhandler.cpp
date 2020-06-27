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

#include <math.h>

#include <QLoggingCategory>

#include "jsonpath.h"

// TODO(zehnm) set logging category from concrete class
static Q_LOGGING_CATEGORY(CLASS_LC, "yio.intg.webhook");

EntityHandler::EntityHandler(const QString &entityType, const QString &baseUrl, QObject *parent)
    : QObject(parent), m_entityType(entityType), m_baseUrl(baseUrl) {}

int EntityHandler::readEntities(const QVariantList &entityCfgList, const QVariantMap &headers) {
    int count = 0;
    for (QVariant entityCfg : entityCfgList) {
        QVariantMap    entityCfgMap = entityCfg.toMap();
        WebhookEntity *entity = new WebhookEntity(entityCfgMap.value("entity_id").toString(), entityType(),
                                                  entityCfgMap.value("friendly_name").toString(), this);

        QMapIterator<QString, QVariant> iter(entityCfgMap.value("commands").toMap());
        while (iter.hasNext()) {
            iter.next();
            QString  feature = iter.key();
            QVariant featureCfg = iter.value();
            if (feature == "STATUS") {
                // TODO(zehnm) handle STATUS
            } else {
                entity->supportedFeatures.append(feature);
            }

            WebhookCommand *command = new WebhookCommand(this);
            command->headers = headers;
            if (featureCfg.type() == QVariant::String) {
                // simple GET command without any options
                command->url = featureCfg.toString();
                command->method = HttpMethod::GET;
            } else {
                // the full package
                QVariantMap attrMap = featureCfg.toMap();

                command->url = attrMap.value("url").toString();
                command->method = stringToEnum<HttpMethod::Enum>(attrMap.value("method").toString(), HttpMethod::GET);

                if (command->method != HttpMethod::GET) {
                    command->body = attrMap.value("body");
                    // Attention: QMap.unite inserts the same key multiple times instead of replacing it!
                    QMapIterator<QString, QVariant> iter(attrMap.value("headers").toMap());
                    while (iter.hasNext()) {
                        iter.next();
                        command->headers.insert(iter.key(), iter.value());
                    }
                }

                if (attrMap.contains("response")) {
                    QMapIterator<QString, QVariant> iter(attrMap.value("response").toMap().value("mappings").toMap());
                    while (iter.hasNext()) {
                        iter.next();
                        command->responseMappings.insert(iter.key(), iter.value().toString());
                    }
                }
            }
            entity->commands.insert(feature, command);
        }

        count++;
        m_webhookEntities.insert(entity->id, entity);
    }

    return count;
}

int EntityHandler::convertBrightnessToPercentage(float value) const {
    // TODO(zehnm) implement scaling option
    return static_cast<int>(round(value / 255 * 100));
}

QString EntityHandler::resolveVariables(const QString &text, const QVariantMap &placeholders) const {
    QString newText = text;

    QMapIterator<QString, QVariant> iter(placeholders);
    while (iter.hasNext()) {
        iter.next();
        QString marker = QString("${%1}").arg(iter.key());
        if (newText.contains(marker)) {
            newText.replace(marker, iter.value().toString());
        }
    }

    return newText;
}

QUrl EntityHandler::buildUrl(const QVariant &commandUrl, const QVariantMap &placeholders) const {
    QString baseUrl = resolveVariables(m_baseUrl, placeholders);
    if (!commandUrl.isValid()) {
        return baseUrl;
    }

    QUrl url = resolveVariables(commandUrl.toString(), placeholders);
    if (url.isRelative()) {
        return QUrl(baseUrl).resolved(url);
    }
    return url;
}

WebhookRequest *EntityHandler::createRequest(const QString &commandName, const QString &entityId,
                                             const QVariantMap &placeholders) const {
    if (!m_webhookEntities.contains(entityId)) {
        qCWarning(CLASS_LC) << "Entity not found:" << entityId;
        return Q_NULLPTR;
    }

    WebhookEntity *entity = m_webhookEntities.value(entityId);
    if (!entity->commands.contains(commandName)) {
        qCWarning(CLASS_LC) << "Command" << commandName << "not defined for entity:" << entityId;
        return Q_NULLPTR;
    }
    WebhookCommand *command = entity->commands.value(commandName);

    WebhookRequest *request = new WebhookRequest();
    request->webhookCommand = command;
    request->networkRequest.setUrl(buildUrl(command->url, placeholders));

    if (command->body.isValid()) {
        if (command->body.type() == QVariant::Map) {
            QJsonDocument jsonDoc = QJsonDocument::fromVariant(command->body);
            request->body.append(resolveVariables(jsonDoc.toJson(QJsonDocument::Compact), placeholders));
            request->networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        } else {
            request->body.append(resolveVariables(command->body.toString(), placeholders));
            request->networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/text");
        }
    }

    for (QString headerName : command->headers.keys()) {
        QString headerValue = resolveVariables(command->headers.value(headerName).toString(), placeholders);
        request->networkRequest.setRawHeader(headerName.toUtf8(), headerValue.toUtf8());
    }

    return request;
}

int EntityHandler::retrieveResponseValues(QNetworkReply *reply, const QMap<QString, QString> &mappings,
                                          QVariantMap *values) {
    QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    if (contentType.startsWith("application/json")) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
        return retrieveResponseValues(jsonDoc, mappings, values);
    }

    qCDebug(CLASS_LC) << "Response mapping not yet implemented for content type:" << contentType;

    return 0;
}

int EntityHandler::retrieveResponseValues(const QJsonDocument &jsonDoc, const QMap<QString, QString> &mappings,
                                          QVariantMap *values) {
    int      count = 0;
    JsonPath jsonPath(jsonDoc);

    QMapIterator<QString, QString> iter(mappings);
    while (iter.hasNext()) {
        iter.next();

        QVariant value = jsonPath.value(iter.value());
        if (value.isValid()) {
            count++;
            values->insert(iter.key(), value);
        }
    }

    return count;
}
