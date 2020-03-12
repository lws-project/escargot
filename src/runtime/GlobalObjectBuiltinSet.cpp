/*
 * Copyright (c) 2018-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 *  USA
 */

#include "Escargot.h"
#include "GlobalObject.h"
#include "Context.h"
#include "VMInstance.h"
#include "SetObject.h"
#include "IteratorObject.h"
#include "NativeFunctionObject.h"
#include "ToStringRecursionPreventer.h"

namespace Escargot {

Value builtinSetConstructor(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Value newTarget)
{
    // If NewTarget is undefined, throw a TypeError exception.
    if (newTarget.isUndefined()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, errorMessage_GlobalObject_ConstructorRequiresNew);
    }

    // Let set be ? OrdinaryCreateFromConstructor(NewTarget, "%SetPrototype%", « [[SetData]] »).
    // Set set's [[SetData]] internal slot to a new empty List.
    SetObject* set = new SetObject(state);

    // If iterable is not present, let iterable be undefined.
    Value iterable;
    if (argc >= 1) {
        iterable = argv[0];
    }

    // If iterable is either undefined or null, return set.
    if (iterable.isUndefinedOrNull()) {
        return set;
    }

    // Let adder be ? Get(set, "add").
    Value adder = set->get(state, ObjectPropertyName(state.context()->staticStrings().add)).value(state, set);
    // If IsCallable(adder) is false, throw a TypeError exception.
    if (!adder.isCallable()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, errorMessage_NOT_Callable);
    }
    // Let iteratorRecord be ? GetIterator(iterable).
    auto iteratorRecord = IteratorObject::getIterator(state, iterable);

    // Repeat
    while (true) {
        // Let next be ? IteratorStep(iteratorRecord).
        auto next = IteratorObject::iteratorStep(state, iteratorRecord);
        // If next is false, return set.
        if (!next.hasValue()) {
            return set;
        }
        // Let nextValue be ? IteratorValue(next).
        Value nextValue = IteratorObject::iteratorValue(state, next.value());

        // Let status be Call(adder, set, « nextValue »).
        try {
            Value argv[1] = { nextValue };
            Object::call(state, adder, set, 1, argv);
        } catch (const Value& v) {
            // we should save thrown value bdwgc cannot track thrown value
            Value exceptionValue = v;
            // If status is an abrupt completion, return ? IteratorClose(iteratorRecord, status).
            return IteratorObject::iteratorClose(state, iteratorRecord, exceptionValue, true);
        }
    }

    return set;
}

#define RESOLVE_THIS_BINDING_TO_SET(NAME, OBJ, BUILT_IN_METHOD)                                                                                                                                                                                \
    if (!thisValue.isObject() || !thisValue.asObject()->isSetObject() || thisValue.asObject()->isSetPrototypeObject()) {                                                                                                                       \
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().OBJ.string(), true, state.context()->staticStrings().BUILT_IN_METHOD.string(), errorMessage_GlobalObject_CalledOnIncompatibleReceiver); \
    }                                                                                                                                                                                                                                          \
    SetObject* NAME = thisValue.asObject()->asSetObject();

static Value builtinSetAdd(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Value newTarget)
{
    RESOLVE_THIS_BINDING_TO_SET(S, Set, add);
    S->add(state, argv[0]);
    return S;
}

static Value builtinSetClear(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Value newTarget)
{
    RESOLVE_THIS_BINDING_TO_SET(S, Set, clear);
    S->clear(state);
    return Value();
}

static Value builtinSetDelete(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Value newTarget)
{
    RESOLVE_THIS_BINDING_TO_SET(S, Set, stringDelete);
    return Value(S->deleteOperation(state, argv[0]));
}

static Value builtinSetHas(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Value newTarget)
{
    RESOLVE_THIS_BINDING_TO_SET(S, Set, has);
    return Value(S->has(state, argv[0]));
}

static Value builtinSetForEach(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Value newTarget)
{
    // Let S be the this value.
    // If Type(S) is not Object, throw a TypeError exception.
    // If S does not have a [[SetData]] internal slot, throw a TypeError exception.
    RESOLVE_THIS_BINDING_TO_SET(S, Set, forEach);

    Value callbackfn = argv[0];
    // If IsCallable(callbackfn) is false, throw a TypeError exception.
    if (!callbackfn.isCallable()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().Set.string(), true, state.context()->staticStrings().forEach.string(), errorMessage_GlobalObject_CallbackNotCallable);
    }
    // If thisArg was supplied, let T be thisArg; else let T be undefined.
    Value T;
    if (argc >= 2) {
        T = argv[1];
    }
    // Let entries be the List that is the value of S's [[SetData]] internal slot.
    const SetObject::SetObjectData& entries = S->storage();
    // Repeat for each e that is an element of entries, in original insertion order
    for (size_t i = 0; i < entries.size(); i++) {
        Value e = entries[i];
        // If e is not empty, then
        if (!e.isEmpty()) {
            // If e.[[Key]] is not empty, then
            // Perform ? Call(callbackfn, T, « e, e, S »).
            Value argv[3] = { Value(e), Value(e), Value(S) };
            Object::call(state, callbackfn, T, 3, argv);
        }
    }

    return Value();
}

static Value builtinSetValues(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Value newTarget)
{
    RESOLVE_THIS_BINDING_TO_SET(S, Set, values);
    return S->values(state);
}

