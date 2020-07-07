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

#include "climatehandler.h"

#include "yio-interface/entities/climateinterface.h"

static Q_LOGGING_CATEGORY(CLASS_LC, "yio.intg.webhook.climate");

ClimateHandler::ClimateHandler(const QString &baseUrl, QObject *parent) : EntityHandler("climate", baseUrl, parent) {}

bool ClimateHandler::onWebhookEntityRead(const QVariantMap &entityCfgMap, WebhookEntity *entity) {
    Q_UNUSED(entityCfgMap)

    // enable features based on configuration attributes, otherwise ui interaction will be very limited!
    if (entity->attributes.contains("min_temp")) {
        entity->supportedFeatures.append("TEMPERATURE_MIN");
    }
    if (entity->attributes.contains("max_temp")) {
        entity->supportedFeatures.append("TEMPERATURE_MAX");
    }
    if (entity->attributes.contains("current_temp")) {
        entity->supportedFeatures.append("TEMPERATURE");
    }

    if (entity->supportedFeatures.contains("HEAT") || entity->supportedFeatures.contains("COOL")) {
        entity->supportedFeatures.append("HVAC_MODES");
    }
    return true;
}

WebhookRequest *ClimateHandler::createCommandRequest(const QString &entityId, EntityInterface *entity, int command,
                                                     const QVariantMap &placeholders, const QVariant &param) const {
    QString           feature;
    QVariantMap       parameters(placeholders);
    ClimateInterface *climateInterface = static_cast<ClimateInterface *>(entity->getSpecificInterface());

    // get current entity values for request parameters
    int    state;
    double targetTemperature = climateInterface->targetTemperature();

    switch (command) {
        case ClimateDef::C_OFF:
            feature = "OFF";
            state = ClimateDef::OFF;
            break;
        case ClimateDef::C_ON:
            feature = "ON";
            state = ClimateDef::ON;
            break;
        case ClimateDef::C_HEAT:
            feature = "HEAT";
            state = ClimateDef::HEAT;
            break;
        case ClimateDef::C_COOL:
            feature = "COOL";
            state = ClimateDef::COOL;
            break;
        case ClimateDef::C_TARGET_TEMPERATURE:
            feature = "TARGET_TEMPERATURE";
            state = ClimateDef::TARGET_TEMPERATURE;
            targetTemperature = param.toDouble();
            break;
        default:
            qCWarning(CLASS_LC) << "Unsupported command:" << command;
            return nullptr;
    }

    qCDebug(CLASS_LC()) << entity->friendly_name() << "command:" << feature << ", parameter:" << param;

    setPlaceholderValues(&parameters, state, targetTemperature);

    return createRequest(feature, entityId, parameters);
}

void ClimateHandler::commandReply(int command, EntityInterface *entity, const QVariant &param,
                                  const WebhookRequest *request, QNetworkReply *reply) {
    ClimateInterface *climateInterface = static_cast<ClimateInterface *>(entity->getSpecificInterface());

    QVariantMap attributes;
    QVariantMap oldAttributes;

    oldAttributes.insert("state", entity->state());
    oldAttributes.insert("current_temp", climateInterface->temperature());
    oldAttributes.insert("target_temp", climateInterface->targetTemperature());
    oldAttributes.insert("min_temp", climateInterface->temperatureMax());
    oldAttributes.insert("max_temp", climateInterface->temperatureMin());

    switch (command) {
        case ClimateDef::C_OFF:
            attributes.insert("state", ClimateDef::States::OFF);
            break;
        case ClimateDef::C_ON:
            attributes.insert("state", ClimateDef::States::ON);
            break;
        case ClimateDef::C_HEAT:
            attributes.insert("state", ClimateDef::States::HEAT);
            break;
        case ClimateDef::C_COOL:
            attributes.insert("state", ClimateDef::States::COOL);
            break;
        case ClimateDef::C_TARGET_TEMPERATURE:
            attributes.insert("target_temp", param);
            break;
    }

    updateEntity(entity, attributes);

    if (reply->error() == QNetworkReply::NoError) {
        handleResponseData(entity, request, reply);
    } else {
        // revert entity / UI state in case request failed
        // TODO(zehnm) enhance EntityInterface with refresh() option to simplify entity state -> UI reset.
        updateEntity(entity, oldAttributes);
    }
}

const QLoggingCategory &ClimateHandler::logCategory() const { return CLASS_LC(); }

void ClimateHandler::updateEntity(EntityInterface *entity, const QVariantMap &attributes) {
    if (!entity) {
        return;
    }

    if (attributes.contains("state")) {
        // TODO(zehnm) user configurabel state mapping. Eg. "cooling", "off", "0", "1", ...
        int state = attributes.value("state").toInt();
        entity->setState(state);
    }

    // TODO(zehnm) Fahrenheit / Celsius conversion logic
    // - Add configuration option for F / C of the called device
    // - Check unit system of remote
    // - Figure out temperature handling of the yio climate entity.
    //   Afaik it just displays the value received from the integration!
    // UnitSystem::Enum us = configObj->getUnitSystem();

    if (entity->isSupported(ClimateDef::F_TEMPERATURE) && attributes.contains("current_temp")) {
        entity->updateAttrByIndex(ClimateDef::TEMPERATURE, attributes.value("current_temp").toDouble());
    }
    if (entity->isSupported(ClimateDef::F_TARGET_TEMPERATURE) && attributes.contains("target_temp")) {
        entity->updateAttrByIndex(ClimateDef::TARGET_TEMPERATURE, attributes.value("target_temp"));
    }
    if (entity->isSupported(ClimateDef::F_TEMPERATURE_MIN) && attributes.contains("min_temp")) {
        entity->updateAttrByIndex(ClimateDef::TEMPERATURE_MIN, attributes.value("min_temp"));
    }
    if (entity->isSupported(ClimateDef::F_TEMPERATURE_MAX) && attributes.contains("max_temp")) {
        entity->updateAttrByIndex(ClimateDef::TEMPERATURE_MAX, attributes.value("max_temp"));
    }
}

void ClimateHandler::setPlaceholderValues(QVariantMap *placeholders, int state, double targetTemperature) const {
    // TODO(zehnm) define placeholder constants
    placeholders->insert("state", state);
    // TODO(zehnm) Fahrenheit / Celsius conversion?
    placeholders->insert("target_temp", targetTemperature);
}
