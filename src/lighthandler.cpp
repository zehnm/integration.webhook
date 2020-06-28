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

#include <QLoggingCategory>

#include "yio-interface/entities/lightinterface.h"

static Q_LOGGING_CATEGORY(CLASS_LC, "yio.intg.webhook");

LightHandler::LightHandler(const QString &baseUrl, QObject *parent) : EntityHandler("light", baseUrl, parent) {}

WebhookRequest *LightHandler::prepareRequest(const QString &entityId, EntityInterface *entity, int command,
                                             const QVariantMap &placeholders, const QVariant &param) {
    Q_UNUSED(entity)

    QString         feature;
    QVariantMap     parameters(placeholders);
    LightInterface *lightInterface = static_cast<LightInterface *>(entity->getSpecificInterface());

    // get current entity values for request parameters
    int    state = entity->state();
    QColor color = lightInterface->color();
    int    colorTemp = lightInterface->colorTemp();
    int    brightness = lightInterface->brightness();

    switch (command) {
        case LightDef::C_ON:
            feature = "ON";
            state = LightDef::ON;
            break;
        case LightDef::C_OFF:
            feature = "OFF";
            state = LightDef::OFF;
            break;
        case LightDef::C_TOGGLE:
            feature = "TOGGLE";
            break;
        case LightDef::C_BRIGHTNESS:
            feature = "BRIGHTNESS";
            brightness = param.toInt();
            state = brightness > 0 ? LightDef::ON : LightDef::OFF;
            break;
        case LightDef::C_COLOR:
            feature = "COLOR";
            color = param.value<QColor>();
            break;
        case LightDef::C_COLORTEMP: {
            feature = "COLORTEMP";
            colorTemp = param.toInt();
            break;
        }
        default:
            qCWarning(CLASS_LC) << "Unsupported command:" << command;
            return Q_NULLPTR;
    }

    setPlaceholderValues(&parameters, state, color, brightness, colorTemp);

    return createRequest(feature, entityId, parameters);
}

void LightHandler::onReply(int command, EntityInterface *entity, const QVariant &param, const WebhookRequest *request,
                           QNetworkReply *reply) {
    LightInterface *lightInterface = static_cast<LightInterface *>(entity->getSpecificInterface());

    // reflect intended state in entity. This is also required in case of a failed request!
    int      state = LightDef::OFF;
    int      brightness = -1;
    int      colorTemp = -1;
    QVariant color;
    int      oldState = entity->state();
    int      oldBrightness = lightInterface->brightness();
    int      oldColorTemp = lightInterface->colorTemp();
    QVariant oldColor = lightInterface->color();

    switch (command) {
        case LightDef::C_ON:
            state = LightDef::ON;
            break;
        case LightDef::C_OFF:
            state = LightDef::OFF;
            break;
        case LightDef::C_BRIGHTNESS:
            state = LightDef::ON;
            brightness = param.toInt();
            break;
        case LightDef::C_COLOR:
            state = LightDef::ON;
            color = param;
            break;
        case LightDef::C_COLORTEMP:
            state = LightDef::ON;
            break;
    }

    updateEntity(entity, state, color, brightness, colorTemp);

    if (reply->error() == QNetworkReply::NoError) {
        QVariantMap values;
        int         count = retrieveResponseValues(reply, request->webhookCommand->responseMappings, &values);
        if (count > 0 && CLASS_LC().isDebugEnabled()) {
            qCDebug(CLASS_LC) << "Extracted response values:" << values;
            updateEntity(entity, values);
        }
    } else {
        // revert entity / UI state in case request failed
        // TODO(zehnm) enhance EntityInterface with refresh() option to simplify entity state -> UI reset.
        updateEntity(entity, oldState, oldColor, oldBrightness, oldColorTemp);
    }
}

void LightHandler::setPlaceholderValues(QVariantMap *placeholders, int state, const QColor &color, int brightness,
                                        int colorTemp) const {
    // TODO(zehnm) define placeholder constants
    placeholders->insert("state_bool", state == LightDef::ON);
    placeholders->insert("state_bin", state);
    placeholders->insert("brightness_percent", brightness);
    placeholders->insert("color_temp", colorTemp);
    placeholders->insert("color_r", color.red());
    placeholders->insert("color_g", color.green());
    placeholders->insert("color_b", color.blue());
    placeholders->insert("color_h", color.hue());
    placeholders->insert("color_s", color.saturation());
    placeholders->insert("color_v", color.value());
}

void LightHandler::updateEntity(EntityInterface *entity, const QVariantMap &placeholders) {
    int    state = -1;
    int    brightness = -1;
    int    colorTemp = -1;
    QColor color;

    if (placeholders.contains("state_bool")) {
        state = placeholders.value("state_bool").toBool() ? LightDef::ON : LightDef::OFF;
    } else if (placeholders.contains("state_bin")) {
        state = placeholders.value("state_bin").toInt() == 0 ? LightDef::OFF : LightDef::ON;
    }

    if (placeholders.contains("brightness_percent")) {
        brightness = placeholders.value("brightness_percent").toInt();
    }

    // TODO(zehnm) other brightness types, e.g. float values 0.0..1.0?

    if (placeholders.contains("color_temp")) {
        colorTemp = placeholders.value("color_temp").toInt();
    }

    // TODO(zehnm) color temperature conversion? --> not yet implemented in the UI!

    if (placeholders.contains("color_r")) {
        color.setRgb(placeholders.value("color_r").toInt(), placeholders.value("color_g").toInt(),
                     placeholders.value("color_b").toInt());
    } else if (placeholders.contains("color_h")) {
        color.setHsv(placeholders.value("color_h").toInt(), placeholders.value("color_s").toInt(),
                     placeholders.value("color_v").toInt());
    }

    updateEntity(entity, state, color, brightness, colorTemp);
}

void LightHandler::updateEntity(EntityInterface *entity, int state, const QVariant &color, int brightness,
                                int colorTemp) {
    if (state >= 0) {
        entity->setState(state);
    }

    if (brightness >= 0 && entity->isSupported(LightDef::F_BRIGHTNESS)) {
        entity->updateAttrByIndex(LightDef::BRIGHTNESS, brightness);
    }

    if (color.isValid() && entity->isSupported(LightDef::F_COLOR)) {
        entity->updateAttrByIndex(LightDef::COLOR, color);
    }

    if (colorTemp >= 0 && entity->isSupported(LightDef::F_COLORTEMP)) {
        entity->updateAttrByIndex(LightDef::COLORTEMP, colorTemp);
    }
}