static Value builtinSetEntries(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Value newTarget)
{
    RESOLVE_THIS_BINDING_TO_SET(S, Set, entries);
    return S->entries(state);
}

static Value builtinSetSizeGetter(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Value newTarget)
{
    RESOLVE_THIS_BINDING_TO_SET(S, Set, size);
    return Value(S->size(state));
}

static Value builtinSetIteratorNext(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Value newTarget)
{
    if (!thisValue.isObject() || !thisValue.asObject()->isIteratorObject() || !thisValue.asObject()->asIteratorObject()->isSetIteratorObject()) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, state.context()->staticStrings().SetIterator.string(), true, state.context()->staticStrings().next.string(), errorMessage_GlobalObject_CalledOnIncompatibleReceiver);
    }
    SetIteratorObject* iter = thisValue.asObject()->asIteratorObject()->asSetIteratorObject();
    return iter->next(state);
}

void GlobalObject::installSet(ExecutionState& state)
{
    m_set = new NativeFunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().Set, builtinSetConstructor, 0), NativeFunctionObject::__ForBuiltinConstructor__);
    m_set->markThisObjectDontNeedStructureTransitionTable();
    m_set->setPrototype(state, m_functionPrototype);

    {
        JSGetterSetter gs(
            new NativeFunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().getSymbolSpecies, builtinSpeciesGetter, 0, NativeFunctionInfo::Strict)), Value(Value::EmptyValue));
        ObjectPropertyDescriptor desc(gs, ObjectPropertyDescriptor::ConfigurablePresent);
        m_set->defineOwnProperty(state, ObjectPropertyName(state.context()->vmInstance()->globalSymbols().species), desc);
    }

    m_setPrototype = m_objectPrototype;
    m_setPrototype = new SetPrototypeObject(state);
    m_setPrototype->markThisObjectDontNeedStructureTransitionTable();
    m_setPrototype->defineOwnProperty(state, ObjectPropertyName(state.context()->staticStrings().constructor), ObjectPropertyDescriptor(m_set, (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    m_setPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->staticStrings().clear),
                                                     ObjectPropertyDescriptor(new NativeFunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().clear, builtinSetClear, 0, NativeFunctionInfo::Strict)), (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    m_setPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->staticStrings().stringDelete),
                                                     ObjectPropertyDescriptor(new NativeFunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().stringDelete, builtinSetDelete, 1, NativeFunctionInfo::Strict)), (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    m_setPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->staticStrings().has),
                                                     ObjectPropertyDescriptor(new NativeFunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().has, builtinSetHas, 1, NativeFunctionInfo::Strict)), (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    m_setPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->staticStrings().add),
                                                     ObjectPropertyDescriptor(new NativeFunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().add, builtinSetAdd, 1, NativeFunctionInfo::Strict)), (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    m_setPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->staticStrings().forEach),
                                                     ObjectPropertyDescriptor(new NativeFunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().forEach, builtinSetForEach, 1, NativeFunctionInfo::Strict)), (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    auto valuesFn = new NativeFunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().values, builtinSetValues, 0, NativeFunctionInfo::Strict));
    auto values = ObjectPropertyDescriptor(valuesFn, (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent));
    m_setPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->staticStrings().values), values);
    m_setPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->staticStrings().keys), values);

    m_setPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->staticStrings().entries),
                                                     ObjectPropertyDescriptor(new NativeFunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().entries, builtinSetEntries, 0, NativeFunctionInfo::Strict)), (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    m_setPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->vmInstance()->globalSymbols().iterator),
                                                     ObjectPropertyDescriptor(valuesFn, (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    m_setPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->vmInstance()->globalSymbols().toStringTag),
                                                     ObjectPropertyDescriptor(Value(state.context()->staticStrings().Set.string()), (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::ConfigurablePresent)));
    JSGetterSetter gs(
        new NativeFunctionObject(state, NativeFunctionInfo(AtomicString(state, String::fromASCII("get size")), builtinSetSizeGetter, 0, NativeFunctionInfo::Strict)),
        Value(Value::EmptyValue));
    ObjectPropertyDescriptor desc(gs, ObjectPropertyDescriptor::ConfigurablePresent);
    m_setPrototype->defineOwnProperty(state, ObjectPropertyName(state.context()->staticStrings().size), desc);

    m_setIteratorPrototype = m_iteratorPrototype;
    m_setIteratorPrototype = new SetIteratorObject(state, nullptr, SetIteratorObject::TypeKey);

    m_setIteratorPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->staticStrings().next),
                                                             ObjectPropertyDescriptor(new NativeFunctionObject(state, NativeFunctionInfo(state.context()->staticStrings().next, builtinSetIteratorNext, 0, NativeFunctionInfo::Strict)), (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));

    m_setIteratorPrototype->defineOwnPropertyThrowsException(state, ObjectPropertyName(state.context()->vmInstance()->globalSymbols().toStringTag),
                                                             ObjectPropertyDescriptor(Value(String::fromASCII("Set Iterator")), (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::ConfigurablePresent)));

    m_set->setFunctionPrototype(state, m_setPrototype);
    defineOwnProperty(state, ObjectPropertyName(state.context()->staticStrings().Set),
                      ObjectPropertyDescriptor(m_set, (ObjectPropertyDescriptor::PresentAttribute)(ObjectPropertyDescriptor::WritablePresent | ObjectPropertyDescriptor::ConfigurablePresent)));
}
} // namespace Escargot
