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

#if defined(ENABLE_CODE_CACHE)

#include "Escargot.h"
#include "CodeCacheReaderWriter.h"
#include "parser/Script.h"
#include "parser/CodeBlock.h"
#include "interpreter/ByteCode.h"
#include "runtime/Context.h"
#include "runtime/AtomicString.h"
#include "runtime/ObjectStructurePropertyName.h"

namespace Escargot {

size_t CacheStringTable::add(const AtomicString& string)
{
    size_t index = 0;
    for (; index < m_table.size(); index++) {
        if (m_table[index] == string) {
            return index;
        }
    }

    m_table.push_back(string);
    ASSERT(index == m_table.size() - 1);

    size_t length = string.string()->length();
    m_maxLength = length > m_maxLength ? length : m_maxLength;

    if (!m_has16BitString && !string.string()->has8BitContent()) {
        m_has16BitString = true;
    }

    return index;
}

void CacheStringTable::initAdd(const AtomicString& string)
{
#ifndef NDEBUG
    for (size_t i = 0; i < m_table.size(); i++) {
        if (m_table[i] == string) {
            ASSERT_NOT_REACHED();
        }
    }
#endif

    m_table.push_back(string);
}

AtomicString& CacheStringTable::get(size_t index)
{
    ASSERT(index < m_table.size());
    return m_table[index];
}

void CodeCacheWriter::CacheBuffer::ensureSize(size_t size)
{
    if (!isAvailable(size)) {
        ComputeReservedCapacityFunctionWithLog2<200> f;
        size_t newCapacity = f(m_capacity + size);

        char* newBuffer = static_cast<char*>(malloc(newCapacity));
        memcpy(newBuffer, m_buffer, m_index);
        free(m_buffer);

        m_buffer = newBuffer;
        m_capacity = newCapacity;
    }
}

void CodeCacheWriter::CacheBuffer::reset()
{
    free(m_buffer);

    m_buffer = nullptr;
    m_capacity = 0;
    m_index = 0;
}

void CodeCacheWriter::storeInterpretedCodeBlock(InterpretedCodeBlock* codeBlock)
{
    ASSERT(GC_is_disabled());
    ASSERT(!!codeBlock && codeBlock->isInterpretedCodeBlock());

    size_t size;

    m_buffer.ensureSize(sizeof(bool) + 4 * sizeof(size_t));

    // represent if InterpretedCodeBlockWithRareData
    bool hasRareData = codeBlock->hasRareData() ? true : false;
    m_buffer.put(hasRareData);

    {
        // InterpretedCodeBlock::m_src
        size_t start = codeBlock->src().start();
        size_t end = codeBlock->src().end();
        m_buffer.put(start);
        m_buffer.put(end);
    }

    // InterpretedCodeBlock::m_parent
    // end position is used as cache id of each CodeBlock
    size = codeBlock->parent() ? codeBlock->parent()->src().end() : SIZE_MAX;
    m_buffer.put(size);

    // InterpretedCodeBlock::m_children
    size = codeBlock->hasChildren() ? codeBlock->children().size() : 0;
    m_buffer.put(size);
    m_buffer.ensureSize(size * sizeof(size_t));
    for (size_t i = 0; i < size; i++) {
        // end position is used as cache id of each CodeBlock
        size_t srcIndex = codeBlock->children()[i]->src().end();
        m_buffer.put(srcIndex);
    }

    // InterpretedCodeBlock::m_parameterNames
    const AtomicStringTightVector& atomicStringVector = codeBlock->m_parameterNames;
    size = atomicStringVector.size();
    m_buffer.ensureSize((size + 1) * sizeof(size_t));
    m_buffer.put(size);
    for (size_t i = 0; i < size; i++) {
        const size_t stringIndex = m_stringTable->add(atomicStringVector[i]);
        m_buffer.put(stringIndex);
    }

    // InterpretedCodeBlock::m_identifierInfos
    const InterpretedCodeBlock::IdentifierInfoVector& identifierVector = codeBlock->m_identifierInfos;
    size = identifierVector.size();
    m_buffer.ensureSize(sizeof(size_t) + size * (5 * sizeof(bool) + 2 * sizeof(size_t)));
    m_buffer.put(size);
    for (size_t i = 0; i < size; i++) {
        const InterpretedCodeBlock::IdentifierInfo& info = identifierVector[i];
        m_buffer.put(info.m_needToAllocateOnStack);
        m_buffer.put(info.m_isMutable);
        m_buffer.put(info.m_isParameterName);
        m_buffer.put(info.m_isExplicitlyDeclaredOrParameterName);
        m_buffer.put(info.m_isVarDeclaration);
        m_buffer.put(info.m_indexForIndexedStorage);
        m_buffer.put(m_stringTable->add(info.m_name));
    }

    // InterpretedCodeBlock::m_blockInfos
    const InterpretedCodeBlock::BlockInfoVector& blockInfoVector = codeBlock->m_blockInfos;
    size = blockInfoVector.size();
    m_buffer.ensureSize(sizeof(size_t));
    m_buffer.put(size);
    // FIXME ignore BlockInfo::m_loc member
    for (size_t i = 0; i < size; i++) {
        m_buffer.ensureSize(2 * sizeof(bool) + 3 * sizeof(uint16_t));
        const InterpretedCodeBlock::BlockInfo* info = blockInfoVector[i];
        m_buffer.put(info->m_canAllocateEnvironmentOnStack);
        m_buffer.put(info->m_shouldAllocateEnvironment);
        m_buffer.put((uint16_t)info->m_nodeType);
        m_buffer.put((uint16_t)info->m_parentBlockIndex);
        m_buffer.put((uint16_t)info->m_blockIndex);

        // InterpretedCodeBlock::BlockInfo::m_identifiers
        const InterpretedCodeBlock::BlockIdentifierInfoVector& infoVector = info->m_identifiers;
        const size_t vectorSize = infoVector.size();
        m_buffer.ensureSize(sizeof(size_t) + vectorSize * (2 * sizeof(bool) + 2 * sizeof(size_t)));
        m_buffer.put(vectorSize);
        for (size_t j = 0; j < vectorSize; j++) {
            const InterpretedCodeBlock::BlockIdentifierInfo& info = infoVector[j];
            m_buffer.put(info.m_needToAllocateOnStack);
            m_buffer.put(info.m_isMutable);
            m_buffer.put(info.m_indexForIndexedStorage);
            m_buffer.put(m_stringTable->add(info.m_name));
        }
    }

    // InterpretedCodeBlock::m_functionName
    m_buffer.ensureSize(sizeof(size_t));
    m_buffer.put(m_stringTable->add(codeBlock->m_functionName));

    // InterpretedCodeBlock::m_functionStart
    m_buffer.ensureSize(sizeof(ExtendedNodeLOC));
    m_buffer.put(codeBlock->m_functionStart);

#ifndef NDEBUG
    // InterpretedCodeBlock::m_bodyEndLOC
    m_buffer.ensureSize(sizeof(ExtendedNodeLOC));
    m_buffer.put(codeBlock->m_bodyEndLOC);
#endif

    m_buffer.ensureSize(5 * sizeof(uint16_t));
    m_buffer.put(codeBlock->m_functionLength);
    m_buffer.put(codeBlock->m_parameterCount);
    m_buffer.put(codeBlock->m_identifierOnStackCount);
    m_buffer.put(codeBlock->m_identifierOnHeapCount);
    m_buffer.put(codeBlock->m_lexicalBlockStackAllocatedIdentifierMaximumDepth);

    m_buffer.ensureSize(2 * sizeof(LexicalBlockIndex));
    m_buffer.put(codeBlock->m_functionBodyBlockIndex);
    m_buffer.put(codeBlock->m_lexicalBlockIndexFunctionLocatedIn);

    m_buffer.ensureSize(29 * sizeof(bool));
    m_buffer.put(codeBlock->m_isFunctionNameSaveOnHeap);
    m_buffer.put(codeBlock->m_isFunctionNameExplicitlyDeclared);
    m_buffer.put(codeBlock->m_canUseIndexedVariableStorage);
    m_buffer.put(codeBlock->m_canAllocateVariablesOnStack);
    m_buffer.put(codeBlock->m_canAllocateEnvironmentOnStack);
    m_buffer.put(codeBlock->m_hasDescendantUsesNonIndexedVariableStorage);
    m_buffer.put(codeBlock->m_hasEval);
    m_buffer.put(codeBlock->m_hasWith);
    m_buffer.put(codeBlock->m_isStrict);
    m_buffer.put(codeBlock->m_inWith);
    m_buffer.put(codeBlock->m_isEvalCode);
    m_buffer.put(codeBlock->m_isEvalCodeInFunction);
    m_buffer.put(codeBlock->m_usesArgumentsObject);
    m_buffer.put(codeBlock->m_isFunctionExpression);
    m_buffer.put(codeBlock->m_isFunctionDeclaration);
    m_buffer.put(codeBlock->m_isArrowFunctionExpression);
    m_buffer.put(codeBlock->m_isOneExpressionOnlyArrowFunctionExpression);
    m_buffer.put(codeBlock->m_isClassConstructor);
    m_buffer.put(codeBlock->m_isDerivedClassConstructor);
    m_buffer.put(codeBlock->m_isObjectMethod);
    m_buffer.put(codeBlock->m_isClassMethod);
    m_buffer.put(codeBlock->m_isClassStaticMethod);
    m_buffer.put(codeBlock->m_isGenerator);
    m_buffer.put(codeBlock->m_isAsync);
    m_buffer.put(codeBlock->m_needsVirtualIDOperation);
    m_buffer.put(codeBlock->m_hasArrowParameterPlaceHolder);
    m_buffer.put(codeBlock->m_hasParameterOtherThanIdentifier);
    m_buffer.put(codeBlock->m_allowSuperCall);
    m_buffer.put(codeBlock->m_allowSuperProperty);

    // InterpretedCodeBlockWithRareData
    if (UNLIKELY(hasRareData)) {
        ASSERT(codeBlock->taggedTemplateLiteralCache().size() == 0);
        ASSERT(codeBlock->identifierInfoMap() && !codeBlock->identifierInfoMap()->empty());

        auto varMap = codeBlock->identifierInfoMap();
        size_t mapSize = varMap->size();
        m_buffer.ensureSize((1 + mapSize * 2) * sizeof(size_t));
        m_buffer.put(mapSize);
        for (auto iter = varMap->begin(); iter != varMap->end(); iter++) {
            size_t stringIndex = m_stringTable->add(iter->first);
            size_t index = iter->second;
            m_buffer.put(stringIndex);
            m_buffer.put(index);
        }
    }
}

void CodeCacheWriter::storeByteCodeBlock(ByteCodeBlock* block)
{
    ASSERT(GC_is_disabled());
    ASSERT(!!block);

    size_t size;

    m_buffer.ensureSize(4 * sizeof(bool) + sizeof(uint16_t));
    m_buffer.put(block->m_isEvalMode);
    m_buffer.put(block->m_isOnGlobal);
    m_buffer.put(block->m_shouldClearStack);
    m_buffer.put(block->m_isOwnerMayFreed);
    m_buffer.put((uint16_t)block->m_requiredRegisterFileSizeInValueSize);

    // ByteCodeBlock::m_numeralLiteralData
    ByteCodeNumeralLiteralData& numeralLiteralData = block->m_numeralLiteralData;
    m_buffer.putData(numeralLiteralData.data(), numeralLiteralData.size());

    // ByteCodeBlock::m_stringLiteralData
    ByteCodeStringLiteralData& stringLiteralData = block->m_stringLiteralData;
    size = stringLiteralData.size();
    m_buffer.ensureSize(sizeof(size_t));
    m_buffer.put(size);
    for (size_t i = 0; i < size; i++) {
        m_buffer.putString(stringLiteralData[i]);
    }

    // ByteCodeBlock::m_otherLiteralData
    ByteCodeOtherLiteralData& otherLiteralData = block->m_otherLiteralData;
    size = otherLiteralData.size();
    m_buffer.ensureSize(sizeof(size_t) + size * sizeof(ControlFlowRecord));
    m_buffer.put(size);
    for (size_t i = 0; i < size; i++) {
        ControlFlowRecord* record = (ControlFlowRecord*)otherLiteralData[i];
        ASSERT(record->m_reason == ControlFlowRecord::NeedsJump);
        m_buffer.put(*record);
    }

    // ByteCodeBlock::m_code bytecode stream
    storeByteCodeStream(block);

    // Do not store m_inlineCacheDataSize and m_locData
    // these members are used during the runtime
    ASSERT(block->m_inlineCacheDataSize == 0);
    ASSERT(!block->m_locData);
}

#define STORE_ATOMICSTRING_RELOC(member)                 \
    size_t stringIndex = m_stringTable->add(bc->member); \
    relocInfoVector.push_back(ByteCodeRelocInfo(ByteCodeRelocType::RELOC_ATOMICSTRING, (size_t)currentCode - codeBase, stringIndex));


void CodeCacheWriter::storeByteCodeStream(ByteCodeBlock* block)
{
    ByteCodeBlockData& byteCodeStream = block->m_code;
    ASSERT(byteCodeStream.size() > 0);

    // store ByteCode stream
    m_buffer.putData(byteCodeStream.data(), byteCodeStream.size());

    Vector<ByteCodeRelocInfo, std::allocator<ByteCodeRelocInfo>> relocInfoVector;
    ByteCodeStringLiteralData& stringLiteralData = block->m_stringLiteralData;

    // mark bytecode relocation infos
    {
        char* code = byteCodeStream.data();
        size_t codeBase = (size_t)code;
        char* end = code + byteCodeStream.size();

        Context* context = block->codeBlock()->context();

        while (code < end) {
            ByteCode* currentCode = (ByteCode*)code;
#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
            Opcode opcode = (Opcode)(size_t)currentCode->m_opcodeInAddress;
#else
            Opcode opcode = currentCode->m_opcode;
#endif
            switch (opcode) {
            case LoadLiteralOpcode: {
                LoadLiteral* bc = (LoadLiteral*)currentCode;
                if (bc->m_value.isPointerValue()) {
                    ASSERT(bc->m_value.asPointerValue()->isString());
                    String* string = bc->m_value.asPointerValue()->asString();
                    size_t stringIndex = VectorUtil::findInVector(stringLiteralData, string);
                    if (stringIndex != VectorUtil::invalidIndex) {
                        relocInfoVector.push_back(ByteCodeRelocInfo(ByteCodeRelocType::RELOC_STRING, (size_t)currentCode - codeBase, stringIndex));
                    } else {
                        ASSERT(string->isAtomicStringSource());
                        stringIndex = m_stringTable->add(AtomicString(context, string));
                        relocInfoVector.push_back(ByteCodeRelocInfo(ByteCodeRelocType::RELOC_ATOMICSTRING, (size_t)currentCode - codeBase, stringIndex));
                    }
                }
                break;
            }
            case LoadByNameOpcode: {
                LoadByName* bc = (LoadByName*)currentCode;
                STORE_ATOMICSTRING_RELOC(m_name);
                break;
            }
            case StoreByNameOpcode: {
                StoreByName* bc = (StoreByName*)currentCode;
                STORE_ATOMICSTRING_RELOC(m_name);
                break;
            }
            case InitializeByNameOpcode: {
                InitializeByName* bc = (InitializeByName*)currentCode;
                STORE_ATOMICSTRING_RELOC(m_name);
                break;
            }
            case CreateFunctionOpcode: {
                CreateFunction* bc = (CreateFunction*)currentCode;
                InterpretedCodeBlock* codeBlock = bc->m_codeBlock;
                ASSERT(!!codeBlock);
                size_t codeBlockIndex = VectorUtil::findInVector(block->codeBlock()->children(), codeBlock);
                ASSERT(codeBlockIndex != VectorUtil::invalidIndex);
                relocInfoVector.push_back(ByteCodeRelocInfo(ByteCodeRelocType::RELOC_CODEBLOCK, (size_t)currentCode - codeBase, codeBlockIndex));
                break;
            }
            case CreateClassOpcode: {
                CreateClass* bc = (CreateClass*)currentCode;
                InterpretedCodeBlock* codeBlock = bc->m_codeBlock;
                if (codeBlock) {
                    size_t codeBlockIndex = VectorUtil::findInVector(block->codeBlock()->children(), codeBlock);
                    ASSERT(codeBlockIndex != VectorUtil::invalidIndex);
                    relocInfoVector.push_back(ByteCodeRelocInfo(ByteCodeRelocType::RELOC_CODEBLOCK, (size_t)currentCode - codeBase, codeBlockIndex));
                }

                String* string = bc->m_classSrc;
                ASSERT(!!string && string->length() > 0);
                size_t stringIndex = VectorUtil::findInVector(stringLiteralData, string);

                ASSERT(stringIndex != VectorUtil::invalidIndex);
                relocInfoVector.push_back(ByteCodeRelocInfo(ByteCodeRelocType::RELOC_STRING, (size_t)currentCode - codeBase, stringIndex));
                break;
            }
            case ObjectDefineOwnPropertyWithNameOperationOpcode: {
                ObjectDefineOwnPropertyWithNameOperation* bc = (ObjectDefineOwnPropertyWithNameOperation*)currentCode;
                STORE_ATOMICSTRING_RELOC(m_propertyName);
                break;
            }
            case InitializeGlobalVariableOpcode: {
                InitializeGlobalVariable* bc = (InitializeGlobalVariable*)currentCode;
                STORE_ATOMICSTRING_RELOC(m_variableName);
                break;
            }
            case UnaryTypeofOpcode: {
                UnaryTypeof* bc = (UnaryTypeof*)currentCode;
                STORE_ATOMICSTRING_RELOC(m_id);
                break;
            }
            case UnaryDeleteOpcode: {
                UnaryDelete* bc = (UnaryDelete*)currentCode;
                STORE_ATOMICSTRING_RELOC(m_id);
                break;
            }
            case JumpComplexCaseOpcode: {
                JumpComplexCase* bc = (JumpComplexCase*)currentCode;
                ControlFlowRecord* record = bc->m_controlFlowRecord;
                size_t recordIndex = VectorUtil::findInVector(block->m_otherLiteralData, (void*)record);
                ASSERT(recordIndex != VectorUtil::invalidIndex);
                relocInfoVector.push_back(ByteCodeRelocInfo(ByteCodeRelocType::RELOC_CONTROLFLOWRECORD, (size_t)currentCode - codeBase, recordIndex));
                break;
            }
            case CallFunctionComplexCaseOpcode: {
                CallFunctionComplexCase* bc = (CallFunctionComplexCase*)currentCode;
                if (bc->m_kind == CallFunctionComplexCase::InWithScope) {
                    STORE_ATOMICSTRING_RELOC(m_calleeName);
                }
                break;
            }
            case GetMethodOpcode: {
                GetMethod* bc = (GetMethod*)currentCode;
                STORE_ATOMICSTRING_RELOC(m_propertyName);
                break;
            }
            case LoadRegexpOpcode: {
                LoadRegexp* bc = (LoadRegexp*)currentCode;

                String* bodyString = bc->m_body;
                String* optionString = bc->m_option;
                ASSERT(!!bodyString && bodyString->length() > 0);
                ASSERT(!!optionString && optionString->length() > 0);

                size_t bodyIndex = VectorUtil::findInVector(stringLiteralData, bodyString);
                size_t optionIndex = VectorUtil::findInVector(stringLiteralData, optionString);
                ASSERT(bodyIndex != VectorUtil::invalidIndex);
                ASSERT(optionIndex != VectorUtil::invalidIndex);
                relocInfoVector.push_back(ByteCodeRelocInfo(ByteCodeRelocType::RELOC_STRING, (size_t)currentCode - codeBase, bodyIndex));
                relocInfoVector.push_back(ByteCodeRelocInfo(ByteCodeRelocType::RELOC_STRING, (size_t)currentCode - codeBase, optionIndex));
                break;
            }
            case BlockOperationOpcode: {
                BlockOperation* bc = (BlockOperation*)currentCode;
                InterpretedCodeBlock::BlockInfo* info = bc->m_blockInfo;
                size_t infoIndex = VectorUtil::findInVector(block->codeBlock()->m_blockInfos, info);
                ASSERT(infoIndex != VectorUtil::invalidIndex);
                relocInfoVector.push_back(ByteCodeRelocInfo(ByteCodeRelocType::RELOC_BLOCKINFO, (size_t)currentCode - codeBase, infoIndex));
                break;
            }
            case ReplaceBlockLexicalEnvironmentOperationOpcode: {
                ReplaceBlockLexicalEnvironmentOperation* bc = (ReplaceBlockLexicalEnvironmentOperation*)currentCode;
                InterpretedCodeBlock::BlockInfo* info = bc->m_blockInfo;
                size_t infoIndex = VectorUtil::findInVector(block->codeBlock()->m_blockInfos, info);
                ASSERT(infoIndex != VectorUtil::invalidIndex);
                relocInfoVector.push_back(ByteCodeRelocInfo(ByteCodeRelocType::RELOC_BLOCKINFO, (size_t)currentCode - codeBase, infoIndex));
                break;
            }
            case ResolveNameAddressOpcode: {
                ResolveNameAddress* bc = (ResolveNameAddress*)currentCode;
                STORE_ATOMICSTRING_RELOC(m_name);
                break;
            }
            case StoreByNameWithAddressOpcode: {
                StoreByNameWithAddress* bc = (StoreByNameWithAddress*)currentCode;
                STORE_ATOMICSTRING_RELOC(m_name);
                break;
            }
            case GetObjectPreComputedCaseOpcode: {
                GetObjectPreComputedCase* bc = (GetObjectPreComputedCase*)currentCode;
                ASSERT(!bc->m_inlineCache);
                ASSERT(bc->m_propertyName.hasAtomicString());
                STORE_ATOMICSTRING_RELOC(m_propertyName.asAtomicString());
                break;
            }
            case SetObjectPreComputedCaseOpcode: {
                SetObjectPreComputedCase* bc = (SetObjectPreComputedCase*)currentCode;
                ASSERT(!bc->m_inlineCache);
                ASSERT(bc->m_propertyName.hasAtomicString());
                STORE_ATOMICSTRING_RELOC(m_propertyName.asAtomicString());
                break;
            }
            case GetGlobalVariableOpcode: {
                GetGlobalVariable* bc = (GetGlobalVariable*)currentCode;
                ASSERT(!!bc->m_slot);
                STORE_ATOMICSTRING_RELOC(m_slot->m_propertyName);
                break;
            }
            case SetGlobalVariableOpcode: {
                SetGlobalVariable* bc = (SetGlobalVariable*)currentCode;
                ASSERT(!!bc->m_slot);
                STORE_ATOMICSTRING_RELOC(m_slot->m_propertyName);
                break;
            }
            // TODO
            case ThrowStaticErrorOperationOpcode:
            case ExecutionResumeOpcode:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            default:
                break;
            }

            ASSERT(opcode <= EndOpcode);
            code += byteCodeLengths[opcode];
        }
    }

    // store relocInfoVector
    {
        m_buffer.putData(relocInfoVector.data(), relocInfoVector.size());
    }
}

void CodeCacheReader::CacheBuffer::resize(size_t size)
{
    ASSERT(!m_buffer && m_capacity == 0 && m_index == 0);

    m_buffer = static_cast<char*>(malloc(size));
    m_capacity = size;
}

void CodeCacheReader::CacheBuffer::reset()
{
    free(m_buffer);

    m_buffer = nullptr;
    m_capacity = 0;
    m_index = 0;
}

void CodeCacheReader::loadDataFile(FILE* file, size_t size)
{
    m_buffer.resize(size);
    fread((void*)bufferData(), sizeof(char), size, file);
}

InterpretedCodeBlock* CodeCacheReader::loadInterpretedCodeBlock(Context* context, Script* script)
{
    ASSERT(!!context);
    ASSERT(GC_is_disabled());

    size_t size;
    bool hasRareData = m_buffer.get<bool>();

    InterpretedCodeBlock* codeBlock = InterpretedCodeBlock::createInterpretedCodeBlock(context, script, hasRareData);

    {
        // InterpretedCodeBlock::m_src
        size_t start = m_buffer.get<size_t>();
        size_t end = m_buffer.get<size_t>();
        codeBlock->m_src = StringView(script->sourceCode(), start, end);
    }

    // InterpretedCodeBlock::m_parent
    size = m_buffer.get<size_t>();
    codeBlock->m_parent = (InterpretedCodeBlock*)size;

    // InterpretedCodeBlock::m_children
    ASSERT(!codeBlock->hasChildren());
    size = m_buffer.get<size_t>();
    if (size) {
        size_t vectorSize = size;
        InterpretedCodeBlockVector* codeBlockVector = new InterpretedCodeBlockVector();
        codeBlockVector->resizeWithUninitializedValues(vectorSize);
        for (size_t i = 0; i < vectorSize; i++) {
            size_t srcIndex = m_buffer.get<size_t>();
            (*codeBlockVector)[i] = (InterpretedCodeBlock*)srcIndex;
        }
        codeBlock->setChildren(codeBlockVector);
    }

    // InterpretedCodeBlock::m_parameterNames
    AtomicStringTightVector& atomicStringVector = codeBlock->m_parameterNames;
    size = m_buffer.get<size_t>();
    atomicStringVector.resizeWithUninitializedValues(size);
    for (size_t i = 0; i < size; i++) {
        size_t stringIndex = m_buffer.get<size_t>();
        atomicStringVector[i] = m_stringTable->get(stringIndex);
    }

    // InterpretedCodeBlock::m_identifierInfos
    InterpretedCodeBlock::IdentifierInfoVector& identifierVector = codeBlock->m_identifierInfos;
    size = m_buffer.get<size_t>();
    identifierVector.resizeWithUninitializedValues(size);
    for (size_t i = 0; i < size; i++) {
        InterpretedCodeBlock::IdentifierInfo info;
        info.m_needToAllocateOnStack = m_buffer.get<bool>();
        info.m_isMutable = m_buffer.get<bool>();
        info.m_isParameterName = m_buffer.get<bool>();
        info.m_isExplicitlyDeclaredOrParameterName = m_buffer.get<bool>();
        info.m_isVarDeclaration = m_buffer.get<bool>();
        info.m_indexForIndexedStorage = m_buffer.get<size_t>();
        info.m_name = m_stringTable->get(m_buffer.get<size_t>());
        identifierVector[i] = info;
    }

    // InterpretedCodeBlock::m_blockInfos
    InterpretedCodeBlock::BlockInfoVector& blockInfoVector = codeBlock->m_blockInfos;
    size = m_buffer.get<size_t>();
    blockInfoVector.resizeWithUninitializedValues(size);
    for (size_t i = 0; i < size; i++) {
        InterpretedCodeBlock::BlockInfo* info = new InterpretedCodeBlock::BlockInfo(
#ifndef NDEBUG
            ExtendedNodeLOC(0, 0, 0)
#endif
                );

        info->m_canAllocateEnvironmentOnStack = m_buffer.get<bool>();
        info->m_shouldAllocateEnvironment = m_buffer.get<bool>();
        info->m_nodeType = (ASTNodeType)m_buffer.get<uint16_t>();
        info->m_parentBlockIndex = (LexicalBlockIndex)m_buffer.get<uint16_t>();
        info->m_blockIndex = (LexicalBlockIndex)m_buffer.get<uint16_t>();

        size_t vectorSize = m_buffer.get<size_t>();
        info->m_identifiers.resizeWithUninitializedValues(vectorSize);
        for (size_t j = 0; j < vectorSize; j++) {
            InterpretedCodeBlock::BlockIdentifierInfo idInfo;
            idInfo.m_needToAllocateOnStack = m_buffer.get<bool>();
            idInfo.m_isMutable = m_buffer.get<bool>();
            idInfo.m_indexForIndexedStorage = m_buffer.get<size_t>();
            idInfo.m_name = m_stringTable->get(m_buffer.get<size_t>());
            info->m_identifiers[j] = idInfo;
        }

        blockInfoVector[i] = info;
    }

    // InterpretedCodeBlock::m_functionName
    codeBlock->m_functionName = m_stringTable->get(m_buffer.get<size_t>());

    // InterpretedCodeBlock::m_functionStart
    codeBlock->m_functionStart = m_buffer.get<ExtendedNodeLOC>();

#ifndef NDEBUG
    // InterpretedCodeBlock::m_bodyEndLOC
    codeBlock->m_bodyEndLOC = m_buffer.get<ExtendedNodeLOC>();
#endif

    codeBlock->m_functionLength = m_buffer.get<uint16_t>();
    codeBlock->m_parameterCount = m_buffer.get<uint16_t>();
    codeBlock->m_identifierOnStackCount = m_buffer.get<uint16_t>();
    codeBlock->m_identifierOnHeapCount = m_buffer.get<uint16_t>();
    codeBlock->m_lexicalBlockStackAllocatedIdentifierMaximumDepth = m_buffer.get<uint16_t>();

    codeBlock->m_functionBodyBlockIndex = m_buffer.get<LexicalBlockIndex>();
    codeBlock->m_lexicalBlockIndexFunctionLocatedIn = m_buffer.get<LexicalBlockIndex>();

    codeBlock->m_isFunctionNameSaveOnHeap = m_buffer.get<bool>();
    codeBlock->m_isFunctionNameExplicitlyDeclared = m_buffer.get<bool>();
    codeBlock->m_canUseIndexedVariableStorage = m_buffer.get<bool>();
    codeBlock->m_canAllocateVariablesOnStack = m_buffer.get<bool>();
    codeBlock->m_canAllocateEnvironmentOnStack = m_buffer.get<bool>();
    codeBlock->m_hasDescendantUsesNonIndexedVariableStorage = m_buffer.get<bool>();
    codeBlock->m_hasEval = m_buffer.get<bool>();
    codeBlock->m_hasWith = m_buffer.get<bool>();
    codeBlock->m_isStrict = m_buffer.get<bool>();
    codeBlock->m_inWith = m_buffer.get<bool>();
    codeBlock->m_isEvalCode = m_buffer.get<bool>();
    codeBlock->m_isEvalCodeInFunction = m_buffer.get<bool>();
    codeBlock->m_usesArgumentsObject = m_buffer.get<bool>();
    codeBlock->m_isFunctionExpression = m_buffer.get<bool>();
    codeBlock->m_isFunctionDeclaration = m_buffer.get<bool>();
    codeBlock->m_isArrowFunctionExpression = m_buffer.get<bool>();
    codeBlock->m_isOneExpressionOnlyArrowFunctionExpression = m_buffer.get<bool>();
    codeBlock->m_isClassConstructor = m_buffer.get<bool>();
    codeBlock->m_isDerivedClassConstructor = m_buffer.get<bool>();
    codeBlock->m_isObjectMethod = m_buffer.get<bool>();
    codeBlock->m_isClassMethod = m_buffer.get<bool>();
    codeBlock->m_isClassStaticMethod = m_buffer.get<bool>();
    codeBlock->m_isGenerator = m_buffer.get<bool>();
    codeBlock->m_isAsync = m_buffer.get<bool>();
    codeBlock->m_needsVirtualIDOperation = m_buffer.get<bool>();
    codeBlock->m_hasArrowParameterPlaceHolder = m_buffer.get<bool>();
    codeBlock->m_hasParameterOtherThanIdentifier = m_buffer.get<bool>();
    codeBlock->m_allowSuperCall = m_buffer.get<bool>();
    codeBlock->m_allowSuperProperty = m_buffer.get<bool>();

    // InterpretedCodeBlockWithRareData
    if (UNLIKELY(hasRareData)) {
        ASSERT(codeBlock->hasRareData());
        ASSERT(codeBlock->taggedTemplateLiteralCache().size() == 0);
        ASSERT(!codeBlock->identifierInfoMap());
        FunctionContextVarMap* varMap = new (GC) FunctionContextVarMap();
        size_t mapSize = m_buffer.get<size_t>();
        for (size_t i = 0; i < mapSize; i++) {
            size_t stringIndex = m_buffer.get<size_t>();
            size_t index = m_buffer.get<size_t>();
            m_stringTable->get(stringIndex);
            varMap->insert(std::make_pair(m_stringTable->get(stringIndex), index));
        }
        codeBlock->rareData()->m_identifierInfoMap = varMap;
    }

    return codeBlock;
}

ByteCodeBlock* CodeCacheReader::loadByteCodeBlock(Context* context, Script* script)
{
    ASSERT(GC_is_disabled());
    ASSERT(!!script->topCodeBlock());

    size_t size;
    ByteCodeBlock* block = new ByteCodeBlock(script->topCodeBlock());

    block->m_isEvalMode = m_buffer.get<bool>();
    block->m_isOnGlobal = m_buffer.get<bool>();
    block->m_shouldClearStack = m_buffer.get<bool>();
    block->m_isOwnerMayFreed = m_buffer.get<bool>();
    block->m_requiredRegisterFileSizeInValueSize = m_buffer.get<uint16_t>();

    // ByteCodeBlock::m_numeralLiteralData
    ByteCodeNumeralLiteralData& numeralLiteralData = block->m_numeralLiteralData;
    size = m_buffer.get<size_t>();
    numeralLiteralData.resizeWithUninitializedValues(size);
    m_buffer.getData(numeralLiteralData.data(), size);

    // ByteCodeBlock::m_stringLiteralData
    ByteCodeStringLiteralData& stringLiteralData = block->m_stringLiteralData;
    size = m_buffer.get<size_t>();
    stringLiteralData.resizeWithUninitializedValues(size);
    for (size_t i = 0; i < size; i++) {
        stringLiteralData[i] = m_buffer.getString();
    }

    // ByteCodeBlock::m_otherLiteralData
    ByteCodeOtherLiteralData& otherLiteralData = block->m_otherLiteralData;
    size = m_buffer.get<size_t>();
    otherLiteralData.resizeWithUninitializedValues(size);
    for (size_t i = 0; i < size; i++) {
        ControlFlowRecord* record = new ControlFlowRecord(ControlFlowRecord::ControlFlowReason::NeedsJump, SIZE_MAX, SIZE_MAX);
        m_buffer.getData(record, 1);
        otherLiteralData[i] = record;
    }

    // ByteCodeBlock::m_code bytecode stream
    loadByteCodeStream(context, block);

    // finally, relocate opcode address and register index for each bytecode
    ByteCodeGenerator::relocateByteCode(block);

    return block;
}

#define LOAD_ATOMICSTRING_RELOC(member)   \
    size_t stringIndex = info.dataOffset; \
    bc->member = m_stringTable->get(stringIndex);

void CodeCacheReader::loadByteCodeStream(Context* context, ByteCodeBlock* block)
{
    ByteCodeBlockData& byteCodeStream = block->m_code;

    // load ByteCode stream
    {
        size_t codeSize = m_buffer.get<size_t>();
        byteCodeStream.resizeWithUninitializedValues(codeSize);
        m_buffer.getData(byteCodeStream.data(), codeSize);
    }

    Vector<ByteCodeRelocInfo, std::allocator<ByteCodeRelocInfo>> relocInfoVector;
    ByteCodeStringLiteralData& stringLiteralData = block->m_stringLiteralData;

    // load relocInfoVector
    {
        size_t relocSize = m_buffer.get<size_t>();
        relocInfoVector.resizeWithUninitializedValues(relocSize);
        m_buffer.getData(relocInfoVector.data(), relocSize);
    }

    // relocate ByteCodeStream
    {
        char* code = byteCodeStream.data();
        size_t codeBase = (size_t)code;
        char* end = code + byteCodeStream.size();

        InterpretedCodeBlock* codeBlock = block->codeBlock();

        // mark for LoadRegexp bytecode
        bool bodyStringForLoadRegexp = true;

        for (size_t i = 0; i < relocInfoVector.size(); i++) {
            ByteCodeRelocInfo& info = relocInfoVector[i];
            ByteCode* currentCode = (ByteCode*)(code + info.codeOffset);

#if defined(COMPILER_GCC) || defined(COMPILER_CLANG)
            Opcode opcode = (Opcode)(size_t)currentCode->m_opcodeInAddress;
#else
            Opcode opcode = currentCode->m_opcode;
#endif
            switch (opcode) {
            case LoadLiteralOpcode: {
                LoadLiteral* bc = (LoadLiteral*)currentCode;
                size_t stringIndex = info.dataOffset;

                if (info.relocType == ByteCodeRelocType::RELOC_STRING) {
                    ASSERT(stringIndex < stringLiteralData.size());
                    String* string = stringLiteralData[stringIndex];
                    bc->m_value = Value(string);
                } else {
                    ASSERT(info.relocType == ByteCodeRelocType::RELOC_ATOMICSTRING);
                    bc->m_value = Value(m_stringTable->get(stringIndex).string());
                }
                break;
            }
            case LoadByNameOpcode: {
                LoadByName* bc = (LoadByName*)currentCode;
                LOAD_ATOMICSTRING_RELOC(m_name);
                break;
            }
            case StoreByNameOpcode: {
                StoreByName* bc = (StoreByName*)currentCode;
                LOAD_ATOMICSTRING_RELOC(m_name);
                break;
            }
            case InitializeByNameOpcode: {
                InitializeByName* bc = (InitializeByName*)currentCode;
                LOAD_ATOMICSTRING_RELOC(m_name);
                break;
            }
            case CreateFunctionOpcode: {
                CreateFunction* bc = (CreateFunction*)currentCode;
                InterpretedCodeBlockVector& children = codeBlock->children();
                size_t childIndex = info.dataOffset;
                ASSERT(childIndex < children.size());
                bc->m_codeBlock = children[childIndex];
                break;
            }
            case CreateClassOpcode: {
                CreateClass* bc = (CreateClass*)currentCode;

                if (info.relocType == ByteCodeRelocType::RELOC_CODEBLOCK) {
                    InterpretedCodeBlockVector& children = codeBlock->children();
                    size_t childIndex = info.dataOffset;
                    ASSERT(childIndex < children.size());
                    bc->m_codeBlock = children[childIndex];
                } else {
                    ASSERT(info.relocType == ByteCodeRelocType::RELOC_STRING);
                    size_t stringIndex = info.dataOffset;
                    ASSERT(stringIndex < stringLiteralData.size());
                    bc->m_classSrc = stringLiteralData[stringIndex];
                }
                break;
            }
            case ObjectDefineOwnPropertyWithNameOperationOpcode: {
                ObjectDefineOwnPropertyWithNameOperation* bc = (ObjectDefineOwnPropertyWithNameOperation*)currentCode;
                LOAD_ATOMICSTRING_RELOC(m_propertyName);
                break;
            }
            case InitializeGlobalVariableOpcode: {
                InitializeGlobalVariable* bc = (InitializeGlobalVariable*)currentCode;
                LOAD_ATOMICSTRING_RELOC(m_variableName);
                break;
            }
            case UnaryTypeofOpcode: {
                UnaryTypeof* bc = (UnaryTypeof*)currentCode;
                LOAD_ATOMICSTRING_RELOC(m_id);
                break;
            }
            case UnaryDeleteOpcode: {
                UnaryDelete* bc = (UnaryDelete*)currentCode;
                LOAD_ATOMICSTRING_RELOC(m_id);
                break;
            }
            case JumpComplexCaseOpcode: {
                JumpComplexCase* bc = (JumpComplexCase*)currentCode;
                size_t recordIndex = info.dataOffset;
                ByteCodeOtherLiteralData& otherLiteralData = block->m_otherLiteralData;
                ASSERT(recordIndex < otherLiteralData.size());
                bc->m_controlFlowRecord = (ControlFlowRecord*)otherLiteralData[recordIndex];
                break;
            }
            case CallFunctionComplexCaseOpcode: {
                CallFunctionComplexCase* bc = (CallFunctionComplexCase*)currentCode;
                ASSERT(bc->m_kind == CallFunctionComplexCase::InWithScope);
                LOAD_ATOMICSTRING_RELOC(m_calleeName);
                break;
            }
            case GetMethodOpcode: {
                GetMethod* bc = (GetMethod*)currentCode;
                LOAD_ATOMICSTRING_RELOC(m_propertyName);
                break;
            }
            case LoadRegexpOpcode: {
                LoadRegexp* bc = (LoadRegexp*)currentCode;

                String* bodyString = bc->m_body;
                String* optionString = bc->m_option;

                if (bodyStringForLoadRegexp) {
                    ASSERT(info.dataOffset < stringLiteralData.size());
                    bc->m_body = stringLiteralData[info.dataOffset];
                    bodyStringForLoadRegexp = false;
                } else {
                    ASSERT(info.dataOffset < stringLiteralData.size());
                    bc->m_option = stringLiteralData[info.dataOffset];
                    bodyStringForLoadRegexp = true;
                }
                break;
            }
            case BlockOperationOpcode: {
                BlockOperation* bc = (BlockOperation*)currentCode;
                size_t blockIndex = info.dataOffset;
                ASSERT(blockIndex < codeBlock->m_blockInfos.size());
                bc->m_blockInfo = codeBlock->m_blockInfos[blockIndex];
                break;
            }
            case ReplaceBlockLexicalEnvironmentOperationOpcode: {
                ReplaceBlockLexicalEnvironmentOperation* bc = (ReplaceBlockLexicalEnvironmentOperation*)currentCode;
                size_t blockIndex = info.dataOffset;
                ASSERT(blockIndex < codeBlock->m_blockInfos.size());
                bc->m_blockInfo = codeBlock->m_blockInfos[blockIndex];
                break;
            }
            case ResolveNameAddressOpcode: {
                ResolveNameAddress* bc = (ResolveNameAddress*)currentCode;
                LOAD_ATOMICSTRING_RELOC(m_name);
                break;
            }
            case StoreByNameWithAddressOpcode: {
                StoreByNameWithAddress* bc = (StoreByNameWithAddress*)currentCode;
                LOAD_ATOMICSTRING_RELOC(m_name);
                break;
            }
            case GetObjectPreComputedCaseOpcode: {
                GetObjectPreComputedCase* bc = (GetObjectPreComputedCase*)currentCode;
                ASSERT(!bc->m_inlineCache);
                LOAD_ATOMICSTRING_RELOC(m_propertyName);
                break;
            }
            case SetObjectPreComputedCaseOpcode: {
                SetObjectPreComputedCase* bc = (SetObjectPreComputedCase*)currentCode;
                ASSERT(!bc->m_inlineCache);
                LOAD_ATOMICSTRING_RELOC(m_propertyName);
                break;
            }
            case GetGlobalVariableOpcode: {
                GetGlobalVariable* bc = (GetGlobalVariable*)currentCode;
                size_t stringIndex = info.dataOffset;
                bc->m_slot = context->ensureGlobalVariableAccessCacheSlot(m_stringTable->get(stringIndex));
                break;
            }
            case SetGlobalVariableOpcode: {
                SetGlobalVariable* bc = (SetGlobalVariable*)currentCode;
                size_t stringIndex = info.dataOffset;
                bc->m_slot = context->ensureGlobalVariableAccessCacheSlot(m_stringTable->get(stringIndex));
                break;
            }
            default:
                RELEASE_ASSERT_NOT_REACHED();
                break;
            }
        }
    }
}
}

#endif // ENABLE_CODE_CACHE
