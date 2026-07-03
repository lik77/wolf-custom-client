/****************************************************************************
** Meta object code from reading C++ file 'main_window.h'
**
** Created by: The Qt Meta Object Compiler version 67 (Qt 5.15.3)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../src/main_window.h"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'main_window.h' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 67
#error "This file was generated using the moc from 5.15.3. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_MainWindow_t {
    QByteArrayData data[20];
    char stringdata0[295];
};
#define QT_MOC_LITERAL(idx, ofs, len) \
    Q_STATIC_BYTE_ARRAY_DATA_HEADER_INITIALIZER_WITH_OFFSET(len, \
    qptrdiff(offsetof(qt_meta_stringdata_MainWindow_t, stringdata0) + ofs \
        - idx * sizeof(QByteArrayData)) \
    )
static const qt_meta_stringdata_MainWindow_t qt_meta_stringdata_MainWindow = {
    {
QT_MOC_LITERAL(0, 0, 10), // "MainWindow"
QT_MOC_LITERAL(1, 11, 17), // "controlFrameReady"
QT_MOC_LITERAL(2, 29, 0), // ""
QT_MOC_LITERAL(3, 30, 12), // "ControlState"
QT_MOC_LITERAL(4, 43, 5), // "state"
QT_MOC_LITERAL(5, 49, 22), // "customControlRequested"
QT_MOC_LITERAL(6, 72, 7), // "payload"
QT_MOC_LITERAL(7, 80, 19), // "updateOfficialFrame"
QT_MOC_LITERAL(8, 100, 5), // "image"
QT_MOC_LITERAL(9, 106, 15), // "updateHeroFrame"
QT_MOC_LITERAL(10, 122, 16), // "updateGameStatus"
QT_MOC_LITERAL(11, 139, 4), // "text"
QT_MOC_LITERAL(12, 144, 19), // "updateDynamicStatus"
QT_MOC_LITERAL(13, 164, 18), // "updateModuleStatus"
QT_MOC_LITERAL(14, 183, 20), // "updatePositionStatus"
QT_MOC_LITERAL(15, 204, 22), // "updateHeroConfigStatus"
QT_MOC_LITERAL(16, 227, 22), // "updateConnectionStatus"
QT_MOC_LITERAL(17, 250, 9), // "appendLog"
QT_MOC_LITERAL(18, 260, 16), // "emitControlFrame"
QT_MOC_LITERAL(19, 277, 17) // "sendCustomControl"

    },
    "MainWindow\0controlFrameReady\0\0"
    "ControlState\0state\0customControlRequested\0"
    "payload\0updateOfficialFrame\0image\0"
    "updateHeroFrame\0updateGameStatus\0text\0"
    "updateDynamicStatus\0updateModuleStatus\0"
    "updatePositionStatus\0updateHeroConfigStatus\0"
    "updateConnectionStatus\0appendLog\0"
    "emitControlFrame\0sendCustomControl"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_MainWindow[] = {

 // content:
       8,       // revision
       0,       // classname
       0,    0, // classinfo
      13,   14, // methods
       0,    0, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags
       1,    1,   79,    2, 0x06 /* Public */,
       5,    1,   82,    2, 0x06 /* Public */,

 // slots: name, argc, parameters, tag, flags
       7,    1,   85,    2, 0x0a /* Public */,
       9,    1,   88,    2, 0x0a /* Public */,
      10,    1,   91,    2, 0x0a /* Public */,
      12,    1,   94,    2, 0x0a /* Public */,
      13,    1,   97,    2, 0x0a /* Public */,
      14,    1,  100,    2, 0x0a /* Public */,
      15,    1,  103,    2, 0x0a /* Public */,
      16,    1,  106,    2, 0x0a /* Public */,
      17,    1,  109,    2, 0x0a /* Public */,
      18,    0,  112,    2, 0x08 /* Private */,
      19,    0,  113,    2, 0x08 /* Private */,

 // signals: parameters
    QMetaType::Void, 0x80000000 | 3,    4,
    QMetaType::Void, QMetaType::QByteArray,    6,

 // slots: parameters
    QMetaType::Void, QMetaType::QImage,    8,
    QMetaType::Void, QMetaType::QImage,    8,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void, QMetaType::QString,   11,
    QMetaType::Void,
    QMetaType::Void,

       0        // eod
};

void MainWindow::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<MainWindow *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->controlFrameReady((*reinterpret_cast< const ControlState(*)>(_a[1]))); break;
        case 1: _t->customControlRequested((*reinterpret_cast< const QByteArray(*)>(_a[1]))); break;
        case 2: _t->updateOfficialFrame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 3: _t->updateHeroFrame((*reinterpret_cast< const QImage(*)>(_a[1]))); break;
        case 4: _t->updateGameStatus((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 5: _t->updateDynamicStatus((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 6: _t->updateModuleStatus((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 7: _t->updatePositionStatus((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 8: _t->updateHeroConfigStatus((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 9: _t->updateConnectionStatus((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 10: _t->appendLog((*reinterpret_cast< const QString(*)>(_a[1]))); break;
        case 11: _t->emitControlFrame(); break;
        case 12: _t->sendCustomControl(); break;
        default: ;
        }
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        switch (_id) {
        default: *reinterpret_cast<int*>(_a[0]) = -1; break;
        case 0:
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
            using _t = void (MainWindow::*)(const ControlState & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::controlFrameReady)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (MainWindow::*)(const QByteArray & );
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&MainWindow::customControlRequested)) {
                *result = 1;
                return;
            }
        }
    }
}

QT_INIT_METAOBJECT const QMetaObject MainWindow::staticMetaObject = { {
    QMetaObject::SuperData::link<QMainWindow::staticMetaObject>(),
    qt_meta_stringdata_MainWindow.data,
    qt_meta_data_MainWindow,
    qt_static_metacall,
    nullptr,
    nullptr
} };


const QMetaObject *MainWindow::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *MainWindow::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_MainWindow.stringdata0))
        return static_cast<void*>(this);
    return QMainWindow::qt_metacast(_clname);
}

int MainWindow::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QMainWindow::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 13)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 13;
    }
    return _id;
}

// SIGNAL 0
void MainWindow::controlFrameReady(const ControlState & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 0, _a);
}

// SIGNAL 1
void MainWindow::customControlRequested(const QByteArray & _t1)
{
    void *_a[] = { nullptr, const_cast<void*>(reinterpret_cast<const void*>(std::addressof(_t1))) };
    QMetaObject::activate(this, &staticMetaObject, 1, _a);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
