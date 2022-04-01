/*
 * Copyright (c) 2020-present Samsung Electronics Co., Ltd
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

#ifndef __EscargotTemplate__
#define __EscargotTemplate__

#include "util/Vector.h"
#include "runtime/Value.h"
#include "runtime/ObjectStructure.h"

namespace Escargot {

class Template;
class ObjectTemplate;
class FunctionTemplate;

class TemplateData : public gc {
    friend class Template;

public:
    enum TemplateDataType {
        TemplateDataValue,
        TemplateDataTemplate,
    };

    TemplateData(const Value& v)
    {
        m_type = TemplateDataValue;
        m_value = v;
    }

    TemplateData(Template* t)
    {
        m_type = TemplateDataTemplate;
        m_template = t;
    }

private:
    TemplateDataType m_type;
    union {
        EncodedValue m_value;
        Template* m_template;
    };
};

// don't modify template after create object
// it is not intented operation
// Note) only String or Symbol type is allowed for `propertyName`
// otherwise, it will abort
class Template : public gc {
public:
    void set(const Value& propertyName, const TemplateData& data, bool isWritable, bool isEnumerable, bool isConfigurable);
    void setAccessorProperty(const Value& propertyName, Optional<FunctionTemplate*> getter, Optional<FunctionTemplate*> setter, bool isEnumerable, bool isConfigurable);
    void setNativeDataAccessorProperty(const Value& propertyName, ObjectPropertyNativeGetterSetterData* nativeGetterSetterData, void* privateData);

    bool has(const Value& propertyName);
    // return true if removed
    bool remove(const Value& propertyName);

    void setInstanceExtraData(void* ptr)
    {
        m_instanceExtraData = ptr;
    }
    void* instanceExtraData()
    {
        return m_instanceExtraData;
    }

    virtual Object* instantiate(Context* ctx) = 0;
    bool didInstantiate() const
    {
        return m_cachedObjectStructure.m_objectStructure;
    }

    virtual bool isObjectTemplate() const
    {
        return false;
    }

    virtual bool isFunctionTemplate() const
    {
        return false;
    }

    Optional<ObjectStructure*> cachedObjectStructure()
    {
        return m_cachedObjectStructure.m_objectStructure;
    }

protected:
    struct CachedObjectStructure {
        ObjectStructure* m_objectStructure;
        bool m_inlineCacheable;

        CachedObjectStructure()
            : m_objectStructure(nullptr)
            , m_inlineCacheable(false)
        {
        }
    };

    void addNativeDataAccessorProperties(Template* other);
    CachedObjectStructure constructObjectStructure(Context* ctx, ObjectStructureItem* baseItems, size_t baseItemCount);
    void constructObjectPropertyValues(Context* ctx, ObjectPropertyValue* baseItems, size_t baseItemCount, ObjectPropertyValueVector& objectPropertyValues);
    void postProcessing(Object* instantiatedObject);

    Template()
        : m_instanceExtraData(nullptr)
    {
    }
    virtual ~Template()
    {
    }

    static inline void fillGCDescriptor(GC_word* desc)
    {
        GC_set_bit(desc, GC_WORD_OFFSET(Template, m_properties));
        GC_set_bit(desc, GC_WORD_OFFSET(Template, m_instanceExtraData));
        GC_set_bit(desc, GC_WORD_OFFSET(Template, m_cachedObjectStructure.m_objectStructure));
    }

    class TemplatePropertyData {
    public:
        enum PropertyType {
            PropertyValueData,
            PropertyTemplateData,
            PropertyAccessorData,
            PropertyNativeAccessorData,
        };

        struct AccessorData {
            Optional<FunctionTemplate*> m_getterTemplate;
            Optional<FunctionTemplate*> m_setterTemplate;
        };

        TemplatePropertyData(const TemplateData& data, bool isWritable, bool isEnumerable, bool isConfigurable)
        {
            if (data.m_type == TemplateData::TemplateDataValue) {
                m_value = data.m_value;
                m_propertyData = 1;
                m_propertyData = m_propertyData | (PropertyValueData << 1);
            } else {
                m_template = data.m_template;
                m_propertyData = 1;
                m_propertyData = m_propertyData | (PropertyTemplateData << 1);
            }

            int attr = 0;

            if (isWritable) {
                attr |= ObjectStructurePropertyDescriptor::PresentAttribute::WritablePresent;
            }
            if (isEnumerable) {
                attr |= ObjectStructurePropertyDescriptor::PresentAttribute::EnumerablePresent;
            }
            if (isConfigurable) {
                attr |= ObjectStructurePropertyDescriptor::PresentAttribute::ConfigurablePresent;
            }

            m_propertyData = m_propertyData | (attr << 4);
        }

        TemplatePropertyData(Optional<FunctionTemplate*> getter, Optional<FunctionTemplate*> setter, bool isEnumerable, bool isConfigurable)
        {
            m_accessorData.m_getterTemplate = getter;
            m_accessorData.m_setterTemplate = setter;

            m_propertyData = 1;
            m_propertyData = m_propertyData | (PropertyAccessorData << 1);

            int attr = 0;
            if (isEnumerable) {
                attr |= ObjectStructurePropertyDescriptor::PresentAttribute::EnumerablePresent;
            }
            if (isConfigurable) {
                attr |= ObjectStructurePropertyDescriptor::PresentAttribute::ConfigurablePresent;
            }

            m_propertyData = m_propertyData | (attr << 4);
        }

        TemplatePropertyData(ObjectPropertyNativeGetterSetterData* nativeGetterSetterData, void* privateData)
        {
            m_nativeAccessorData.m_nativeGetterSetterData = nativeGetterSetterData;
            m_nativeAccessorData.m_privateData = privateData;

            m_propertyData = 1;
            m_propertyData = m_propertyData | (PropertyNativeAccessorData << 1);
            m_propertyData = m_propertyData | (nativeGetterSetterData->m_presentAttributes << 4);
        }

        TemplatePropertyData(const TemplatePropertyData& src)
        {
            m_propertyData = src.m_propertyData;
            m_accessorData = src.m_accessorData;
        }

        TemplatePropertyData& operator=(const TemplatePropertyData&) = default;

        PropertyType propertyType() const
        {
            return (PropertyType)((m_propertyData >> 1) & 0x3);
        }

        ObjectStructurePropertyDescriptor::PresentAttribute presentAttributes() const
        {
            return (ObjectStructurePropertyDescriptor::PresentAttribute)(m_propertyData >> 4);
        }

        Value valueData() const
        {
            ASSERT(propertyType() == PropertyType::PropertyValueData);
            return m_value;
        }

        Template* templateData() const
        {
            ASSERT(propertyType() == PropertyType::PropertyTemplateData);
            return m_template;
        }

        AccessorData accessorData() const
        {
            ASSERT(propertyType() == PropertyType::PropertyAccessorData);
            return m_accessorData;
        }

        ObjectPropertyNativeGetterSetterData* nativeAccessorData() const
        {
            ASSERT(propertyType() == PropertyType::PropertyNativeAccessorData);
            return m_nativeAccessorData.m_nativeGetterSetterData;
        }

        void* nativeAccessorPrivateData() const
        {
            ASSERT(propertyType() == PropertyType::PropertyNativeAccessorData);
            return m_nativeAccessorData.m_privateData;
        }

    protected:
        size_t m_propertyData;

        union {
            EncodedValue m_value;
            Template* m_template;
            AccessorData m_accessorData;
            struct {
                ObjectPropertyNativeGetterSetterData* m_nativeGetterSetterData;
                void* m_privateData;
            } m_nativeAccessorData;
        };
    };
    COMPILE_ASSERT(sizeof(TemplatePropertyData) == sizeof(size_t) * 3, "");

    // internally, property name is stored as ObjectStructurePropertyName because it only requires the size of `size_t` (4 or 8 bytes) which is memory-efficient
    Vector<std::pair<ObjectStructurePropertyName, TemplatePropertyData>, GCUtil::gc_malloc_allocator<std::pair<ObjectStructurePropertyName, TemplatePropertyData>>> m_properties;
    void* m_instanceExtraData;
    CachedObjectStructure m_cachedObjectStructure;
};
} // namespace Escargot

#endif
