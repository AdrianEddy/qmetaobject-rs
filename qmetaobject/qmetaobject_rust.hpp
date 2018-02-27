#pragma once

#include <QtCore/QObject>
#include <QtCore/QEvent>
#include <QtCore/QDebug>

struct TraitObject {
    void *a;
    void *b;
};

extern "C" QMetaObject *RustObject_metaObject(TraitObject);
extern "C" void RustObject_destruct(TraitObject);

template <typename Base>
struct RustObject : Base {
    TraitObject rust_object;
    const QMetaObject *metaObject() const override {
        return RustObject_metaObject(rust_object);
    }
    int qt_metacall(QMetaObject::Call _c, int _id, void **_a) override {
        _id = Base::qt_metacall(_c, _id, _a);
        if (_id < 0)
            return _id;
        const QMetaObject *mo = metaObject();
        if (_c == QMetaObject::InvokeMetaMethod || _c == QMetaObject::RegisterMethodArgumentMetaType) {
            int methodCount = mo->methodCount();
            if (_id < methodCount)
                mo->d.static_metacall(this, _c, _id, _a);
            _id -= methodCount;
        } else if ((_c >= QMetaObject::ReadProperty && _c <= QMetaObject::QueryPropertyUser)
            || _c == QMetaObject::RegisterPropertyMetaType) {
            int propertyCount = mo->propertyCount();
            if (_id < propertyCount)
                mo->d.static_metacall(this, _c, _id, _a);
            _id -= propertyCount;
        }
        return _id;
    }
    bool event(QEvent *event) override {
        if (event->type() == 513) {
            // "513 reserved for Qt Jambi's DeleteOnMainThread event"
            // This event is sent by rust when we are deleted.
            rust_object = { nullptr, nullptr }; // so the destructor don't recurse
            delete this;
            return true;
        }
        return Base::event(event);
    }
    ~RustObject() {
        if (rust_object.a || rust_object.b)
            RustObject_destruct(rust_object);
    }
};


// Hack to access QMetaType::registerConverterFunction which is private, but ConverterFunctor
// is a friend
namespace QtPrivate {
template<>
struct ConverterFunctor<TraitObject, TraitObject, TraitObject> : public AbstractConverterFunction
{
    using AbstractConverterFunction::AbstractConverterFunction;
    bool registerConverter(int from, int to) {
        return QMetaType::registerConverterFunction(this, from, to);
    }
};
}