/****************************************************************************
** Meta object code from reading C++ file 'mqtt_client.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/mqtt_client.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'mqtt_client.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MqttClient_t {
    QByteArrayData data[15];
    char stringdata0[224];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MqttClient_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MqttClient_t qt_meta_stringdata_MqttClient = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MqttClient"
QT_MOC_LITERAL(1, 11, 23), // "customByteBlockReceived"
QT_MOC_LITERAL(2, 35, 0), // ""
QT_MOC_LITERAL(3, 36, 7), // "payload"
QT_MOC_LITERAL(4, 44, 17), // "gameStatusUpdated"
QT_MOC_LITERAL(5, 62, 4), // "text"
QT_MOC_LITERAL(6, 67, 20), // "dynamicStatusUpdated"
QT_MOC_LITERAL(7, 88, 19), // "moduleStatusUpdated"
QT_MOC_LITERAL(8, 108, 21), // "positionStatusUpdated"
QT_MOC_LITERAL(9, 130, 22), // "connectionStateChanged"
QT_MOC_LITERAL(10, 153, 10), // "logMessage"
QT_MOC_LITERAL(11, 164, 19), // "publishControlFrame"
QT_MOC_LITERAL(12, 184, 12), // "ControlState"
QT_MOC_LITERAL(13, 197, 5), // "state"
QT_MOC_LITERAL(14, 203, 20) // "publishCustomControl"

    },
    "MqttClient\0customByteBlockReceived\0\0"
    "payload\0gameStatusUpdated\0text\0"
    "dynamicStatusUpdated\0moduleStatusUpdated\0"
    "positionStatusUpdated\0connectionStateChanged\0"
    "logMessage\0publishControlFrame\0"
    "ControlState\0state\0publishCustomControl"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MqttClient[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
       9,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       7,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   59,    2, 0x06 /* Public */,
       4,    1,   62,    2, 0x06 /* Public */,
       6,    1,   65,    2, 0x06 /* Public */,
       7,    1,   68,    2, 0x06 /* Public */,
       8,    1,   71,    2, 0x06 /* Public */,
       9,    1,   74,    2, 0x06 /* Public */,
      10,    1,   77,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
      11,    1,   80,    2, 0x0a /* Public */,
      14,    1,   83,    2, 0x0a /* Public */,

 // signals: parameters
    QMetaType::Void, QMetaType::QByteArray,    3,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,
    QMetaType::Void, QMetaType::QString,    5,

 // slots: parameters
    QMetaType::Void, 0x80000000 | 12,   13,
    QMetaType::Void, QMetaType::QByteArray,    3,

       0        // eod
};

void MqttClient::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MqttClient *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->customByteBlockReceived((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 1: _t->gameStatusUpdated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 2: _t->dynamicStatusUpdated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 3: _t->moduleStatusUpdated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 4: _t->positionStatusUpdated((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->connectionStateChanged((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->logMessage((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->publishControlFrame((*reinterpret_cast< const ControlState(*)>(_a[1]))); break;
        case 8: _t->publishCustomControl((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 7:
            switch (*reinterpret_cast<int*>(_a[1])) {
            default: *reinterpret_cast<int*>(_a[0]) = -1; break;
            case 0:
                *reinterpret_cast<int*>(_a[0]) = qRegisterMetaType< ControlState >(); break;
            }
            break;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (MqttClient::*)(const QByteArray & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MqttClient::customByteBlockReceived)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MqttClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MqttClient::gameStatusUpdated)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (MqttClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MqttClient::dynamicStatusUpdated)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (MqttClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MqttClient::moduleStatusUpdated)) {
                *result = 3;
                return;
            }
        }
        {
            using _t = void (MqttClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MqttClient::positionStatusUpdated)) {
                *result = 4;
                return;
            }
        }
        {
            using _t = void (MqttClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MqttClient::connectionStateChanged)) {
                *result = 5;
                return;
            }
        }
        {
            using _t = void (MqttClient::*)(const QString & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MqttClient::logMessage)) {
                *result = 6;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MqttClient::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_MqttClient.data,
    qt_meta_data_MqttClient,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MqttClient::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MqttClient::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MqttClient.stringdata0))
        return static_cast<void*>(this);
    if (!strcmp(_clname, "mqtt::callback"))
        return static_cast< mqtt::callback*>(this);
    return QObject::qt_metacast(_clname);
}

int MqttClient::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 9)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 9;
    }
    return _id;
}

// SIGNAL 0
void MqttClient::customByteBlockReceived(const QByteArray & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MqttClient::gameStatusUpdated(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}

// SIGNAL 2
void MqttClient::dynamicStatusUpdated(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 2, _a);
}

// SIGNAL 3
void MqttClient::moduleStatusUpdated(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 3, _a);
}

// SIGNAL 4
void MqttClient::positionStatusUpdated(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 4, _a);
}

// SIGNAL 5
void MqttClient::connectionStateChanged(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 5, _a);
}

// SIGNAL 6
void MqttClient::logMessage(const QString & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 6, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
