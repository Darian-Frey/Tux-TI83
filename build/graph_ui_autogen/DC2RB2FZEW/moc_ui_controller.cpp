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
    const uint offsetsAndSize[76];
    char stringdata0[365];
};
#define QT_MOC_LITERAL(ofs, len) \
    uint(offsetof(qt_meta_stringdata_tux_ti83__UIController_t, stringdata0) + ofs), len 
static const qt_meta_stringdata_tux_ti83__UIController_t qt_meta_stringdata_tux_ti83__UIController = {
    {
QT_MOC_LITERAL(0, 22), // "tux_ti83::UIController"
QT_MOC_LITERAL(23, 14), // "displayChanged"
QT_MOC_LITERAL(38, 0), // ""
QT_MOC_LITERAL(39, 14), // "historyChanged"
QT_MOC_LITERAL(54, 26), // "activeFunctionIndexChanged"
QT_MOC_LITERAL(81, 15), // "viewportChanged"
QT_MOC_LITERAL(97, 16), // "graphModeChanged"
QT_MOC_LITERAL(114, 12), // "processInput"
QT_MOC_LITERAL(127, 5), // "input"
QT_MOC_LITERAL(133, 17), // "setActiveFunction"
QT_MOC_LITERAL(151, 5), // "index"
QT_MOC_LITERAL(157, 15), // "toggleGraphMode"
QT_MOC_LITERAL(173, 13), // "resetViewport"
QT_MOC_LITERAL(187, 7), // "zoomFit"
QT_MOC_LITERAL(195, 12), // "updateMatrix"
QT_MOC_LITERAL(208, 4), // "name"
QT_MOC_LITERAL(213, 4), // "rows"
QT_MOC_LITERAL(218, 4), // "cols"
QT_MOC_LITERAL(223, 6), // "values"
QT_MOC_LITERAL(230, 19), // "getMultiGraphPoints"
QT_MOC_LITERAL(250, 10), // "resolution"
QT_MOC_LITERAL(261, 3), // "pan"
QT_MOC_LITERAL(265, 2), // "dx"
QT_MOC_LITERAL(268, 2), // "dy"
QT_MOC_LITERAL(271, 2), // "vw"
QT_MOC_LITERAL(274, 2), // "vh"
QT_MOC_LITERAL(277, 4), // "zoom"
QT_MOC_LITERAL(282, 1), // "f"
QT_MOC_LITERAL(284, 2), // "mx"
QT_MOC_LITERAL(287, 2), // "my"
QT_MOC_LITERAL(290, 4), // "xMin"
QT_MOC_LITERAL(295, 4), // "xMax"
QT_MOC_LITERAL(300, 4), // "yMin"
QT_MOC_LITERAL(305, 4), // "yMax"
QT_MOC_LITERAL(310, 14), // "currentDisplay"
QT_MOC_LITERAL(325, 7), // "history"
QT_MOC_LITERAL(333, 19), // "activeFunctionIndex"
QT_MOC_LITERAL(353, 11) // "isGraphMode"

    },
    "tux_ti83::UIController\0displayChanged\0"
    "\0historyChanged\0activeFunctionIndexChanged\0"
    "viewportChanged\0graphModeChanged\0"
    "processInput\0input\0setActiveFunction\0"
    "index\0toggleGraphMode\0resetViewport\0"
    "zoomFit\0updateMatrix\0name\0rows\0cols\0"
    "values\0getMultiGraphPoints\0resolution\0"
    "pan\0dx\0dy\0vw\0vh\0zoom\0f\0mx\0my\0xMin\0"
    "xMax\0yMin\0yMax\0currentDisplay\0history\0"
    "activeFunctionIndex\0isGraphMode"
};
#undef QT_MOC_LITERAL

