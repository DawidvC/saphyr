/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2014, Justin Madru (justin.jdm64@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __INSTRUCTIONS_H__
#define __INSTRUCTIONS_H__

#include "AST.h"
#include "CodeContext.h"

typedef CmpInst::Predicate Predicate;
typedef Instruction::BinaryOps BinaryOps;
typedef Instruction::CastOps CastOps;

class Inst
{
	static inline void castError(CodeContext& context, SType* from, SType* to, Token* token);

	static BinaryOps getOperator(int oper, Token* optToken, SType* type, CodeContext& context);

	static Predicate getPredicate(int oper, Token* token, SType* type, CodeContext& context);

	static void NumericCast(RValue& value, SType* from, SType* to, SType* final, CodeContext& context);

	static bool CastMatch(CodeContext& context, Token* optToken, RValue& lhs, RValue& rhs, bool upcast = false);

	static RValue PointerMath(int type, Token* optToken, RValue ptr, RValue val, CodeContext& context);

public:
	static bool CastTo(CodeContext& context, Token* token, RValue& value, SType* type, bool upcast = false);

	static RValue BinaryOp(int type, Token* optToken, RValue lhs, RValue rhs, CodeContext& context);

	static RValue Branch(BasicBlock* trueBlock, BasicBlock* falseBlock, NExpression* condExp, Token* token, CodeContext& context);

	static RValue Cmp(int type, Token* optToken, RValue lhs, RValue rhs, CodeContext& context);

	static RValue Load(CodeContext& context, RValue value);

	static RValue Deref(CodeContext& context, RValue value, bool recursive = false);

	static RValue SizeOf(CodeContext& context, Token* token, SType* type);

	static RValue SizeOf(CodeContext& context, Token* token, NDataType* type);

	static RValue SizeOf(CodeContext& context, Token* token, NExpression* type);

	static RValue SizeOf(CodeContext& context, Token* token, const string& type);

	inline static RValue GetElementPtr(CodeContext& context, const RValue& ptr, ArrayRef<Value*> idxs, SType* type)
	{
		#if LLVM_VERSION_MAJOR == 3 && LLVM_VERSION_MINOR >= 7
			auto ptrVal = GetElementPtrInst::Create(nullptr, ptr, idxs, "", context);
		#else
			auto ptrVal = GetElementPtrInst::Create(ptr, idxs, "", context);
		#endif
		return RValue(ptrVal, type);
	}

	static RValue CallFunction(CodeContext& context, SFunction& func, Token* name, NExpressionList* args, vector<Value*>& expList);

	static void CallDestructor(CodeContext& context, RValue value, Token* valueToken);

	static RValue LoadMemberVar(CodeContext& context, const string& baseName, RValue baseVar, Token* dotToken, Token* memberName);
};

#endif
