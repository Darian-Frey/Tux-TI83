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
    const uint offsetsAndSize[32];
    char stringdata0[158];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_tux_ti83__UIController_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_tux_ti83__UIController_t qt_meta_stringdata_tux_ti83__UIController = {
    {
QT_MOC_LITERAL(0, 22), // "tux_ti83::UIController"
QT_MOC_LITERAL(23, 14), // "displayChanged"
QT_MOC_LITERAL(38, 0), // ""
QT_MOC_LITERAL(39, 18), // "graphPointsChanged"
QT_MOC_LITERAL(58, 15), // "viewPortChanged"
QT_MOC_LITERAL(74, 12), // "processInput"
QT_MOC_LITERAL(87, 7), // "tokenId"
QT_MOC_LITERAL(95, 9), // "handlePan"
QT_MOC_LITERAL(105, 2), // "dx"
QT_MOC_LITERAL(108, 2), // "dy"
QT_MOC_LITERAL(111, 14), // "currentDisplay"
QT_MOC_LITERAL(126, 11), // "graphPoints"
QT_MOC_LITERAL(138, 4), // "xMin"
QT_MOC_LITERAL(143, 4), // "xMax"
QT_MOC_LITERAL(148, 4), // "yMin"
QT_MOC_LITERAL(153, 4) // "yMax"

    },
    "tux_ti83::UIController\0displayChanged\0"
    "\0graphPointsChanged\0viewPortChanged\0"
    "processInput\0tokenId\0handlePan\0dx\0dy\0"
    "currentDisplay\0graphPoints\0xMin\0xMax\0"
    "yMin\0yMax"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_tux_ti83__UIController[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
       5,   14, // methods
       6,   55, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       3,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   44,    2, 0x06,    7 /* Public */,
       3,    0,   45,    2, 0x06,    8 /* Public */,
       4,    0,   46,    2, 0x06,    9 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       5,    1,   47,    2, 0x02,   10 /* Public */,
       7,    2,   50,    2, 0x02,   12 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::Int,    6,
    QMetaType::Void, QMetaType::Double, QMetaType::Double,    8,    9,

 // properties: name, type, flags
      10, QMetaType::QString, 0x00015001, uint(0), 0,
      11, QMetaType::QVariantList, 0x00015001, uint(1), 0,
      12, QMetaType::Double, 0x00015001, uint(2), 0,
      13, QMetaType::Double, 0x00015001, uint(2), 0,
      14, QMetaType::Double, 0x00015001, uint(2), 0,
      15, QMetaType::Double, 0x00015001, uint(2), 0,

       0        // eod
};

void tux_ti83::UIController::qt_static_metacall(QObject *_o, QMetaObject::Call _c, int _id, void **_a)
{
    if (_c == QMetaObject::InvokeMetaMethod) {
        auto *_t = static_cast<UIController *>(_o);
        (void)_t;
        switch (_id) {
        case 0: _t->displayChanged(); break;
        case 1: _t->graphPointsChanged(); break;
        case 2: _t->viewPortChanged(); break;
        case 3: _t->processInput((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 4: _t->handlePan((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2]))); break;
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
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UIController::graphPointsChanged)) {
                *result = 1;
                return;
            }
        }
        {
            using _t = void (UIController::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UIController::viewPortChanged)) {
                *result = 2;
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
        case 1: *reinterpret_cast< QVariantList*>(_v) = _t->graphPoints(); break;
        case 2: *reinterpret_cast< double*>(_v) = _t->xMin(); break;
        case 3: *reinterpret_cast< double*>(_v) = _t->xMax(); break;
        case 4: *reinterpret_cast< double*>(_v) = _t->yMin(); break;
        case 5: *reinterpret_cast< double*>(_v) = _t->yMax(); break;
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
, QtPrivate::TypeAndForceComplete<QString, std::true_type>, QtPrivate::TypeAndForceComplete<QVariantList, std::true_type>, QtPrivate::TypeAndForceComplete<double, std::true_type>, QtPrivate::TypeAndForceComplete<double, std::true_type>, QtPrivate::TypeAndForceComplete<double, std::true_type>, QtPrivate::TypeAndForceComplete<double, std::true_type>, QtPrivate::TypeAndForceComplete<UIController, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>

, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>

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
        if (_id < 5)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 5;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 5)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 5;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 6;
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
void tux_ti83::UIController::graphPointsChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 1, nullptr);
}

// SIGNAL 2
void tux_ti83::UIController::viewPortChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
