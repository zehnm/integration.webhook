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

#include "switchhandler.h"

#include "yio-interface/entities/switchinterface.h"

static Q_LOGGING_CATEGORY(CLASS_LC, "yio.intg.webhook.switch");

SwitchHandler::SwitchHandler(const QString &baseUrl, QObject *parent) : EntityHandler("switch", baseUrl, parent) {}

WebhookRequest *SwitchHandler::prepareRequest(const QString &entityId, EntityInterface *entity, int command,
                                              const QVariantMap &placeholders, const QVariant &param) {
    Q_UNUSED(entity)
    Q_UNUSED(param)

    switch (command) {
        case SwitchDef::C_ON:
            return createRequest("ON", entityId, placeholders);
        case SwitchDef::C_OFF:
            return createRequest("OFF", entityId, placeholders);
        case SwitchDef::C_TOGGLE:
            return createRequest("TOGGLE", entityId, placeholders);
        default:
            qCWarning(CLASS_LC) << "Unsupported command:" << command;
    }

    return Q_NULLPTR;
}

void SwitchHandler::onReply(int command, EntityInterface *entity, const QVariant &param, const WebhookRequest *request,
                            QNetworkReply *reply) {
    Q_UNUSED(param)

    int oldState = entity->state();

    if ((command == SwitchDef::C_ON || command == SwitchDef::C_OFF)) {
        entity->setState(command == SwitchDef::C_ON ? SwitchDef::ON : SwitchDef::OFF);
    }

    if (reply->error() == QNetworkReply::NoError) {
        QVariantMap values;
        int         count = retrieveResponseValues(reply, request->webhookCommand->responseMappings, &values);
        if (count > 0) {
            if (CLASS_LC().isDebugEnabled()) {
                qCDebug(CLASS_LC) << "Extracted response values:" << values;
            }
            updateEntity(entity, values);
        }
    } else {
        // revert entity / UI state in case request failed
        entity->setState(oldState);
    }
}

const QLoggingCategory &SwitchHandler::logCategory() const { return CLASS_LC(); }

void SwitchHandler::updateEntity(EntityInterface *entity, const QVariantMap &placeholders) {
    int state = -1;

    if (placeholders.contains("state_bool")) {
        state = placeholders.value("state_bool").toBool() ? SwitchDef::ON : SwitchDef::OFF;
    } else if (placeholders.contains("state_bin")) {
        state = placeholders.value("state_bin").toInt() == 0 ? SwitchDef::OFF : SwitchDef::ON;
    }

    if (state >= 0) {
        qCDebug(CLASS_LC()) << "Update" << entity->friendly_name() << "state:" << state;
        entity->setState(state);
    }

    if (placeholders.contains("power")) {
        int power = placeholders.value("power").toInt();
        if (power >= 0 && entity->isSupported(SwitchDef::F_POWER)) {
            qCDebug(CLASS_LC()) << "Update" << entity->friendly_name() << "power:" << state;
            entity->updateAttrByIndex(SwitchDef::POWER, power);
        }
    }
}
