/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
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

#ifndef __EscargotCodeBlock__
#define __EscargotCodeBlock__

#include "parser/ast/Node.h"
#include "parser/ast/ASTContext.h"
#include "runtime/AtomicString.h"
#include "runtime/ExecutionState.h"
#include "runtime/String.h"

namespace Escargot {

class Node;
class ByteCodeBlock;
class LexicalEnvironment;
class CodeBlock;
class InterpretedCodeBlock;
class Script;

// length of argv is same with NativeFunctionInfo.m_argumentCount
// only in construct call, newTarget have Object*
typedef Value (*NativeFunctionPointer)(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Optional<Object*> newTarget);

struct NativeFunctionInfo {
    enum Flags {
        Strict = 1,
        Constructor = 1 << 1,
    };

    bool m_isStrict : 1;
    bool m_isConstructor : 1;
    AtomicString m_name;
    NativeFunctionPointer m_nativeFunction;
    size_t m_argumentCount;

    NativeFunctionInfo(AtomicString name, NativeFunctionPointer fn, size_t argc, int flags = Flags::Strict | Flags::Constructor)
        : m_isStrict(flags & Strict)
        , m_isConstructor(flags & Constructor)
        , m_name(name)
        , m_nativeFunction(fn)
        , m_argumentCount(argc)
    {
    }
};

class CallNativeFunctionData : public gc {
public:
    CallNativeFunctionData(NativeFunctionPointer fn)
        : m_fn(fn)
    {
    }

    NativeFunctionPointer m_fn;
};

class NativeCodeBlock;
class InterpretedCodeBlock;

class CodeBlock : public gc {
public:
    void* operator new(size_t size) = delete;
    void* operator new[](size_t size) = delete;

    virtual uint16_t functionLength() const = 0;
    virtual AtomicString functionName() const = 0;

    virtual bool isNativeCodeBlock() const
    {
        return false;
    }

    virtual bool isInterpretedCodeBlock() const
    {
        return false;
    }

    NativeCodeBlock* asNativeCodeBlock()
    {
        ASSERT(isNativeCodeBlock());
        return (NativeCodeBlock*)this;
    }

    InterpretedCodeBlock* asInterpretedCodeBlock()
    {
        ASSERT(isInterpretedCodeBlock());
        return (InterpretedCodeBlock*)this;
    }

    Context* context()
    {
        return m_context;
    }

protected:
    CodeBlock()
        : m_context(nullptr)
    {
    }

    CodeBlock(Context* ctx)
        : m_context(ctx)
    {
    }

    Context* m_context;
};

class NativeCodeBlock : public CodeBlock {
    friend int getValidValueInNativeCodeBlock(void* ptr, GC_mark_custom_result* arr);

public:
    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    NativeCodeBlock(Context* ctx, const NativeFunctionInfo& info)
        : CodeBlock(ctx)
        , m_isNativeConstructor(info.m_isConstructor)
        , m_isStrict(info.m_isStrict)
        , m_functionLength(info.m_argumentCount)
        , m_functionName(info.m_name)
        , m_nativeFunctionData((CallNativeFunctionData*)(new (PointerFreeGC) CallNativeFunctionData(info.m_nativeFunction)))
    {
    }

    NativeCodeBlock(Context* ctx, const NativeFunctionInfo& info, CallNativeFunctionData* nativeData)
        : CodeBlock(ctx)
        , m_isNativeConstructor(info.m_isConstructor)
        , m_isStrict(info.m_isStrict)
        , m_functionLength(info.m_argumentCount)
        , m_functionName(info.m_name)
        , m_nativeFunctionData(nativeData)
    {
    }

    virtual bool isNativeCodeBlock() const override
    {
        return true;
    }

    virtual uint16_t functionLength() const override
    {
        return m_functionLength;
    }

    virtual AtomicString functionName() const override
    {
        return m_functionName;
    }

    bool isNativeConstructor() const
    {
        return m_isNativeConstructor;
    }

    bool isStrict() const
    {
        return m_isStrict;
    }

