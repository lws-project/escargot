/*
 * Copyright (c) 2016-present Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
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

#ifndef __EscargotByteCodeInterpreter__
#define __EscargotByteCodeInterpreter__

#include "parser/CodeBlock.h"
#include "runtime/ExecutionState.h"
#include "runtime/String.h"
#include "runtime/Value.h"

namespace Escargot {

class Context;
class ByteCodeBlock;
class LexicalEnvironment;
struct GetObjectInlineCache;
struct SetObjectInlineCache;
struct EnumerateObjectData;
struct GlobalVariableAccessCacheItem;
class InitializeGlobalVariable;
class CallFunctionInWithScope;
class CallEvalFunction;
class CreateFunction;
class CreateClass;
class SuperReference;
class CallSuper;
class WithOperation;
class BlockOperation;
class TryOperation;
class UnaryDelete;
class TemplateOperation;
class DeclareFunctionDeclarations;
class ObjectDefineGetter;
class ObjectDefineSetter;
class GlobalObject;

class ByteCodeInterpreter {
public:
    static Value interpret(ExecutionState& state, ByteCodeBlock* byteCodeBlock, size_t programCounter, Value* registerFile);
    static Value loadByName(ExecutionState& state, LexicalEnvironment* env, const AtomicString& name, bool throwException = true);
    static EnvironmentRecord* getBindedEnvironmentRecordByName(ExecutionState& state, LexicalEnvironment* env, const AtomicString& name, Value& bindedValue, bool throwException = true);
    static void storeByName(ExecutionState& state, LexicalEnvironment* env, const AtomicString& name, const Value& value);
    static void initializeByName(ExecutionState& state, LexicalEnvironment* env, const AtomicString& name, bool isLexicallyDeclaredName, const Value& value);
    static Value plusSlowCase(ExecutionState& state, const Value& a, const Value& b);
    static Value modOperation(ExecutionState& state, const Value& left, const Value& right);
    static Value instanceOfOperation(ExecutionState& state, const Value& left, const Value& right);
    static void deleteOperation(ExecutionState& state, LexicalEnvironment* env, UnaryDelete* code, Value* registerFile);
    static void templateOperation(ExecutionState& state, LexicalEnvironment* env, TemplateOperation* code, Value* registerFile);

    // http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
    static bool abstractRelationalComparisonSlowCase(ExecutionState& state, const Value& left, const Value& right, bool leftFirst);
    static bool abstractRelationalComparison(ExecutionState& state, const Value& left, const Value& right, bool leftFirst);
    static bool abstractRelationalComparisonOrEqualSlowCase(ExecutionState& state, const Value& left, const Value& right, bool leftFirst);
    static bool abstractRelationalComparisonOrEqual(ExecutionState& state, const Value& left, const Value& right, bool leftFirst);

    static Value getObjectPrecomputedCaseOperation(ExecutionState& state, Object* obj, const Value& receiver, const PropertyName& name, GetObjectInlineCache& inlineCache, ByteCodeBlock* block);
    static Value getObjectPrecomputedCaseOperationCacheMiss(ExecutionState& state, Object* obj, const Value& receiver, const PropertyName& name, GetObjectInlineCache& inlineCache, ByteCodeBlock* block);
    static void setObjectPreComputedCaseOperation(ExecutionState& state, const Value& willBeObject, const PropertyName& name, const Value& value, SetObjectInlineCache& inlineCache, ByteCodeBlock* block);
    static void setObjectPreComputedCaseOperationCacheMiss(ExecutionState& state, Object* obj, const Value& willBeObject, const PropertyName& name, const Value& value, SetObjectInlineCache& inlineCache, ByteCodeBlock* block);

    static EnumerateObjectData* executeEnumerateObject(ExecutionState& state, Object* obj);
    static EnumerateObjectData* updateEnumerateObjectData(ExecutionState& state, EnumerateObjectData* data);

    static Object* fastToObject(ExecutionState& state, const Value& obj);

    static Value getGlobalVariableSlowCase(ExecutionState& state, Object* go, GlobalVariableAccessCacheItem* slot, ByteCodeBlock* block);
    static void setGlobalVariableSlowCase(ExecutionState& state, Object* go, GlobalVariableAccessCacheItem* slot, const Value& value, ByteCodeBlock* block);
    static void initializeGlobalVariable(ExecutionState& state, InitializeGlobalVariable* code, const Value& value);

    static size_t tryOperation(ExecutionState& state, TryOperation* code, LexicalEnvironment* env, size_t programCounter, ByteCodeBlock* byteCodeBlock, Value* registerFile);

    static void createFunctionOperation(ExecutionState& state, CreateFunction* createFunction, ByteCodeBlock* byteCodeBlock, Value* registerFile);
    static ArrayObject* createRestElementOperation(ExecutionState& state, EnvironmentRecord* record, ByteCodeBlock* byteCodeBlock);
    static void evalOperation(ExecutionState& state, CallEvalFunction* code, Value* registerFile, ByteCodeBlock* byteCodeBlock);
    static void classOperation(ExecutionState& state, CreateClass* code, Value* registerFile);
    static void superOperation(ExecutionState& state, SuperReference* code, Value* registerFile);
    static void callSuperOperation(ExecutionState& state, CallSuper* code, Value* registerFile);
    static Value withOperation(ExecutionState& state, WithOperation* code, Object* obj, LexicalEnvironment* env, size_t& programCounter, ByteCodeBlock* byteCodeBlock, Value* registerFile, Value* stackStorage);
    static Value blockOperation(ExecutionState& state, BlockOperation* code, size_t& programCounter, ByteCodeBlock* byteCodeBlock, Value* registerFile, Value* stackStorage);
    static bool binaryInOperation(ExecutionState& state, const Value& left, const Value& right);
    static Value callFunctionInWithScope(ExecutionState& state, CallFunctionInWithScope* code, LexicalEnvironment* env, Value* argv);
    static void spreadFunctionArguments(ExecutionState& state, const Value* argv, const size_t argc, ValueVector& argVector);
    static Value yieldDelegateOperation(ExecutionState& state, Value* registerFile, size_t& programCounter, char* codeBuffer);

    static void defineObjectGetter(ExecutionState& state, ObjectDefineGetter* code, Value* registerFile);
    static void defineObjectSetter(ExecutionState& state, ObjectDefineSetter* code, Value* registerFile);
    static Value incrementOperation(ExecutionState& state, const Value& value);
    static Value decrementOperation(ExecutionState& state, const Value& value);

    static void processException(ExecutionState& state, const Value& value, CodeBlock* codeBlock, size_t programCounter);
};
}

#endif
