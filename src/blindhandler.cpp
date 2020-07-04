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

#include "blindhandler.h"

#include "yio-interface/entities/blindinterface.h"

static Q_LOGGING_CATEGORY(CLASS_LC, "yio.intg.webhook.blind");

BlindHandler::BlindHandler(const QString &baseUrl, QObject *parent) : EntityHandler("switch", baseUrl, parent) {}

WebhookRequest *BlindHandler::createCommandRequest(const QString &entityId, EntityInterface *entity, int command,
                                                   const QVariantMap &placeholders, const QVariant &param) const {
    QString         feature;
    QVariantMap     parameters(placeholders);
    BlindInterface *blindInterface = static_cast<BlindInterface *>(entity->getSpecificInterface());

    // get current entity values for request parameters
    int state = entity->state();
    int position = blindInterface->position();

    switch (command) {
        case BlindDef::C_OPEN:
            feature = "OPEN";
            state = BlindDef::OPEN;
            break;
        case BlindDef::C_CLOSE:
            feature = "CLOSE";
            state = BlindDef::CLOSED;
            break;
        case BlindDef::C_STOP:
            feature = "STOP";
            break;
        case BlindDef::C_POSITION:
            feature = "POSITION";
            position = param.toInt();
            break;
        default:
            qCWarning(CLASS_LC) << "Unsupported command:" << command;
            return Q_NULLPTR;
    }

    qCDebug(CLASS_LC()) << entity->friendly_name() << "command:" << feature << ", parameter:" << param;

    position = convertPosition(position);

    setPlaceholderValues(&parameters, state, position);

    return createRequest(feature, entityId, parameters);
}

void BlindHandler::commandReply(int command, EntityInterface *entity, const QVariant &param,
                                const WebhookRequest *request, QNetworkReply *reply) {
    BlindInterface *blindInterface = static_cast<BlindInterface *>(entity->getSpecificInterface());

    int state = -1;
    int position = -1;
    int oldState = entity->state();
    int oldPosition = blindInterface->position();

    switch (command) {
        case BlindDef::C_OPEN:
            state = BlindDef::OPEN;
            position = 0;
            break;
        case BlindDef::C_CLOSE:
            state = BlindDef::CLOSED;
            position = 100;
            break;
        case BlindDef::C_STOP:
            break;
        case BlindDef::C_POSITION:
            position = param.toInt();
            if (position == 0) {
                state = BlindDef::OPEN;
            } else if (position == 100) {
                state = BlindDef::CLOSED;
            }
            break;
    }

    updateEntity(entity, state, position, false);  // no conversion of UI position!

    if (reply->error() == QNetworkReply::NoError) {
        handleResponseData(entity, request, reply);
    } else {
        // revert entity / UI state in case request failed
        // TODO(zehnm) enhance EntityInterface with refresh() option to simplify entity state -> UI reset.
        updateEntity(entity, oldState, oldPosition, false);
    }
}

const QLoggingCategory &BlindHandler::logCategory() const { return CLASS_LC(); }

bool BlindHandler::isConvertPosition(const QString &entityId) {
    WebhookEntity *entity = m_webhookEntities.value(entityId);
    if (!entity) {
        return false;
    }
    return entity->attributes.value("invert_position", false).toBool();
}

int BlindHandler::convertPosition(int position) const {
    if (position < 0 || position > 100) {
        return position;
    }

    return 100 - position;
}

void BlindHandler::setPlaceholderValues(QVariantMap *placeholders, int state, int position) const {
    // TODO(zehnm) define placeholder constants
    placeholders->insert("state_bool", state == BlindDef::OPEN);
    placeholders->insert("state_bin", state);
    placeholders->insert("position_percent", position);
}

void BlindHandler::updateEntity(EntityInterface *entity, const QVariantMap &placeholders) {
    int state = -1;
    int position = -1;

    if (placeholders.contains("state_bool")) {
        state = placeholders.value("state_bool").toBool() ? BlindDef::OPEN : BlindDef::CLOSED;
    } else if (placeholders.contains("state_bin")) {
        state = placeholders.value("state_bin").toInt() == 0 ? BlindDef::CLOSED : BlindDef::OPEN;
    }

    if (placeholders.contains("position_percent")) {
        position = placeholders.value("position_percent").toInt();
    }

    updateEntity(entity, state, position);
}

void BlindHandler::updateEntity(EntityInterface *entity, int state, int position, bool convert) {
    if (state >= 0) {
        qCDebug(CLASS_LC()) << "Update" << entity->friendly_name() << "state:" << state;
        entity->setState(state);
    }

    if (position >= 0 && entity->isSupported(BlindDef::F_POSITION)) {
        if (convert && isConvertPosition(entity->entity_id())) {
            position = convertPosition(position);
        }
        qCDebug(CLASS_LC()) << "Update" << entity->friendly_name() << "position:" << position;
        entity->updateAttrByIndex(BlindDef::POSITION, position);
    }
}