    CallNativeFunctionData* nativeFunctionData()
    {
        return m_nativeFunctionData;
    }

private:
    bool m_isNativeConstructor : 1;
    bool m_isStrict : 1;
    uint16_t m_functionLength;
    AtomicString m_functionName;
    CallNativeFunctionData* m_nativeFunctionData;
};

class InterpretedCodeBlock : public CodeBlock {
    friend class Script;
    friend class ScriptParser;
    friend class VMInstance;
    friend int getValidValueInInterpretedCodeBlock(void* ptr, GC_mark_custom_result* arr);

public:
    struct IndexedIdentifierInfo {
        bool m_isResultSaved : 1;
        bool m_isStackAllocated : 1;
        bool m_isMutable : 1;
        bool m_isGlobalLexicalVariable : 1;
        enum DeclarationType {
            VarDeclared,
            LexicallyDeclared,
        };

        DeclarationType m_type : 1;

        LexicalBlockIndex m_blockIndex : 16;

        size_t m_upperIndex;
        size_t m_index;

        IndexedIdentifierInfo()
            : m_isResultSaved(false)
            , m_isStackAllocated(false)
            , m_isMutable(false)
            , m_isGlobalLexicalVariable(false)
            , m_type(VarDeclared)
            , m_blockIndex(LEXICAL_BLOCK_INDEX_MAX)
            , m_upperIndex(SIZE_MAX)
            , m_index(SIZE_MAX)
        {
        }
    };

    struct BlockIdentifierInfo {
        bool m_needToAllocateOnStack : 1;
        bool m_isMutable : 1;
        size_t m_indexForIndexedStorage; // TODO reduce variable size into uint16_t.
        AtomicString m_name;
    };

    typedef TightVector<BlockIdentifierInfo, GCUtil::gc_malloc_atomic_allocator<BlockIdentifierInfo>> BlockIdentifierInfoVector;

    struct BlockInfo : public gc {
        ASTNodeType m_nodeType : 16;
        bool m_canAllocateEnvironmentOnStack : 1;
        bool m_shouldAllocateEnvironment : 1;
        LexicalBlockIndex m_parentBlockIndex;
        LexicalBlockIndex m_blockIndex;
        BlockIdentifierInfoVector m_identifiers;

#ifndef NDEBUG
        ExtendedNodeLOC m_loc;
#endif
        BlockInfo(
#ifndef NDEBUG
            ExtendedNodeLOC loc
#endif
            )
            : m_nodeType(ASTNodeType::ASTNodeTypeError)
            , m_canAllocateEnvironmentOnStack(false)
            , m_shouldAllocateEnvironment(false)
            , m_parentBlockIndex(LEXICAL_BLOCK_INDEX_MAX)
            , m_blockIndex(LEXICAL_BLOCK_INDEX_MAX)
#ifndef NDEBUG
            , m_loc(loc)
#endif
        {
        }

        void* operator new(size_t size);
        void* operator new[](size_t size) = delete;
    };

    typedef TightVector<BlockInfo*, GCUtil::gc_malloc_allocator<BlockInfo*>> BlockInfoVector;

    struct IdentifierInfo {
        bool m_needToAllocateOnStack : 1;
        bool m_isMutable : 1;
        bool m_isParameterName : 1;
        bool m_isExplicitlyDeclaredOrParameterName : 1;
        bool m_isVarDeclaration : 1;
        size_t m_indexForIndexedStorage; // TODO reduce variable size into uint16_t.
        AtomicString m_name;
    };

    typedef TightVector<IdentifierInfo, GCUtil::gc_malloc_atomic_allocator<IdentifierInfo>> IdentifierInfoVector;

    struct InterpretedCodeBlockRareData : public gc {
        FunctionContextVarMap* m_identifierInfoMap;
        TightVector<Optional<ArrayObject*>, GCUtil::gc_malloc_allocator<Optional<ArrayObject*>>> m_taggedTemplateLiteralCache;

        InterpretedCodeBlockRareData(FunctionContextVarMap* map)
            : m_identifierInfoMap(map)
        {
        }
    };

    // init global codeBlock
    InterpretedCodeBlock(Context* ctx, Script* script, StringView src, ASTScopeContext* scopeCtx, bool isEvalCode, bool isEvalCodeInFunction);
    // init function codeBlock
    InterpretedCodeBlock(Context* ctx, Script* script, StringView src, ASTScopeContext* scopeCtx, InterpretedCodeBlock* parentBlock, bool isEvalCode, bool isEvalCodeInFunction);

    void* operator new(size_t size);
    void* operator new[](size_t size) = delete;

