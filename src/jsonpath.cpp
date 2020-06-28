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

#include "jsonpath.h"

#include <QJsonArray>
#include <QRegularExpression>
#include <QStringList>

JsonPath::JsonPath(const QJsonDocument &jsonDoc, QObject *parent) : QObject(parent) {
    if (jsonDoc.isArray()) {
        m_root = jsonDoc.array();
    } else {
        m_root = jsonDoc.object();
    }
}

QVariant JsonPath::value(const QString &path, QVariant defaultValue) const {
    if (m_root.isUndefined() || m_root.isNull()) {
        return defaultValue;
    }

    QStringList segments = path.split('.', QString::SkipEmptyParts);
    QJsonValue  currNode = m_root;
    bool        error = false;

    QRegularExpression arrayRegEx("(\\w*)\\[(\\d+)\\]$");

    for (auto segment : segments) {
        QRegularExpressionMatch match = arrayRegEx.match(segment);
        if (match.hasMatch()) {
            QString objectName = match.captured(1);
            if (!objectName.isEmpty()) {
                currNode = nextObjectSegment(currNode, objectName);
                if (currNode == QJsonValue::Undefined) {
                    return defaultValue;
                }
            }

            bool ok = false;
            int  index = match.captured(2).toInt(&ok);
            if (!ok || !currNode.isArray()) {
                return defaultValue;
            }

            QJsonArray arrayNode = currNode.toArray();
            if (index < arrayNode.size()) {
                currNode = arrayNode.at(index);
            } else {
                return defaultValue;
            }
        } else {
            currNode = nextObjectSegment(currNode, segment);
            if (currNode == QJsonValue::Undefined) {
                return defaultValue;
            }
        }
    }

    return (!error ? currNode.toVariant() : defaultValue);
}

QJsonValue JsonPath::nextObjectSegment(const QJsonValue &currNode, const QString &key) const {
    if (currNode.isObject()) {
        QJsonObject objNode = currNode.toObject();
        if (objNode.contains(key)) {
            return objNode.value(key);
        }
    }

    return QJsonValue::Undefined;
}
