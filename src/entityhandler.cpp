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

#include <QRegularExpression>

#include "jsonpath.h"

const QString EntityHandler::STATUS_COMMAND = "STATUS_POLLING";

EntityHandler::EntityHandler(const QString &entityType, const QString &baseUrl, QObject *parent)
    : QObject(parent), m_entityType(entityType), m_baseUrl(baseUrl) {}

int EntityHandler::readEntities(const QVariantList &entityCfgList, const QVariantMap &headers) {
    int count = 0;
    for (const QVariant &entityCfg : entityCfgList) {
        QVariantMap    entityCfgMap = entityCfg.toMap();
        WebhookEntity *entity = new WebhookEntity(entityCfgMap.value("entity_id").toString(), entityType(),
                                                  entityCfgMap.value("friendly_name").toString(),
                                                  entityCfgMap.value("attributes").toMap(), this);

        QMapIterator<QString, QVariant> iter(entityCfgMap.value("commands").toMap());
        while (iter.hasNext()) {
            iter.next();
            QString  feature = iter.key();
            QVariant featureCfg = iter.value();
            if (feature != STATUS_COMMAND) {
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

        if (onWebhookEntityRead(entityCfgMap, entity)) {
            count++;
            m_webhookEntities.insert(entity->id, entity);
        }
    }

    return count;
}

QMapIterator<QString, WebhookEntity *> EntityHandler::entityIter() const {
    return QMapIterator<QString, WebhookEntity *>(m_webhookEntities);
}

void EntityHandler::initialize(EntitiesInterface *entities) {
    // transfer entity attributes to configure entity or initialize values
    QMapIterator<QString, WebhookEntity *> iter = entityIter();
    while (iter.hasNext()) {
        iter.next();
        const WebhookEntity *webhook = iter.value();

        EntityInterface *entity = entities->getEntityInterface(webhook->id);
        updateEntity(entity, webhook->attributes);
    }
}

bool EntityHandler::hasStatusCommand(const QString &entityId) const {
    WebhookEntity *entity = m_webhookEntities.value(entityId);
    if (!entity) {
        return false;
    }
    return entity->commands.contains(STATUS_COMMAND);
}

WebhookRequest *EntityHandler::createStatusRequest(const QString &entityId, const QVariantMap &placeholders) const {
    return createRequest(STATUS_COMMAND, entityId, placeholders);
}

void EntityHandler::statusReply(EntityInterface *entity, const WebhookRequest *request, QNetworkReply *reply) {
    if (reply->error() == QNetworkReply::NoError) {
        handleResponseData(entity, request, reply);
    }
}

int EntityHandler::convertBrightnessToPercentage(float value) const {
    // TODO(zehnm) implement scaling option
    return static_cast<int>(round(value / 255 * 100));
}

QString EntityHandler::resolveVariables(const QString &text, const QVariantMap &placeholders) const {
    if (placeholders.isEmpty()) {
        return text;
    }

    QString resolved = text;

    // Simplified c printf regex supporting decimal and hex only. Good enough for now :-)
    // For strict parsing and supporting floats:
    // "\\$\\{(\\w+)(:(%(?:\\d+\\$)?[+-]?(?:[ 0]|'.{1})?-?\\d*(?:\\.\\d+)?[dfFxX]))?\\}"
    // http://www.cplusplus.com/reference/cstdio/printf/
    QRegularExpression              markerRegEx("\\$\\{(\\w+)(:(%\\w*[dxX]))?\\}");
    QRegularExpressionMatchIterator i = markerRegEx.globalMatch(text);
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString                 varName = match.captured(1);
        QString                 format = match.captured(3);

        if (placeholders.contains(varName)) {
            if (format.isEmpty()) {
                resolved.replace(match.captured(0), placeholders.value(varName).toString());
            } else {
                bool ok;
                int  number = placeholders.value(varName).toInt(&ok);
                if (!ok) {
                    qCWarning(logCategory())
                        << "Variable format only supports numbers! Value:" << placeholders.value(varName)
                        << ", placeholder:" << match.captured(0);
                } else {
                    QString formattedValue = QString::asprintf(qPrintable(format), number);
                    resolved.replace(match.captured(0), formattedValue);
                }
            }
        }
    }

    return resolved;
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
        qCWarning(logCategory()) << "Entity not found:" << entityId;
        return nullptr;
    }

    WebhookEntity *entity = m_webhookEntities.value(entityId);
    if (!entity->commands.contains(commandName)) {
        qCWarning(logCategory()) << "Command" << commandName << "not defined for entity:" << entityId;
        return nullptr;
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

    QMapIterator<QString, QVariant> iter(command->headers);
    while (iter.hasNext()) {
        iter.next();
        QString headerValue = resolveVariables(command->headers.value(iter.key()).toString(), placeholders);
        request->networkRequest.setRawHeader(iter.key().toUtf8(), headerValue.toUtf8());
    }

    return request;
}

void EntityHandler::handleResponseData(EntityInterface *entity, const WebhookRequest *request, QNetworkReply *reply) {
    if (request->webhookCommand->responseMappings.isEmpty()) {
        return;
    }

    QVariantMap values;
    if (retrieveResponseValues(reply, request->webhookCommand->responseMappings, &values) > 0) {
        if (logCategory().isDebugEnabled()) {
            qCDebug(logCategory()) << "Extracted response values:" << values;
        }
        updateEntity(entity, values);
    }
}

int EntityHandler::retrieveResponseValues(QNetworkReply *reply, const QMap<QString, QString> &mappings,
                                          QVariantMap *values) {
    // check optional Content-Length if body parsing can be skipped
    // https://www.w3.org/Protocols/rfc2616/rfc2616-sec14.html
    QVariant length = reply->header(QNetworkRequest::ContentLengthHeader);
    if (length.isValid() && length.toUInt() == 0) {
        return 0;
    }

    QString contentType = reply->header(QNetworkRequest::ContentTypeHeader).toString();
    if (contentType.startsWith("application/json")) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(reply->readAll());
        if (jsonDoc.isNull() || jsonDoc.isEmpty()) {
            return 0;
        }
        return retrieveResponseValues(jsonDoc, mappings, values);
    }

    qCDebug(logCategory()) << "Response mapping not yet implemented for content type:" << contentType;

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
