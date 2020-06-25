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

    setEntityValues(&parameters, state, color, brightness, colorTemp);

    return createRequest(feature, entityId, parameters);
}

void LightHandler::onReply(int command, EntityInterface *entity, const QVariant &param, QNetworkReply *reply) {
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

    // revert entity / UI state in case request failed
    if (reply->error() != QNetworkReply::NetworkError::NoError) {
        updateEntity(entity, oldState, oldColor, oldBrightness, oldColorTemp);
    }
}

void LightHandler::setEntityValues(QVariantMap *placeholders, int state, const QColor &color, int brightness,
                                   int colorTemp) const {
    placeholders->insert("on_bool", state == LightDef::ON);
    placeholders->insert("on_bin", state);
    placeholders->insert("brightness_percent", brightness);
    placeholders->insert("color_temp", colorTemp);
    placeholders->insert("color_r", color.red());
    placeholders->insert("color_g", color.green());
    placeholders->insert("color_b", color.blue());
    placeholders->insert("color_h", color.hue());
    placeholders->insert("color_s", color.saturation());
    placeholders->insert("color_v", color.value());
}

void LightHandler::updateEntity(EntityInterface *entity, int state, const QVariant &color, int brightness,
                                int colorTemp) {
    entity->setState(state);

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