    virtual bool isInterpretedCodeBlock() const override
    {
        return true;
    }

    virtual uint16_t functionLength() const override
    {
        return m_functionLength;
    }

    virtual AtomicString functionName() const override
    {
        return m_functionName;
    }

    bool isKindOfFunction() const
    {
        return m_isFunctionDeclaration || m_isFunctionExpression || m_isArrowFunctionExpression || m_isClassConstructor || m_isObjectMethod || m_isClassMethod || m_isClassStaticMethod;
    }

    Script* script()
    {
        return m_script;
    }

    const StringView& src()
    {
        return m_src;
    }

    ByteCodeBlock* byteCodeBlock()
    {
        return m_byteCodeBlock;
    }

    InterpretedCodeBlock* parentCodeBlock()
    {
        return m_parentCodeBlock;
    }

    InterpretedCodeBlock* firstChild()
    {
        return m_firstChild;
    }

    InterpretedCodeBlock* nextSibling()
    {
        return m_nextSibling;
    }

    InterpretedCodeBlockRareData* ensureRareData()
    {
        if (!m_rareData) {
            m_rareData = new InterpretedCodeBlockRareData(nullptr);
        }
        return m_rareData;
    }

    const AtomicStringTightVector& parameterNames() const
    {
        // return all parameter names vector including targets of patterns and rest element
        return m_parameterNames;
    }

    const IdentifierInfoVector& identifierInfos() const
    {
        return m_identifierInfos;
    }

    const BlockInfoVector& blockInfos() const
    {
        return m_blockInfos;
    }

    ExtendedNodeLOC functionStart()
    {
        return m_functionStart;
    }

#if !(defined NDEBUG) || defined ESCARGOT_DEBUGGER
    ExtendedNodeLOC bodyEndLOC()
    {
        return m_bodyEndLOC;
    }
#endif

    size_t parameterNamesCount() const
    {
        // return the number of all parameter names including targets of patterns and rest element
        return m_parameterNames.size();
    }

    size_t parameterCount() const
    {
        // return the number of parameter elements
        return m_parameterCount;
    }

    size_t identifierOnStackCount() const // var
    {
        return m_identifierOnStackCount;
    }

    size_t identifierOnHeapCount() const // var
    {
        return m_identifierOnHeapCount;
    }

    size_t lexicalBlockStackAllocatedIdentifierMaximumDepth() const // let
    {
        return m_lexicalBlockStackAllocatedIdentifierMaximumDepth;
    }

    LexicalBlockIndex functionBodyBlockIndex() const
    {
        return m_functionBodyBlockIndex;
    }

    LexicalBlockIndex lexicalBlockIndexFunctionLocatedIn() const
    {
        return m_lexicalBlockIndexFunctionLocatedIn;
    }

    /*
    - variable access rule -
    type                                         | canUseIndexedVariableStorage  | canAllocateVariablesOnStack | canAllocateEnvironmentOnStack                      | when access variable
    normal codes                                 | true                          | true                        | true when there is no captured variable by closure | use variable information on CodeBlock. if there is no variable information self or ancestor, use {Load, Store}GlobalVariable bytecode
    codes has eval, with...                      | false                         | false                       | false                                              | every variable should use {Load, Store}ByName bytecode
    ancestors of !canUseIndexedVariableStorage   | true                          | false                       | false                                              | same as normal code
    descendants of !canUseIndexedVariableStorage | true                          | true                        | true when there is no captured variable by closure | use variable information on CodeBlock. if there is no variable information self or ancestor, use {Load, Store}ByName bytecode
                           -                     |  -                            |  -                          | && every usingnames resolved on compile time       |     -
    isEvalCode                                   | false                         | false                       | false                                              | every variable should use {Load, Store}ByName bytecode
    */

    void setHasEval()
    {
        m_hasEval = true;
        m_canUseIndexedVariableStorage = false;
    }

    void setAsClassConstructor()
    {
        m_isClassConstructor = true;
    }

    void setAsDerivedClassConstructor()
    {
        m_isDerivedClassConstructor = true;
    }

    void setNeedsVirtualIDOperation()
    {
        ASSERT(isInterpretedCodeBlock());
        m_needsVirtualIDOperation = true;
    }

    bool shouldHaveMappedArguments() const
    {
        return !hasParameterOtherThanIdentifier() && !isStrict();
    }