static const uint qt_meta_data_tux_ti83__UIController[] = {

 // content:
      10,       // revision
       0,       // classname
       0,    0, // classinfo
      14,   14, // methods
       8,  144, // properties
       0,    0, // enums/sets
       0,    0, // constructors
       0,       // flags
       5,       // signalCount

 // signals: name, argc, parameters, tag, flags, initial metatype offsets
       1,    0,   98,    2, 0x06,    9 /* Public */,
       3,    0,   99,    2, 0x06,   10 /* Public */,
       4,    0,  100,    2, 0x06,   11 /* Public */,
       5,    0,  101,    2, 0x06,   12 /* Public */,
       6,    0,  102,    2, 0x06,   13 /* Public */,

 // methods: name, argc, parameters, tag, flags, initial metatype offsets
       7,    1,  103,    2, 0x02,   14 /* Public */,
       9,    1,  106,    2, 0x02,   16 /* Public */,
      11,    0,  109,    2, 0x02,   18 /* Public */,
      12,    0,  110,    2, 0x02,   19 /* Public */,
      13,    0,  111,    2, 0x02,   20 /* Public */,
      14,    4,  112,    2, 0x02,   21 /* Public */,
      19,    1,  121,    2, 0x02,   26 /* Public */,
      21,    4,  124,    2, 0x02,   28 /* Public */,
      26,    5,  133,    2, 0x02,   33 /* Public */,

 // signals: parameters
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,

 // methods: parameters
    QMetaType::Void, QMetaType::QString,    8,
    QMetaType::Void, QMetaType::Int,   10,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void,
    QMetaType::Void, QMetaType::QString, QMetaType::Int, QMetaType::Int, QMetaType::QVariantList,   15,   16,   17,   18,
    QMetaType::QVariantList, QMetaType::Int,   20,
    QMetaType::Void, QMetaType::Double, QMetaType::Double, QMetaType::Double, QMetaType::Double,   22,   23,   24,   25,
    QMetaType::Void, QMetaType::Double, QMetaType::Double, QMetaType::Double, QMetaType::Double, QMetaType::Double,   27,   28,   29,   24,   25,

 // properties: name, type, flags
      30, QMetaType::Double, 0x00015003, uint(3), 0,
      31, QMetaType::Double, 0x00015003, uint(3), 0,
      32, QMetaType::Double, 0x00015003, uint(3), 0,
      33, QMetaType::Double, 0x00015003, uint(3), 0,
      34, QMetaType::QString, 0x00015001, uint(0), 0,
      35, QMetaType::QStringList, 0x00015001, uint(1), 0,
      36, QMetaType::Int, 0x00015001, uint(2), 0,
      37, QMetaType::Bool, 0x00015003, uint(4), 0,

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
        case 2: _t->activeFunctionIndexChanged(); break;
        case 3: _t->viewportChanged(); break;
        case 4: _t->graphModeChanged(); break;
        case 5: _t->processInput((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1]))); break;
        case 6: _t->setActiveFunction((*reinterpret_cast< std::add_pointer_t<int>>(_a[1]))); break;
        case 7: _t->toggleGraphMode(); break;
        case 8: _t->resetViewport(); break;
        case 9: _t->zoomFit(); break;
        case 10: _t->updateMatrix((*reinterpret_cast< std::add_pointer_t<QString>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<int>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<QVariantList>>(_a[4]))); break;
        case 11: { QVariantList _r = _t->getMultiGraphPoints((*reinterpret_cast< std::add_pointer_t<int>>(_a[1])));
            if (_a[0]) *reinterpret_cast< QVariantList*>(_a[0]) = std::move(_r); }  break;
        case 12: _t->pan((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[4]))); break;
        case 13: _t->zoom((*reinterpret_cast< std::add_pointer_t<double>>(_a[1])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[2])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[3])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[4])),(*reinterpret_cast< std::add_pointer_t<double>>(_a[5]))); break;
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
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UIController::activeFunctionIndexChanged)) {
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
        {
            using _t = void (UIController::*)();
            if (*reinterpret_cast<_t *>(_a[1]) == static_cast<_t>(&UIController::graphModeChanged)) {
                *result = 4;
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
        case 0: *reinterpret_cast< double*>(_v) = _t->m_xMin; break;
        case 1: *reinterpret_cast< double*>(_v) = _t->m_xMax; break;
        case 2: *reinterpret_cast< double*>(_v) = _t->m_yMin; break;
        case 3: *reinterpret_cast< double*>(_v) = _t->m_yMax; break;
        case 4: *reinterpret_cast< QString*>(_v) = _t->currentDisplay(); break;
        case 5: *reinterpret_cast< QStringList*>(_v) = _t->history(); break;
        case 6: *reinterpret_cast< int*>(_v) = _t->activeFunctionIndex(); break;
        case 7: *reinterpret_cast< bool*>(_v) = _t->m_isGraphMode; break;
        default: break;
        }
    } else if (_c == QMetaObject::WriteProperty) {
        auto *_t = static_cast<UIController *>(_o);
        (void)_t;
        void *_v = _a[0];
        switch (_id) {
        case 0:
            if (_t->m_xMin != *reinterpret_cast< double*>(_v)) {
                _t->m_xMin = *reinterpret_cast< double*>(_v);
                Q_EMIT _t->viewportChanged();
            }
            break;
        case 1:
            if (_t->m_xMax != *reinterpret_cast< double*>(_v)) {
                _t->m_xMax = *reinterpret_cast< double*>(_v);
                Q_EMIT _t->viewportChanged();
            }
            break;
        case 2:
            if (_t->m_yMin != *reinterpret_cast< double*>(_v)) {
                _t->m_yMin = *reinterpret_cast< double*>(_v);
                Q_EMIT _t->viewportChanged();
            }
            break;
        case 3:
            if (_t->m_yMax != *reinterpret_cast< double*>(_v)) {
                _t->m_yMax = *reinterpret_cast< double*>(_v);
                Q_EMIT _t->viewportChanged();
            }
            break;
        case 7:
            if (_t->m_isGraphMode != *reinterpret_cast< bool*>(_v)) {
                _t->m_isGraphMode = *reinterpret_cast< bool*>(_v);
                Q_EMIT _t->graphModeChanged();
            }
            break;
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
, QtPrivate::TypeAndForceComplete<double, std::true_type>, QtPrivate::TypeAndForceComplete<double, std::true_type>, QtPrivate::TypeAndForceComplete<double, std::true_type>, QtPrivate::TypeAndForceComplete<double, std::true_type>, QtPrivate::TypeAndForceComplete<QString, std::true_type>, QtPrivate::TypeAndForceComplete<QStringList, std::true_type>, QtPrivate::TypeAndForceComplete<int, std::true_type>, QtPrivate::TypeAndForceComplete<bool, std::true_type>, QtPrivate::TypeAndForceComplete<UIController, std::true_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>

, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<const QString &, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<const QVariantList &, std::false_type>, QtPrivate::TypeAndForceComplete<QVariantList, std::false_type>, QtPrivate::TypeAndForceComplete<int, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<void, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>, QtPrivate::TypeAndForceComplete<double, std::false_type>

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
        if (_id < 14)
            qt_static_metacall(this, _c, _id, _a);
        _id -= 14;
    } else if (_c == QMetaObject::RegisterMethodArgumentMetaType) {
        if (_id < 14)
            *reinterpret_cast<QMetaType *>(_a[0]) = QMetaType();
        _id -= 14;
    }
#ifndef QT_NO_PROPERTIES
    else if (_c == QMetaObject::ReadProperty || _c == QMetaObject::WriteProperty
            || _c == QMetaObject::ResetProperty || _c == QMetaObject::BindableProperty
            || _c == QMetaObject::RegisterPropertyMetaType) {
        qt_static_metacall(this, _c, _id, _a);
        _id -= 8;
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
void tux_ti83::UIController::activeFunctionIndexChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 2, nullptr);
}

// SIGNAL 3
void tux_ti83::UIController::viewportChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 3, nullptr);
}

// SIGNAL 4
void tux_ti83::UIController::graphModeChanged()
{
    QMetaObject::activate(this, &staticMetaObject, 4, nullptr);
}
QT_WARNING_POP
QT_END_MOC_NAMESPACE
