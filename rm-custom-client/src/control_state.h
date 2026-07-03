#pragma once

#include <QtCore/QMetaType>
#include <QtCore/QByteArray>

struct ControlState {
    int mouseX = 0;
    int mouseY = 0;
    int mouseZ = 0;
    bool leftButtonDown = false;
    bool rightButtonDown = false;
    bool midButtonDown = false;
    quint32 keyboardMask = 0;
};

Q_DECLARE_METATYPE(ControlState)