    bool inWith()
    {
        return m_inWith;
    }

    bool hasEval() const
    {
        return m_hasEval;
    }

    bool hasWith() const
    {
        return m_hasWith;
    }

    bool isStrict() const
    {
        return m_isStrict;
    }

    bool canUseIndexedVariableStorage() const
    {
        return m_canUseIndexedVariableStorage;
    }

    bool canAllocateVariablesOnStack() const
    {
        return m_canAllocateVariablesOnStack;
    }

    bool canAllocateEnvironmentOnStack() const
    {
        return m_canAllocateEnvironmentOnStack;
    }

    bool isFunctionDeclaration() const
    {
        return m_isFunctionDeclaration;
    }

    bool isFunctionExpression() const
    {
        return m_isFunctionExpression;
    }

    bool isArrowFunctionExpression() const
    {
        return m_isArrowFunctionExpression;
    }

    bool isOneExpressionOnlyArrowFunctionExpression() const
    {
        return m_isOneExpressionOnlyArrowFunctionExpression;
    }

    bool isClassConstructor() const
    {
        return m_isClassConstructor;
    }

    bool isDerivedClassConstructor() const
    {
        return m_isDerivedClassConstructor;
    }

    bool isObjectMethod() const
    {
        return m_isObjectMethod;
    }

    bool isClassMethod() const
    {
        return m_isClassMethod;
    }

    bool isClassStaticMethod() const
    {
        return m_isClassStaticMethod;
    }

    bool isGenerator() const
    {
        return m_isGenerator;
    }

    bool isAsync() const
    {
        return m_isAsync;
    }

    bool usesArgumentsObject() const
    {
        return m_usesArgumentsObject;
    }

    bool hasArrowParameterPlaceHolder() const
    {
        return m_hasArrowParameterPlaceHolder;
    }

    bool hasParameterOtherThanIdentifier() const
    {
        return m_hasParameterOtherThanIdentifier;
    }

    bool allowSuperCall() const
    {
        return m_allowSuperCall;
    }

    bool allowSuperProperty() const
    {
        return m_allowSuperProperty;
    }

    bool isFunctionNameSaveOnHeap() const
    {
        return m_isFunctionNameSaveOnHeap;
    }

    bool isFunctionNameExplicitlyDeclared() const
    {
        return m_isFunctionNameExplicitlyDeclared;
    }

    bool isEvalCode() const
    {
        return m_isEvalCode;
    }

    bool isEvalCodeInFunction() const
    {
        return m_isEvalCodeInFunction;
    }

    bool needsVirtualIDOperation()
    {
        return m_needsVirtualIDOperation;
    }

    // You can use this function on ScriptParser only
    void computeVariables();
    // You can use this function on ScriptParser only
    void captureArguments();

    // You can use this function on ScriptParser only
    /* capture ok, block vector index(if not block variable, returns SIZE_MAX) */
    std::pair<bool, size_t> tryCaptureIdentifiersFromChildCodeBlock(LexicalBlockIndex blockIndex, AtomicString name);

    BlockInfo* blockInfo(LexicalBlockIndex blockIndex)
    {
        for (size_t i = 0; i < m_blockInfos.size(); i++) {
            if (m_blockInfos[i]->m_blockIndex == blockIndex) {
                return m_blockInfos[i];
            }
        }
        ASSERT_NOT_REACHED();
        return nullptr;
    }

    size_t totalStackAllocatedVariableSize() const
    {
        return identifierOnStackCount() + lexicalBlockStackAllocatedIdentifierMaximumDepth();
    }


    bool needsToLoadThisBindingFromEnvironment();

    bool isGlobalScopeCodeBlock() const
    {
        return m_parentCodeBlock == nullptr;
    }

    bool hasAncestorUsesNonIndexedVariableStorage()
    {
        auto ptr = m_parentCodeBlock;

        while (ptr) {
            if (!ptr->canUseIndexedVariableStorage()) {
                return true;
            }
            ptr = ptr->parentCodeBlock();
        }

        return false;
    }

    IndexedIdentifierInfo indexedIdentifierInfo(const AtomicString& name, LexicalBlockIndex blockIndex);

