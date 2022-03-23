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

#include "Escargot.h"
#include "SandBox.h"
#include "runtime/Context.h"
#include "runtime/Environment.h"
#include "runtime/EnvironmentRecord.h"
#include "runtime/NativeFunctionObject.h"
#include "runtime/VMInstance.h"
#include "parser/Script.h"
#include "interpreter/ByteCode.h"
#include "interpreter/ByteCodeInterpreter.h"

namespace Escargot {

SandBox::SandBox(Context* s)
    : m_context(s)
{
    m_oldSandBox = m_context->vmInstance()->m_currentSandBox;
    m_context->vmInstance()->m_currentSandBox = this;
}

SandBox::~SandBox()
{
    ASSERT(m_context->vmInstance()->m_currentSandBox == this);
    m_context->vmInstance()->m_currentSandBox = m_oldSandBox;
}

void SandBox::processCatch(const Value& error, SandBoxResult& result)
{
    // when exception occurred, an undefined value is allocated for result value which will be never used.
    // this is to avoid dereferencing of null pointer.
    result.result = Value();
    result.error = error;

    fillStackDataIntoErrorObject(error);

#ifdef ESCARGOT_DEBUGGER
    Debugger* debugger = m_context->debugger();
    Debugger::SavedStackTraceDataVector exceptionTrace;
#endif /* ESCARGOT_DEBUGGER */

    ByteCodeLOCDataMap locMap;
    for (size_t i = 0; i < m_stackTraceDataVector.size(); i++) {
        if ((size_t)m_stackTraceDataVector[i].loc.index == SIZE_MAX && (size_t)m_stackTraceDataVector[i].loc.actualCodeBlock != SIZE_MAX) {
            // this means loc not computed yet.
            StackTraceData traceData = m_stackTraceDataVector[i];
            ByteCodeBlock* block = traceData.loc.actualCodeBlock;

            ByteCodeLOCData* locData;
            auto iterMap = locMap.find(block);
            if (iterMap == locMap.end()) {
                locData = new ByteCodeLOCData();
                locMap.insert(std::make_pair(block, locData));
            } else {
                locData = iterMap->second;
            }

            ExtendedNodeLOC loc = block->computeNodeLOCFromByteCode(m_context,
                                                                    traceData.loc.byteCodePosition, block->m_codeBlock, locData);

            traceData.loc = loc;
            result.stackTrace.pushBack(traceData);

#ifdef ESCARGOT_DEBUGGER
            if (i < 8 && debugger != nullptr) {
                exceptionTrace.pushBack(Debugger::SavedStackTraceData(block, (uint32_t)loc.line, (uint32_t)loc.column));
            }
#endif /* ESCARGOT_DEBUGGER */
        } else {
            result.stackTrace.pushBack(m_stackTraceDataVector[i]);
        }
    }
    for (auto iter = locMap.begin(); iter != locMap.end(); iter++) {
        delete iter->second;
    }

#ifdef ESCARGOT_DEBUGGER
    if (debugger != nullptr) {
        ExecutionState state(m_context);
        String* message = error.toStringWithoutException(state);

        debugger->exceptionCaught(message, exceptionTrace);
    }
#endif /* ESCARGOT_DEBUGGER */
}

SandBox::SandBoxResult SandBox::run(Value (*scriptRunner)(ExecutionState&, void*), void* data)
{
    SandBox::SandBoxResult result;
    try {
        ExecutionState state(m_context);
        result.result = scriptRunner(state, data);
    } catch (const Value& err) {
        processCatch(err, result);
    }
    return result;
}

SandBox::SandBoxResult SandBox::run(const std::function<Value()>& scriptRunner)
{
    SandBox::SandBoxResult result;

    try {
        result.result = scriptRunner();
    } catch (const Value& err) {
        processCatch(err, result);
    }
    return result;
}

bool SandBox::createStackTrace(StackTraceDataVector& stackTraceDataVector, ExecutionState& state, bool stopAtPause)
{
    UNUSED_VARIABLE(stopAtPause);

    ExecutionState* pstate = &state;
#ifdef ESCARGOT_DEBUGGER
    uint32_t executionStateDepthIndex = 0;
    ExecutionState* activeSavedStackTraceExecutionState = nullptr;

    if (stopAtPause && state.context()->debuggerEnabled()) {
        activeSavedStackTraceExecutionState = state.context()->debugger()->activeSavedStackTraceExecutionState();
    }
#endif /* ESCARGOT_DEBUGGER */

    std::vector<ExecutionState*> stateStack;

    while (pstate) {
        FunctionObject* callee = pstate->resolveCallee();
        ExecutionState* es = pstate;

        while (es) {
            if (!es->lexicalEnvironment() || !es->lexicalEnvironment()->record()) {
                break;
            }
            if (es->lexicalEnvironment()->record()->isGlobalEnvironmentRecord()) {
                break;
            } else if (es->lexicalEnvironment()->record()->isDeclarativeEnvironmentRecord() && es->lexicalEnvironment()->record()->asDeclarativeEnvironmentRecord()->isFunctionEnvironmentRecord()) {
                break;
            } else if (es->lexicalEnvironment()->record()->isModuleEnvironmentRecord()) {
                break;
            }
            es = es->parent();
        }

        if (!es) {
            break;
        }

        bool alreadyExists = false;

        for (size_t i = 0; i < stateStack.size(); i++) {
            if (stateStack[i] == es || stateStack[i]->lexicalEnvironment() == es->lexicalEnvironment()) {
                alreadyExists = true;
                break;
            }
        }

        if (!alreadyExists) {
            if (!callee && es && es->lexicalEnvironment()) {
                // can be null on module outer env
                InterpretedCodeBlock* cb = nullptr;
                if (es->lexicalEnvironment()->record()->isGlobalEnvironmentRecord()) {
                    cb = es->lexicalEnvironment()->record()->asGlobalEnvironmentRecord()->globalCodeBlock();
                } else {
                    ASSERT(es->lexicalEnvironment()->record()->isModuleEnvironmentRecord());
                    cb = es->lexicalEnvironment()->outerEnvironment()->record()->asGlobalEnvironmentRecord()->globalCodeBlock();
                }
                if (cb) {
                    ByteCodeBlock* b = cb->byteCodeBlock();
                    ExtendedNodeLOC loc(SIZE_MAX, SIZE_MAX, SIZE_MAX);
                    ASSERT(!pstate->isNativeFunctionObjectExecutionContext());
                    if (pstate->m_programCounter != nullptr) {
                        loc.byteCodePosition = *pstate->m_programCounter - (size_t)b->m_code.data();
                        loc.actualCodeBlock = b;
                    }
                    SandBox::StackTraceData data;
                    data.loc = loc;
                    data.srcName = cb->script()->srcName();
                    data.sourceCode = cb->script()->sourceCode();
                    data.isEval = true;
                    data.isFunction = false;
                    data.isAssociatedWithJavaScriptCode = true;
                    data.isConstructor = false;
#ifdef ESCARGOT_DEBUGGER
                    data.executionStateDepth = executionStateDepthIndex;
#endif /* ESCARGOT_DEBUGGER */

                    stateStack.push_back(es);
                    stackTraceDataVector.pushBack(data);
                }
            } else if (pstate->codeBlock() && pstate->codeBlock()->isInterpretedCodeBlock() && pstate->codeBlock()->asInterpretedCodeBlock()->isEvalCodeInFunction()) {
                CodeBlock* cb = pstate->codeBlock();
                ExtendedNodeLOC loc(SIZE_MAX, SIZE_MAX, SIZE_MAX);
                SandBox::StackTraceData data;
                data.loc = loc;
                data.srcName = cb->asInterpretedCodeBlock()->script()->srcName();
                data.sourceCode = String::emptyString;
                data.isEval = true;
                data.isFunction = false;
                data.isAssociatedWithJavaScriptCode = true;
                data.isConstructor = false;
#ifdef ESCARGOT_DEBUGGER
                data.executionStateDepth = executionStateDepthIndex;
#endif /* ESCARGOT_DEBUGGER */

                stateStack.push_back(es);
                stackTraceDataVector.pushBack(data);
            } else if (callee) {
                CodeBlock* cb = callee->codeBlock();
                ExtendedNodeLOC loc(SIZE_MAX, SIZE_MAX, SIZE_MAX);
                if (cb->isInterpretedCodeBlock()) {
                    ByteCodeBlock* b = cb->asInterpretedCodeBlock()->byteCodeBlock();
                    ASSERT(!pstate->isNativeFunctionObjectExecutionContext());
                    if (pstate->m_programCounter != nullptr) {
                        loc.byteCodePosition = *pstate->m_programCounter - (size_t)b->m_code.data();
                        loc.actualCodeBlock = b;
                    }
                }

                SandBox::StackTraceData data;
                data.loc = loc;

                if (cb->isInterpretedCodeBlock() && cb->asInterpretedCodeBlock()->script()) {
                    data.srcName = cb->asInterpretedCodeBlock()->script()->srcName();
                    data.sourceCode = cb->asInterpretedCodeBlock()->script()->sourceCode();
                } else {
                    StringBuilder builder;
                    builder.appendString("function ");
                    builder.appendString(cb->functionName().string());
                    builder.appendString("() { ");
                    builder.appendString("[native function]");
                    builder.appendString(" } ");
                    data.srcName = builder.finalize();
                    data.sourceCode = String::emptyString;
                }

                data.functionName = cb->functionName().string();
                data.isEval = false;
                data.isFunction = true;
                data.callee = callee;
                data.isAssociatedWithJavaScriptCode = cb->isInterpretedCodeBlock();
                data.isConstructor = callee->isConstructor();
#ifdef ESCARGOT_DEBUGGER
                data.executionStateDepth = executionStateDepthIndex;
#endif /* ESCARGOT_DEBUGGER */

                stateStack.push_back(es);
                stackTraceDataVector.pushBack(data);
            }
        }

#ifdef ESCARGOT_DEBUGGER
        if (pstate == activeSavedStackTraceExecutionState) {
            return true;
        }
#endif /* ESCARGOT_DEBUGGER */

        pstate = pstate->parent();
#ifdef ESCARGOT_DEBUGGER
        executionStateDepthIndex++;
#endif /* ESCARGOT_DEBUGGER */
    }

    return false;
}

void SandBox::throwException(ExecutionState& state, Value exception)
{
    m_stackTraceDataVector.clear();
    createStackTrace(m_stackTraceDataVector, state);

    // We MUST save thrown exception Value.
    // because bdwgc cannot track `thrown value`(may turned off by GC_DONT_REGISTER_MAIN_STATIC_DATA)
    m_exception = exception;
    throw exception;
}

void SandBox::rethrowPreviouslyCaughtException(ExecutionState& state, Value exception, const StackTraceDataVector& stackTraceDataVector)
{
    m_stackTraceDataVector = stackTraceDataVector;
    // update stack trace data if needs
    createStackTrace(m_stackTraceDataVector, state);

    // We MUST save thrown exception Value.
    // because bdwgc cannot track `thrown value`(may turned off by GC_DONT_REGISTER_MAIN_STATIC_DATA)
    m_exception = exception;
    throw exception;
}

static Value builtinErrorObjectStackInfo(ExecutionState& state, Value thisValue, size_t argc, Value* argv, Optional<Object*> newTarget)
{
    if (!(LIKELY(thisValue.isPointerValue() && thisValue.asPointerValue()->isErrorObject()))) {
        ErrorObject::throwBuiltinError(state, ErrorObject::TypeError, "get Error.prototype.stack called on incompatible receiver");
    }

    ErrorObject* obj = thisValue.asObject()->asErrorObject();
    if (obj->stackTraceData() == nullptr) {
        return String::emptyString;
    }

    auto stackTraceData = obj->stackTraceData();
    StringBuilder builder;
    stackTraceData->buildStackTrace(state.context(), builder);
    return builder.finalize();
}

ErrorObject::StackTraceData* ErrorObject::StackTraceData::create(SandBox* sandBox)
{
    ErrorObject::StackTraceData* data = new ErrorObject::StackTraceData();
    data->gcValues.resizeWithUninitializedValues(sandBox->m_stackTraceDataVector.size());
    data->nonGCValues.resizeWithUninitializedValues(sandBox->m_stackTraceDataVector.size());
    data->exception = sandBox->m_exception;

    for (size_t i = 0; i < sandBox->m_stackTraceDataVector.size(); i++) {
        if ((size_t)sandBox->m_stackTraceDataVector[i].loc.index == SIZE_MAX && (size_t)sandBox->m_stackTraceDataVector[i].loc.actualCodeBlock != SIZE_MAX) {
            data->gcValues[i].byteCodeBlock = sandBox->m_stackTraceDataVector[i].loc.actualCodeBlock;
            data->nonGCValues[i].byteCodePosition = sandBox->m_stackTraceDataVector[i].loc.byteCodePosition;
        } else {
            data->gcValues[i].infoString = sandBox->m_stackTraceDataVector[i].srcName;
            data->nonGCValues[i].byteCodePosition = SIZE_MAX;
        }
    }

    return data;
}

void ErrorObject::StackTraceData::buildStackTrace(Context* context, StringBuilder& builder)
{
    if (exception.isObject()) {
        ExecutionState state(context);
        SandBox sb(context);
        sb.run([&]() -> Value {
            auto getResult = exception.asObject()->get(state, state.context()->staticStrings().name);
            if (getResult.hasValue()) {
                builder.appendString(getResult.value(state, exception.asObject()).toString(state));
                builder.appendString(": ");
            }
            getResult = exception.asObject()->get(state, state.context()->staticStrings().message);
            if (getResult.hasValue()) {
                builder.appendString(getResult.value(state, exception.asObject()).toString(state));
                builder.appendChar('\n');
            }
            return Value();
        });
    }

    ByteCodeLOCDataMap locMap;
    for (size_t i = 0; i < gcValues.size(); i++) {
        builder.appendString("at ");
        if (nonGCValues[i].byteCodePosition == SIZE_MAX) {
            builder.appendString(gcValues[i].infoString);
        } else {
            ByteCodeBlock* block = gcValues[i].byteCodeBlock;

            ByteCodeLOCData* locData;
            auto iterMap = locMap.find(block);
            if (iterMap == locMap.end()) {
                locData = new ByteCodeLOCData();
                locMap.insert(std::make_pair(block, locData));
            } else {
                locData = iterMap->second;
            }

            ExtendedNodeLOC loc = gcValues[i].byteCodeBlock->computeNodeLOCFromByteCode(context,
                                                                                        nonGCValues[i].byteCodePosition, block->m_codeBlock, locData);

            builder.appendString(block->m_codeBlock->script()->srcName());
            builder.appendChar(':');
            builder.appendString(String::fromDouble(loc.line));
            builder.appendChar(':');
            builder.appendString(String::fromDouble(loc.column));

            String* src = block->m_codeBlock->script()->sourceCode();
            if (src->length()) {
                const size_t preLineMax = 40;
                const size_t afterLineMax = 40;

                size_t preLineSoFar = 0;
                size_t afterLineSoFar = 0;

                size_t start = loc.index;
                int64_t idx = (int64_t)start;
                while (start - idx < preLineMax) {
                    if (idx == 0) {
                        break;
                    }
                    if (src->charAt((size_t)idx) == '\r' || src->charAt((size_t)idx) == '\n') {
                        idx++;
                        break;
                    }
                    idx--;
                }
                preLineSoFar = idx;

                idx = start;
                while (idx - start < afterLineMax) {
                    if ((size_t)idx == src->length() - 1) {
                        break;
                    }
                    if (src->charAt((size_t)idx) == '\r' || src->charAt((size_t)idx) == '\n') {
                        break;
                    }
                    idx++;
                }
                afterLineSoFar = idx;

                if (preLineSoFar <= afterLineSoFar && preLineSoFar <= src->length() && afterLineSoFar <= src->length()) {
                    auto subSrc = src->substring(preLineSoFar, afterLineSoFar);
                    builder.appendChar('\n');
                    builder.appendString(subSrc);
                    builder.appendChar('\n');
                    std::string sourceCodePosition;
                    for (size_t i = preLineSoFar; i < start; i++) {
                        sourceCodePosition += " ";
                    }
                    sourceCodePosition += "^";
                    builder.appendString(String::fromUTF8(sourceCodePosition.data(), sourceCodePosition.length()));
                }
            }
        }

        if (i != gcValues.size() - 1) {
            builder.appendChar('\n');
        }
    }
    for (auto iter = locMap.begin(); iter != locMap.end(); iter++) {
        delete iter->second;
    }
}

void SandBox::fillStackDataIntoErrorObject(const Value& e)
{
    if (e.isObject() && e.asObject()->isErrorObject()) {
        ErrorObject* obj = e.asObject()->asErrorObject();
        ErrorObject::StackTraceData* data = ErrorObject::StackTraceData::create(this);
        obj->setStackTraceData(data);

        ExecutionState state(m_context);
        JSGetterSetter gs(
            new NativeFunctionObject(state, NativeFunctionInfo(m_context->staticStrings().stack, builtinErrorObjectStackInfo, 0, NativeFunctionInfo::Strict)),
            Value(Value::EmptyValue));
        ObjectPropertyDescriptor desc(gs, ObjectPropertyDescriptor::ConfigurablePresent);
        obj->defineOwnProperty(state, ObjectPropertyName(m_context->staticStrings().stack), desc);
    }
}
} // namespace Escargot
