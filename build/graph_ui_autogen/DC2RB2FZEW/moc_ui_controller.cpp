/****************************************************************************
** Meta object code from reading C++ file 'ui_controller.hpp'
**
** Created by: The Qt Meta Object Compiler version 68 (Qt 6.2.4)
**
** WARNING! All changes made in this file will be lost!
*****************************************************************************/

#include <memory>
#include "../../../graph_ui/include/ui_controller.hpp"
#include <QtCore/qbytearray.h>
#include <QtCore/qmetatype.h>
#if !defined(Q_MOC_OUTPUT_REVISION)
#error "The header file 'ui_controller.hpp' doesn't include <QObject>."
#elif Q_MOC_OUTPUT_REVISION != 68
#error "This file was generated using the moc from 6.2.4. It"
#error "cannot be used with the include files from this version of Qt."
#error "(The moc has changed too much.)"
#endif

QT_BEGIN_MOC_NAMESPACE
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
struct qt_meta_stringdata_tux_ti83__UIController_t {
    const uint offsetsAndSize[20];
    char stringdata0[123];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_tux_ti83__UIController_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_tux_ti83__UIController_t qt_meta_stringdata_tux_ti83__UIController = {
    {
QT_MOC_LITERAL(0, 22), // "tux_ti83::UIController"
QT_MOC_LITERAL(23, 14), // "displayChanged"
QT_MOC_LITERAL(38, 0), // ""
QT_MOC_LITERAL(39, 14), // "historyChanged"
QT_MOC_LITERAL(54, 12), // "processInput"
QT_MOC_LITERAL(67, 7), // "tokenId"
QT_MOC_LITERAL(75, 18), // "restoreFromHistory"
QT_MOC_LITERAL(94, 5), // "index"
QT_MOC_LITERAL(100, 14), // "currentDisplay"
QT_MOC_LITERAL(115, 7) // "history"

    },
    "tux_ti83::UIController\0displayChanged\0"
    "\0historyChanged\0processInput\0tokenId\0"
    "restoreFromHistory\0index\0currentDisplay\0"
    "history"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_tux_ti83__UIController[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       4,   14, // methods
       2,   46, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       2,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   38,    2, 0x06,    3 /* Public */,
       3,    0,   39,    2, 0x06,    4 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       4,    1,   40,    2, 0x02,    5 /* Public */,
       6,    1,   43,    2, 0x02,    7 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::Int,    5,
    QMetaType::Void, QMetaType::Int,    7,

 // properties: name, type, flags
       8, QMetaType::QString, 0x00015001, uint(0), 0,
       9, QMetaType::QStringList, 0x00015001, uint(1), 0,

       0        // eod
};

void tux_ti83::UIController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UIController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->displayChanged(); break;
        case 1: _t->historyChanged(); break;
        case 2: _t->processInput((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 3: _t->restoreFromHistory((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        default: ;
        }
    } else if (_c == QMetaObject::IndexOfMethod) {
        int *result = reinterpret_cast<int *>(_a[0]);
        {
            using _t = void (UIController::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UIController::displayChanged)) {
                *result = 0;
                return;
            }
        }
        {
            using _t = void (UIController::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UIController::historyChanged)) {
                *result = 1;
                return;
            }
        }
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty) {
        auto *_t = static_cast<UIController *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0: *reinterpret_cast< QString*>(_v) = _t->currentDisplay(); break;
        case 1: *reinterpret_cast< QStringList*>(_v) = _t->history(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
    } else if (_c == QMetaObject::ResetProperty) {
    } else if (_c == QMetaObject::BindableProperty) {
    }
#endif // QT_NO_PROPERTIES
}

const QMetaObject tux_ti83::UIController::staticMetaObject = { {
    QMetaObject::SuperData::link<QObject::staticMetaObject>(),
    qt_meta_stringdata_tux_ti83__UIController.offsetsAndSize,
    qt_meta_data_tux_ti83__UIController,
    qt_static_metacall,
    nullptr,
qt_incomplete_metaTypeArray<qt_meta_stringdata_tux_ti83__UIController_t
, QtPrivate::TypeAndForceComplete<QString, std::true_type>, QtPrivate::TypeAndForceComplete<QStringList, std::true_type>, QtPrivate::TypeAndForceComplete<UIController, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>

, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>

>,
    nullptr
} };


const QMetaObject *tux_ti83::UIController::metaObject() const
{
    return QObject::d_ptr->metaObject ? QObject::d_ptr->dynamicMetaObject() : &staticMetaObject;
}

void *tux_ti83::UIController::qt_metacast(const char *_clname)
{
    if (!_clname) return nullptr;
    if (!strcmp(_clname, qt_meta_stringdata_tux_ti83__UIController.stringdata0))
        return static_cast<void*>(this);
    return QObject::qt_metacast(_clname);
}

int tux_ti83::UIController::qt_metacall(QMetaObject::Call _c, int _id, void **_a)
{
    _id = QObject::qt_metacall(_c, _id, _a);
    if (_id < 0)
        return _id;
    if (_c == QMetaObject::InvokeMetaMethod) {
        if (_id < 4)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 4;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 4)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 4;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 2;
    }
#endif // QT_NO_PROPERTIES
    return _id;
}

// SIGNAL 0
void tux_ti83::UIController::displayChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 0, nullptr);
}

// SIGNAL 1
void tux_ti83::UIController::historyChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