    void appendChild(InterpretedCodeBlock* child)
    {
        ASSERT(child->m_nextSibling == nullptr);
        if (m_firstChild == nullptr) {
            m_firstChild = child;
        } else {
            InterpretedCodeBlock* tail = firstChild();
            while (tail->m_nextSibling != nullptr) {
                tail = tail->m_nextSibling;
            }
            tail->m_nextSibling = child;
        }
    }

    void appendChild(InterpretedCodeBlock* child, InterpretedCodeBlock* referNode)
    {
        if (referNode == nullptr) {
            appendChild(child);
        } else {
            referNode->m_nextSibling = child;
        }
    }

    InterpretedCodeBlock* childBlockAt(size_t idx)
    {
        ASSERT(!!m_firstChild);
        InterpretedCodeBlock* c = m_firstChild;

        for (size_t i = 0; i < idx; i++) {
            ASSERT(c->nextSibling());
            c = c->nextSibling();
        }

        return c;
    }

    size_t findVarName(const AtomicString& name)
    {
        auto map = identifierInfoMap();
        if (UNLIKELY(map)) {
            auto iter = map->find(name);
            if (iter == map->end()) {
                return SIZE_MAX;
            } else {
                return iter->second;
            }
        }

        auto& v = this->m_identifierInfos;
        size_t size = v.size();

        if (LIKELY(size <= 12)) {
            size_t idx = SIZE_MAX;
            switch (size) {
            case 12:
                if (v[11].m_name == name) {
                    idx = 11;
                }
                FALLTHROUGH;
#define TEST_ONCE(n)                                      \
    case n:                                               \
        if (idx == SIZE_MAX && v[n - 1].m_name == name) { \
            idx = n - 1;                                  \
        }                                                 \
        FALLTHROUGH;
                TEST_ONCE(11)
                TEST_ONCE(10)
                TEST_ONCE(9)
                TEST_ONCE(8)
                TEST_ONCE(7)
                TEST_ONCE(6)
                TEST_ONCE(5)
                TEST_ONCE(4)
                TEST_ONCE(3)
                TEST_ONCE(2)
                TEST_ONCE(1)
#undef TEST_ONCE
            case 0:
                break;
            default:
                ASSERT_NOT_REACHED();
            }

            return idx;
        } else {
            for (size_t i = 0; i < size; i++) {
                if (v[i].m_name == name) {
                    return i;
                }
            }
            return SIZE_MAX;
        }
    }

    bool hasName(LexicalBlockIndex blockIndex, const AtomicString& name)
    {
        if (std::get<0>(findNameWithinBlock(blockIndex, name))) {
            return true;
        }

        if (blockIndex < m_functionBodyBlockIndex) {
            return isParameterName(name);
        }

        return findVarName(name) != SIZE_MAX;
    }

    bool hasParameterName(const AtomicString& name)
    {
        for (size_t i = 0; i < parameterNamesCount(); i++) {
            if (m_parameterNames[i] == name) {
                return true;
            }
        }
        return false;
    }

    bool isParameterName(const AtomicString& name)
    {
        size_t r = findVarName(name);
        if (r != SIZE_MAX) {
            return m_identifierInfos[r].m_isParameterName;
        }

        return false;
    }

    void markHeapAllocatedEnvironmentFromHere(LexicalBlockIndex blockIndex = 0, InterpretedCodeBlock* to = nullptr);

    Optional<FunctionContextVarMap*> identifierInfoMap()
    {
        return m_rareData ? m_rareData->m_identifierInfoMap : nullptr;
    }

#ifndef NDEBUG
    ASTScopeContext* scopeContext()
    {
        ASSERT(!!m_scopeContext);
        return m_scopeContext;
    }
#endif

private:
    Script* m_script;
    StringView m_src; // function source including parameters
    ByteCodeBlock* m_byteCodeBlock;

    InterpretedCodeBlock* m_parentCodeBlock;
    InterpretedCodeBlock* m_firstChild;
    InterpretedCodeBlock* m_nextSibling;

    InterpretedCodeBlockRareData* m_rareData;

    // all parameter names including targets of patterns and rest element
    AtomicStringTightVector m_parameterNames;
    IdentifierInfoVector m_identifierInfos;
    BlockInfoVector m_blockInfos;

    AtomicString m_functionName;

