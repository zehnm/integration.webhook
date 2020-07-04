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

#include <QColor>

#include "entityhandler.h"

class LightHandler : public EntityHandler {
    Q_OBJECT

 public:
    explicit LightHandler(const QString &baseUrl, QObject *parent = nullptr);

    // EntityHandler interface
 public:
    WebhookRequest *createCommandRequest(const QString &entityId, EntityInterface *entity, int command,
                                         const QVariantMap &placeholders, const QVariant &param) const override;

    void commandReply(int command, EntityInterface *entity, const QVariant &param, const WebhookRequest *request,
                      QNetworkReply *reply) override;

 protected:
    const QLoggingCategory &logCategory() const override;

    void updateEntity(EntityInterface *entity, const QVariantMap &placeholders) override;

 private:
    void setPlaceholderValues(QVariantMap *placeholders, int state, const QColor &color, int brightness,
                              int colorTemp) const;

    void updateEntity(EntityInterface *entity, int state, const QVariant &color, int brightness, int colorTemp);
};
