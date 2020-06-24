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

#include "lighthandler.h"

#include <QColor>
#include <QLoggingCategory>
#include <QStringList>
#include <QVariantMap>

#include "yio-interface/entities/lightinterface.h"

static Q_LOGGING_CATEGORY(CLASS_LC, "yio.intg.webhook");

LightHandler::LightHandler(const QString &baseUrl, QObject *parent) : EntityHandler("light", baseUrl, parent) {}

WebhookRequest *LightHandler::prepareRequest(const QString &entityId, EntityInterface *entity, int command,
                                             const QVariantMap &placeholders, const QVariant &param) {
    Q_UNUSED(entity)

    switch (command) {
        case LightDef::C_ON:
            return createRequest("ON", entityId, placeholders);
        case LightDef::C_OFF:
            return createRequest("OFF", entityId, placeholders);
        case LightDef::C_TOGGLE:
            return createRequest("TOGGLE", entityId, placeholders);
        case LightDef::C_BRIGHTNESS:
            // TODO(zehnm) implement me! Scale & put brightness value into variable replacements
            // data.insert("brightness_pct", param);
            return createRequest("BRIGHTNESS", entityId, placeholders);
        case LightDef::C_COLOR: {
            // TODO(zehnm) implement me! Scale & put color values into variable replacements
            QColor       color = param.value<QColor>();
            QVariantList list;
            list.append(color.red());
            list.append(color.green());
            list.append(color.blue());
            return createRequest("COLOR", entityId, placeholders);
        }
        default:
            qCWarning(CLASS_LC) << "Unsupported command:" << command;
    }

    return Q_NULLPTR;
}

void LightHandler::onReply(int command, EntityInterface *entity, const QVariant &param, QNetworkReply *reply) {
    Q_UNUSED(param)

    // TODO(zehnm) implement all features
    switch (command) {
        case LightDef::C_ON:
            entity->setState(LightDef::ON);
            break;
        case LightDef::C_OFF:
            entity->setState(LightDef::OFF);
            break;
    }

    // revert entity / UI state in case request failed
    if (reply->error() != QNetworkReply::NetworkError::NoError) {
        switch (command) {
            case LightDef::C_ON:
                entity->setState(LightDef::OFF);
                break;
            case LightDef::C_OFF:
                entity->setState(LightDef::ON);
                break;
        }
    }
}

void LightHandler::updateEntity(EntityInterface *entity, const QVariantMap &attr) {
    // TODO(zehnm) implement me, this is just copy pasted from HA integration!

    // state
    if (attr.value("state").toString() == "on") {
        entity->setState(LightDef::ON);
    } else {
        entity->setState(LightDef::OFF);
    }

    QVariantMap haAttr = attr.value("attributes").toMap();
    // brightness
    if (entity->isSupported(LightDef::F_BRIGHTNESS)) {
        if (haAttr.contains("brightness")) {
            entity->updateAttrByIndex(LightDef::BRIGHTNESS,
                                      convertBrightnessToPercentage(haAttr.value("brightness").toInt()));
        } else {
            entity->updateAttrByIndex(LightDef::BRIGHTNESS, 0);
        }
    }

    // color
    if (entity->isSupported(LightDef::F_COLOR)) {
        QVariant     color = haAttr.value("rgb_color");
        QVariantList cl(color.toList());
        char         buffer[10];
        snprintf(buffer, sizeof(buffer), "#%02X%02X%02X", cl.value(0).toInt(), cl.value(1).toInt(),
                 cl.value(2).toInt());
        entity->updateAttrByIndex(LightDef::COLOR, buffer);
    }

    // color temp
    if (entity->isSupported(LightDef::F_COLORTEMP)) {
        // FIXME implement me!
    }
}