    ExtendedNodeLOC m_functionStart; // point to the start position
#if !(defined NDEBUG) || defined ESCARGOT_DEBUGGER
    ExtendedNodeLOC m_bodyEndLOC;
#endif

    uint16_t m_functionLength;
    uint16_t m_parameterCount; // number of parameter elements

    uint16_t m_identifierOnStackCount; // this member variable only count `var`
    uint16_t m_identifierOnHeapCount; // this member variable only count `var`
    uint16_t m_lexicalBlockStackAllocatedIdentifierMaximumDepth; // this member variable only count `let`

    LexicalBlockIndex m_functionBodyBlockIndex;
    LexicalBlockIndex m_lexicalBlockIndexFunctionLocatedIn;

    bool m_isFunctionNameSaveOnHeap : 1;
    bool m_isFunctionNameExplicitlyDeclared : 1;
    bool m_canUseIndexedVariableStorage : 1;
    bool m_canAllocateVariablesOnStack : 1;
    bool m_canAllocateEnvironmentOnStack : 1;
    bool m_hasDescendantUsesNonIndexedVariableStorage : 1;
    bool m_hasEval : 1;
    bool m_hasWith : 1;
    bool m_isStrict : 1;
    bool m_inWith : 1;
    bool m_isEvalCode : 1;
    bool m_isEvalCodeInFunction : 1;
    bool m_usesArgumentsObject : 1;
    bool m_isFunctionExpression : 1;
    bool m_isFunctionDeclaration : 1;
    bool m_isArrowFunctionExpression : 1;
    // one expression only arrow function only contains one expression in body(no param, no placeholder, no brace)
    bool m_isOneExpressionOnlyArrowFunctionExpression : 1;
    bool m_isClassConstructor : 1;
    bool m_isDerivedClassConstructor : 1;
    bool m_isObjectMethod : 1;
    bool m_isClassMethod : 1;
    bool m_isClassStaticMethod : 1;
    bool m_isGenerator : 1;
    bool m_isAsync : 1;
    bool m_needsVirtualIDOperation : 1;
    bool m_hasArrowParameterPlaceHolder : 1;
    bool m_hasParameterOtherThanIdentifier : 1;
    bool m_allowSuperCall : 1;
    bool m_allowSuperProperty : 1;

#ifndef NDEBUG
    ASTScopeContext* m_scopeContext;
#endif

    void recordGlobalParsingInfo(ASTScopeContext* scopeCtx, bool isEvalCode, bool isEvalCodeInFunction);
    void recordFunctionParsingInfo(ASTScopeContext* scopeCtxm, bool isEvalCode, bool isEvalCodeInFunction);

    void computeBlockVariables(LexicalBlockIndex currentBlockIndex, size_t currentStackAllocatedVariableIndex, size_t& maxStackAllocatedVariableDepth);
    void initBlockScopeInformation(ASTScopeContext* scopeCtx);

    // You can use this function on ScriptParser only
    std::tuple<bool, size_t, size_t> findNameWithinBlock(LexicalBlockIndex blockIndex, AtomicString name)
    {
        BlockInfo* b = nullptr;
        size_t blockVectorIndex = SIZE_MAX;
        size_t blockInfoSize = m_blockInfos.size();
        for (size_t i = 0; i < blockInfoSize; i++) {
            if (m_blockInfos[i]->m_blockIndex == blockIndex) {
                b = m_blockInfos[i];
                blockVectorIndex = i;
                break;
            }
        }

        ASSERT(b != nullptr);
        while (true) {
            auto& v = b->m_identifiers;
            size_t idSize = v.size();
            for (size_t i = 0; i < idSize; i++) {
                if (v[i].m_name == name) {
                    return std::make_tuple(true, blockVectorIndex, i);
                }
            }

            if (b->m_parentBlockIndex == LEXICAL_BLOCK_INDEX_MAX) {
                break;
            }

#ifndef NDEBUG
            bool finded = false;
#endif
            for (size_t i = 0; i < blockInfoSize; i++) {
                if (m_blockInfos[i]->m_blockIndex == b->m_parentBlockIndex) {
                    b = m_blockInfos[i];
                    blockVectorIndex = i;
#ifndef NDEBUG
                    finded = true;
#endif
                    break;
                }
            }
            ASSERT(finded);
        }

        return std::make_tuple(false, SIZE_MAX, SIZE_MAX);
    }
};
}

#endif
