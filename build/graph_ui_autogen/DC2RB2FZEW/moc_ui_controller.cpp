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
    const uint offsetsAndSize[58];
    char stringdata0[287];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_tux_ti83__UIController_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_tux_ti83__UIController_t qt_meta_stringdata_tux_ti83__UIController = {
    {
QT_MOC_LITERAL(0, 22), // "tux_ti83::UIController"
QT_MOC_LITERAL(23, 14), // "displayChanged"
QT_MOC_LITERAL(38, 0), // ""
QT_MOC_LITERAL(39, 14), // "historyChanged"
QT_MOC_LITERAL(54, 16), // "graphModeChanged"
QT_MOC_LITERAL(71, 15), // "viewportChanged"
QT_MOC_LITERAL(87, 12), // "processInput"
QT_MOC_LITERAL(100, 7), // "tokenId"
QT_MOC_LITERAL(108, 18), // "restoreFromHistory"
QT_MOC_LITERAL(127, 5), // "index"
QT_MOC_LITERAL(133, 14), // "getGraphPoints"
QT_MOC_LITERAL(148, 10), // "resolution"
QT_MOC_LITERAL(159, 15), // "toggleGraphMode"
QT_MOC_LITERAL(175, 3), // "pan"
QT_MOC_LITERAL(179, 2), // "dx"
QT_MOC_LITERAL(182, 2), // "dy"
QT_MOC_LITERAL(185, 9), // "viewWidth"
QT_MOC_LITERAL(195, 10), // "viewHeight"
QT_MOC_LITERAL(206, 4), // "zoom"
QT_MOC_LITERAL(211, 6), // "factor"
QT_MOC_LITERAL(218, 6), // "mouseX"
QT_MOC_LITERAL(225, 6), // "mouseY"
QT_MOC_LITERAL(232, 14), // "currentDisplay"
QT_MOC_LITERAL(247, 7), // "history"
QT_MOC_LITERAL(255, 11), // "isGraphMode"
QT_MOC_LITERAL(267, 4), // "xMin"
QT_MOC_LITERAL(272, 4), // "xMax"
QT_MOC_LITERAL(277, 4), // "yMin"
QT_MOC_LITERAL(282, 4) // "yMax"

    },
    "tux_ti83::UIController\0displayChanged\0"
    "\0historyChanged\0graphModeChanged\0"
    "viewportChanged\0processInput\0tokenId\0"
    "restoreFromHistory\0index\0getGraphPoints\0"
    "resolution\0toggleGraphMode\0pan\0dx\0dy\0"
    "viewWidth\0viewHeight\0zoom\0factor\0"
    "mouseX\0mouseY\0currentDisplay\0history\0"
    "isGraphMode\0xMin\0xMax\0yMin\0yMax"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_tux_ti83__UIController[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      10,   14, // methods
       7,  108, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       4,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   74,    2, 0x06,    8 /* Public */,
       3,    0,   75,    2, 0x06,    9 /* Public */,
       4,    0,   76,    2, 0x06,   10 /* Public */,
       5,    0,   77,    2, 0x06,   11 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       6,    1,   78,    2, 0x02,   12 /* Public */,
       8,    1,   81,    2, 0x02,   14 /* Public */,
      10,    1,   84,    2, 0x02,   16 /* Public */,
      12,    0,   87,    2, 0x02,   18 /* Public */,
      13,    4,   88,    2, 0x02,   19 /* Public */,
      18,    5,   97,    2, 0x02,   24 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::Int,    7,
    QMetaType::Void, QMetaType::Int,    9,
    QMetaType::QVariantList, QMetaType::Int,   11,
    QMetaType::Void,
    QMetaType::Void, QMetaType::Double, QMetaType::Double, QMetaType::Double, QMetaType::Double,   14,   15,   16,   17,
    QMetaType::Void, QMetaType::Double, QMetaType::Double, QMetaType::Double, QMetaType::Double, QMetaType::Double,   19,   20,   21,   16,   17,

 // properties: name, type, flags
      22, QMetaType::QString, 0x00015001, uint(0), 0,
      23, QMetaType::QStringList, 0x00015001, uint(1), 0,
      24, QMetaType::Bool, 0x00015001, uint(2), 0,
      25, QMetaType::Double, 0x00015103, uint(3), 0,
      26, QMetaType::Double, 0x00015103, uint(3), 0,
      27, QMetaType::Double, 0x00015103, uint(3), 0,
      28, QMetaType::Double, 0x00015103, uint(3), 0,

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
        case 2: _t->graphModeChanged(); break;
        case 3: _t->viewportChanged(); break;
        case 4: _t->processInput((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 5: _t->restoreFromHistory((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 6: { QVariantList _r = _t->getGraphPoints((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 7: _t->toggleGraphMode(); break;
        case 8: _t->pan((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[4]))); break;
        case 9: _t->zoom((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[5]))); break;
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
        {
            using _t = void (UIController::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UIController::graphModeChanged)) {
                *result = 2;
                return;
            }
        }
        {
            using _t = void (UIController::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UIController::viewportChanged)) {
                *result = 3;
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
        case 2: *reinterpret_cast< bool*>(_v) = _t->isGraphMode(); break;
        case 3: *reinterpret_cast< double*>(_v) = _t->xMin(); break;
        case 4: *reinterpret_cast< double*>(_v) = _t->xMax(); break;
        case 5: *reinterpret_cast< double*>(_v) = _t->yMin(); break;
        case 6: *reinterpret_cast< double*>(_v) = _t->yMax(); break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<UIController *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 3: _t->setXMin(*reinterpret_cast< double*>(_v)); break;
        case 4: _t->setXMax(*reinterpret_cast< double*>(_v)); break;
        case 5: _t->setYMin(*reinterpret_cast< double*>(_v)); break;
        case 6: _t->setYMax(*reinterpret_cast< double*>(_v)); break;
        default: break;
        }
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
, QtPrivate::TypeAndForceComplete<QString, std::true_type>, QtPrivate::TypeAndForceComplete<QStringList, std::true_type>, QtPrivate::TypeAndForceComplete<bool, std::true_type>, QtPrivate::TypeAndForceComplete<double, std::true_type>, QtPrivate::TypeAndForceComplete<double, std::true_type>, QtPrivate::TypeAndForceComplete<double, std::true_type>, QtPrivate::TypeAndForceComplete<double, std::true_type>, QtPrivate::TypeAndForceComplete<UIController, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>

, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>

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
        if (_id < 10)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 10;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 10)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 10;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 7;
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

// SIGNAL 2
void tux_ti83::UIController::graphModeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void tux_ti83::UIController::viewportChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
