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

#ifndef BinaryExpressionBitwiseAndNode_h
#define BinaryExpressionBitwiseAndNode_h

#include "ExpressionNode.h"

namespace Escargot {

class BinaryExpressionBitwiseAndNode : public ExpressionNode {
public:
    friend class ScriptParser;

    BinaryExpressionBitwiseAndNode(Node* left, Node* right)
        : ExpressionNode()
        , m_left((ExpressionNode*)left)
        , m_right((ExpressionNode*)right)
    {
    }

    virtual ~BinaryExpressionBitwiseAndNode()
    {
    }

    virtual ASTNodeType type() override { return ASTNodeType::BinaryExpressionBitwiseAnd; }
    virtual void generateExpressionByteCode(ByteCodeBlock* codeBlock, ByteCodeGenerateContext* context, ByteCodeRegisterIndex dstRegister) override
    {
        bool isSlow = !canUseDirectRegister(context, m_left.get(), m_right.get());
        bool directBefore = context->m_canSkipCopyToRegister;
        if (isSlow) {
            context->m_canSkipCopyToRegister = false;
        }
        size_t src0 = m_left->getRegister(codeBlock, context);
        size_t src1 = m_right->getRegister(codeBlock, context);
        m_left->generateExpressionByteCode(codeBlock, context, src0);
        m_right->generateExpressionByteCode(codeBlock, context, src1);

        context->giveUpRegister();
        context->giveUpRegister();

        codeBlock->pushCode(BinaryBitwiseAnd(ByteCodeLOC(m_loc.index), src0, src1, dstRegister), context, this);

        context->m_canSkipCopyToRegister = directBefore;
    }

    virtual void iterateChildrenIdentifier(const std::function<void(AtomicString name, bool isAssignment)>& fn) override
    {
        m_left->iterateChildrenIdentifier(fn);
        m_right->iterateChildrenIdentifier(fn);
    }

    virtual void iterateChildren(const std::function<void(Node* node)>& fn) override
    {
        fn(this);

        m_left->iterateChildren(fn);
        m_right->iterateChildren(fn);
    }

private:
    RefPtr<ExpressionNode> m_left;
    RefPtr<ExpressionNode> m_right;
};
}

#endif
