/*
 * Copyright (c) 2020 Meltytech, LLC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.0
import org.shotcut.qml 1.0

Metadata {
    type: Metadata.Filter
    name: qsTr("Corner Pin")
    mlt_service: "frei0r.c0rners"
    qml: "ui.qml"
    keyframes {
        allowAnimateIn: true
        allowAnimateOut: true
        simpleProperties: ['0', '1', '2', '3', '4', '5', '6', '7', '9', '10', '13']
        parameters: [
            Parameter {
                name: qsTr('Corner 1 X')
                property: '0'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Corner 1 Y')
                property: '1'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Corner 2 X')
                property: '2'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Corner 2 Y')
                property: '3'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Corner 3 X')
                property: '4'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Corner 3 Y')
                property: '5'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Corner 4 X')
                property: '6'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Corner 4 Y')
                property: '7'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
            },
            Parameter {
                name: qsTr('Stretch X')
                property: '9'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
             },
             Parameter {
                name: qsTr('Stretch Y')
                property: '10'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
             },
             Parameter {
                name: qsTr('Feathering')
                property: '13'
                isSimple: true
                isCurve: true
                minimum: 0
                maximum: 1
             }
        ]
    }
}
