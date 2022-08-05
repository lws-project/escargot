#if defined(ESCARGOT_ENABLE_TEMPORAL)
/*
 * Copyright (c) 2022-present Samsung Electronics Co., Ltd
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

#ifndef __EscargotTemporal__
#define __EscargotTemporal__

#include "Escargot.h"
#include "intl/Intl.h"
#include "runtime/VMInstance.h"

namespace Escargot {

class Temporal : public DerivedObject {
public:
    explicit Temporal(ExecutionState& state, Object* proto)
        : DerivedObject(state, proto, ESCARGOT_OBJECT_BUILTIN_PROPERTY_NUMBER)
    {
    }

    Temporal(ExecutionState& state, Object* temporalCalendar, Object* temporalCalendarPrototype, Object* temporalDurationPrototype, Object* temporalPlainDatePrototype, Object* temporalPlainTimePrototype, Object* temporalPlainDateTimePrototype, Object* temporalPlainYearMonthPrototype, Object* temporalInstantPrototype)
        : DerivedObject(state)
        , m_temporalCalendar(temporalCalendar)
        , m_temporalCalendarPrototype(temporalCalendarPrototype)
        , m_temporalDurationPrototype(temporalDurationPrototype)
        , m_temporalPlainDatePrototype(temporalPlainDatePrototype)
        , m_temporalPlainTimePrototype(temporalPlainTimePrototype)
        , m_temporalPlainDateTimePrototype(temporalPlainDateTimePrototype)
        , m_temporalPlainYearMonthPrototype(temporalPlainYearMonthPrototype)
        , m_temporalInstantPrototype(temporalInstantPrototype)
    {
    }

    Object* getTemporalCalendar() const
    {
        return m_temporalCalendar;
    }

    Object* getTemporalCalendarPrototype() const
    {
        return m_temporalCalendarPrototype;
    }

    Object* getTemporalDurationPrototype() const
    {
        return m_temporalDurationPrototype;
    }

    Object* getTemporalPlainDatePrototype() const
    {
        return m_temporalPlainDatePrototype;
    }

    Object* getTemporalPlainTimePrototype() const
    {
        return m_temporalPlainTimePrototype;
    }

    Object* getTemporalPlainDateTimePrototype() const
    {
        return m_temporalPlainDateTimePrototype;
    }

    Object* getTemporalPlainYearMonthPrototype() const
    {
        return m_temporalPlainYearMonthPrototype;
    }

    Object* getTemporalInstantPrototype() const
    {
        return m_temporalInstantPrototype;
    }

    bool isTemporalObject() const override
    {
        return true;
    }

    static Value getOptionsObject(ExecutionState& state, const Value& options)
    {
        if (options.isObject()) {
            return options;
        }

        if (!options.isUndefined()) {
            ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, "options must be object");
        }

        return {};
    }

    static Value toTemporalOverflow(ExecutionState& state, const Value& normalizedOptions)
    {
        if (normalizedOptions.isUndefined()) {
            return state.context()->staticStrings().lazyConstrain().string();
        }
        auto options = normalizedOptions.toObject(state);
        Value matcherValues[2] = { state.context()->staticStrings().lazyConstrain().string(), state.context()->staticStrings().reject.string() };
        return Intl::getOption(state, options, state.context()->staticStrings().lazyoverflow().string(), Intl::StringValue, matcherValues, 2, matcherValues[0]);
    }

    static Value prepareTemporalFields(ExecutionState& state, const Value& fields, const ValueVector& fieldNames, const ValueVector& requiredFields)
    {
        ASSERT(fields.isObject());
        auto* result = new Object(state);
        for (auto property : fieldNames) {
            Value value = fields.asObject()->get(state, ObjectPropertyName(state, fields), property).value(state, Value());
            String* prop = property.asString();
            if (value.isUndefined()) {
                if (std::find(requiredFields.begin(), requiredFields.end(), property) != requiredFields.end()) {
                    ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, new ASCIIString("requiredFields contains property"));
                }
                if (prop->equals("year") || prop->equals("month") || prop->equals("monthCode") || prop->equals("day") || prop->equals("offset") || prop->equals("era") || prop->equals("eraYear") || prop->equals("timeZone")) {
                    value = Value();
                } else {
                    value = Value(0);
                }
            } else {
                if (!prop->equals("timeZone")) {
                    if (prop->equals("monthCode") || prop->equals("offset") || prop->equals("era")) {
                        value = Value(value.toString(state));
                    } else if (prop->equals("month") || prop->equals("day")) {
                        value = Value(value.toUint32(state));
                    } else {
                        value = Value(value.toInteger(state));
                    }
                }
            }
            result->defineOwnProperty(state, ObjectPropertyName(AtomicString(state, property.asString())), ObjectPropertyDescriptor(value, ObjectPropertyDescriptor::AllPresent));
        }
        return Value(result);
    }

    static void rejectObjectWithCalendarOrTimeZone(ExecutionState& state, const Value& object)
    {
        ASSERT(object.isObject());
        if (object.asObject()->isTemporalPlainDateObject() || object.asObject()->isTemporalPlainDateTimeObject() || object.asObject()->isTemporalPlainTimeObject() || object.asObject()->isTemporalZonedDateTimeObject() || object.asObject()->isTemporalPlainYearMonthObject() || object.asObject()->isTemporalMonthDayObject()) {
            ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, new ASCIIString("Invalid type of Object"));
        }

        if (!object.asObject()->get(state, ObjectPropertyName(state, state.context()->staticStrings().calendar.string())).value(state, object).isUndefined()) {
            ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, new ASCIIString("Object has calendar property"));
        }

        if (!object.asObject()->get(state, ObjectPropertyName(state, state.context()->staticStrings().lazytimeZone().string())).value(state, object).isUndefined()) {
            ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, new ASCIIString("Object has timezone property"));
        }
    }

private:
    Object* m_temporalCalendar;
    Object* m_temporalCalendarPrototype;
    Object* m_temporalDurationPrototype;
    Object* m_temporalPlainDatePrototype;
    Object* m_temporalPlainTimePrototype;
    Object* m_temporalPlainDateTimePrototype;
    Object* m_temporalPlainYearMonthPrototype;
    Object* m_temporalInstantPrototype;
};

} // namespace Escargot

#endif
#endif
