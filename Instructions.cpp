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
#include "Instructions.h"
#include "parserbase.h"

void Inst::castError(CodeContext& context, SType* from, SType* to, Token* token)
{
	context.addError("can not cast " + from->str(&context) + " to " + to->str(&context), token);
}

bool Inst::CastMatch(CodeContext& context, Token* optToken, RValue& lhs, RValue& rhs, bool upcast)
{
	auto ltype = lhs.stype();
	auto rtype = rhs.stype();

	if (ltype == rtype) {
		return false;
	} else if (ltype->isComplex() || rtype->isComplex()) {
		context.addError("can not cast between " + ltype->str(&context) + " and " + rtype->str(&context) + " types", optToken);
		return true;
	} else if (ltype->isPointer() || rtype->isPointer()) {
		// different pointer types can't be cast automatically
		// the operations need to handle pointers specially
		return false;
	}

	auto toType = SType::numericConv(context, optToken, ltype, rtype, upcast);
	return CastTo(context, optToken, lhs, toType, upcast) || CastTo(context, optToken, rhs, toType, upcast);
}

bool Inst::CastTo(CodeContext& context, Token* token, RValue& value, SType* type, bool upcast)
{
	auto valueType = value.stype();

	if (type == valueType) {
		// non-assignment and non-comparison operations with enum types
		// must result in an int type to keep enum variables within range.
		if (upcast && valueType->isEnum())
			value.castToSubtype();
		return false;
	} else if (type->isComplex() || valueType->isComplex()) {
		castError(context, valueType, type, token);
		return true;
	} else if (type->isPointer()) {
		if (value.isNullPtr()) {
			// NOTE: discard current null value and create
			// a new one using the right type
			value = RValue::getNullPtr(context, type);
			return false;
		} else if (type->subType()->isVoid()) {
			value = RValue(new BitCastInst(value, *type, "", context), type);
			return false;
		}
		castError(context, valueType, type, token);
		return true;
	} else if (type->isVec()) {
		// unwrap enum type
		if (valueType->isEnum())
			valueType = value.castToSubtype();

		if (valueType->isNumeric()) {
			CastTo(context, token, value, type->subType(), upcast);

			auto i32 = SType::getInt(context, 32);
			auto mask = RValue::getZero(context, SType::getVec(context, i32, type->size()));
			auto udef = UndefValue::get(*SType::getVec(context, type->subType(), 1));
			auto instEle = InsertElementInst::Create(udef, value, RValue::getZero(context, i32), "", context);
			auto retVal = new ShuffleVectorInst(instEle, udef, mask, "", context);

			value = RValue(retVal, type);
			return false;
		} else if (valueType->isPointer()) {
			castError(context, valueType, type, token);
			return true;
		} else if (type->size() != valueType->size()) {
			context.addError("can not cast vec types of different sizes", token);
			return true;
		} else if (type->subType()->isBool()) {
			// cast to bool is value != 0
			auto pred = getPredicate(ParserBase::TT_NEQ, token, valueType, context);
			auto op = valueType->subType()->isFloating()? Instruction::FCmp : Instruction::ICmp;
			auto val = CmpInst::Create(op, pred, value, RValue::getZero(context, valueType), "", context);
			value = RValue(val, type);
			return false;
		} else {
			auto op = getCastOp(valueType->subType(), type->subType());
			auto val = CastInst::Create(op, value, *type, "", context);
			value = RValue(val, type);
			return false;
		}
	} else if (type->isEnum()) {
		// casting to enum would violate value constraints
		castError(context, valueType, type, token);
		return true;
	} else if (type->isBool()) {
		if (valueType->isVec()) {
			castError(context, valueType, type, token);
			return true;
		} else if (valueType->isEnum()) {
			valueType = value.castToSubtype();
		}

		// cast to bool is value != 0
		auto pred = getPredicate(ParserBase::TT_NEQ, token, valueType, context);
		auto op = valueType->isFloating()? Instruction::FCmp : Instruction::ICmp;
		auto val = CmpInst::Create(op, pred, value, RValue::getZero(context, valueType), "", context);
		value = RValue(val, type);
		return false;
	}

	if (valueType->isPointer()) {
		castError(context, valueType, type, token);
		return true;
	}

	// unwrap enum type
	if (valueType->isEnum())
		valueType = value.castToSubtype();

	auto op = getCastOp(valueType, type);
	auto val = op != Instruction::AddrSpaceCast? CastInst::Create(op, value, *type, "", context) : value;
	value = RValue(val, type);
	return false;
}

CastOps Inst::getCastOp(SType* from, SType* to)
{
	switch (from->isFloating() | to->isFloating() << 1) {
	case 0:
		// both int
		if (to->size() > from->size())
			return from->isUnsigned()? Instruction::ZExt : Instruction::SExt;
		else if (from->size() > to->size())
			return Instruction::Trunc;
		break;
	case 1:
		// from = float, to = int
		return to->isUnsigned()? Instruction::FPToUI : Instruction::FPToSI;
	case 2:
		// from = int, to = float
		return from->isUnsigned()? Instruction::UIToFP : Instruction::SIToFP;
	case 3:
		// both float
		return to->isDouble()? Instruction::FPExt : Instruction::FPTrunc;
	}
	return Instruction::AddrSpaceCast;
}

BinaryOps Inst::getOperator(int oper, Token* optToken, SType* type, CodeContext& context)
{
	if (type->isVec()) {
		type = type->subType();
	} else if (type->isComplex()) {
		context.addError("can not perform operation on composite types", optToken);
		return Instruction::Add;
	}
	switch (oper) {
	case '*':
		return type->isFloating()? Instruction::FMul : Instruction::Mul;
	case '/':
		if (type->isFloating())
			return Instruction::FDiv;
		return type->isUnsigned()? Instruction::UDiv : Instruction::SDiv;
	case '%':
		if (type->isFloating())
			return Instruction::FRem;
		return type->isUnsigned()? Instruction::URem : Instruction::SRem;
	case '+':
	case ParserBase::TT_INC:
	case ParserBase::TT_DEC:
		return type->isFloating()? Instruction::FAdd : Instruction::Add;
	case '-':
		return type->isFloating()? Instruction::FSub : Instruction::Sub;
	case ParserBase::TT_LSHIFT:
		if (type->isFloating())
			context.addError("shift operator invalid for float types", optToken);
		return Instruction::Shl;
	case ParserBase::TT_RSHIFT:
		if (type->isFloating())
			context.addError("shift operator invalid for float types", optToken);
		return type->isUnsigned()? Instruction::LShr : Instruction::AShr;
	case '&':
		if (type->isFloating())
			context.addError("AND operator invalid for float types", optToken);
		return Instruction::And;
	case '|':
		if (type->isFloating())
			context.addError("OR operator invalid for float types", optToken);
		return Instruction::Or;
	case '^':
		if (type->isFloating())
			context.addError("XOR operator invalid for float types", optToken);
		return Instruction::Xor;
	default:
		context.addError("unrecognized operator " + to_string(oper), optToken);
		return Instruction::Add;
	}
}

Predicate Inst::getPredicate(int oper, Token* token, SType* type, CodeContext& context)
{
	const static Predicate predArr[] = {
		Predicate::FCMP_OLT, Predicate::FCMP_OGT, Predicate::FCMP_OLE,
		Predicate::FCMP_OGE, Predicate::FCMP_ONE, Predicate::FCMP_OEQ,
		Predicate::ICMP_ULT, Predicate::ICMP_UGT, Predicate::ICMP_ULE,
		Predicate::ICMP_UGE, Predicate::ICMP_NE,  Predicate::ICMP_EQ,
		Predicate::ICMP_SLT, Predicate::ICMP_SGT, Predicate::ICMP_SLE,
		Predicate::ICMP_SGE, Predicate::ICMP_NE,  Predicate::ICMP_EQ
	};

	if (type->isVec()) {
		type = type->subType();
	} else if (type->isComplex()) {
		context.addError("can not perform operation on composite types", token);
		return Predicate::ICMP_EQ;
	}

	int offset;
	switch (oper) {
	case '<':
		offset = 0; break;
	case '>':
		offset = 1; break;
	case ParserBase::TT_LEQ:
		offset = 2; break;
	case ParserBase::TT_GEQ:
		offset = 3; break;
	case ParserBase::TT_NEQ:
		offset = 4; break;
	case ParserBase::TT_EQ:
		offset = 5; break;
	default:
		context.addError("unrecognized predicate " + to_string(oper), token);
		return Predicate::ICMP_EQ;
	}

	if (type->isFloating())
		return predArr[offset];
	offset += type->isUnsigned()? 6 : 12;
	return predArr[offset];
}

RValue Inst::PointerMath(int type, Token* optToken, RValue ptr, RValue val, CodeContext& context)
{
	if (type != ParserBase::TT_INC && type != ParserBase::TT_DEC) {
		context.addError("pointer arithmetic only valid using ++/-- operators", optToken);
		return ptr;
	}
	auto ptrVal = GetElementPtrInst::Create(ptr, val.value(), "", context);
	return RValue(ptrVal, ptr.stype());
}

RValue Inst::BinaryOp(int type, Token* optToken, RValue lhs, RValue rhs, CodeContext& context)
{
	if (!lhs || !rhs)
		return RValue();

	if (CastMatch(context, optToken, lhs, rhs, true))
		return RValue();

	switch ((lhs.stype()->isPointer() << 1) | rhs.stype()->isPointer()) {
	default:
	case 0: // no pointer
	{
		auto llvmOp = getOperator(type, optToken, lhs.stype(), context);
		return RValue(BinaryOperator::Create(llvmOp, lhs, rhs, "", context), lhs.stype());
	}
	case 1: // lhs != ptr, rhs == ptr
		swap(lhs, rhs);
	case 2: // lhs == ptr, rhs != ptr
		return PointerMath(type, optToken, lhs, rhs, context);
	case 3: // both ptr
		context.addError("can't perform operation with two pointers", optToken);
		return lhs;
	}
}

RValue Inst::Branch(BasicBlock* trueBlock, BasicBlock* falseBlock, NExpression* condExp, Token* token, CodeContext& context)
{
	auto condValue = condExp? condExp->genValue(context) : RValue::getNumVal(context, SType::getBool(context));
	CastTo(context, token, condValue, SType::getBool(context));
	BranchInst::Create(trueBlock, falseBlock, condValue, context);
	return condValue;
}

RValue Inst::Cmp(int type, Token* optToken, RValue lhs, RValue rhs, CodeContext& context)
{
	if (CastMatch(context, optToken, lhs, rhs))
		return RValue();
	auto pred = getPredicate(type, optToken, lhs.stype(), context);
	auto cmpType = lhs.stype()->isVec()? lhs.stype()->subType() : lhs.stype();
	auto op = cmpType->isFloating()? Instruction::FCmp : Instruction::ICmp;
	auto cmp = CmpInst::Create(op, pred, lhs, rhs, "", context);
	auto retType = lhs.stype()->isVec()?
		SType::getVec(context, SType::getBool(context), lhs.stype()->size()) :
		SType::getBool(context);
	return RValue(cmp, retType);
}

RValue Inst::Load(CodeContext& context, RValue value)
{
	return RValue(new LoadInst(value, "", context), value.stype());
}

RValue Inst::Deref(CodeContext& context, RValue value, bool recursive)
{
	auto retVal = RValue(value.value(), value.stype());
	while (retVal.stype()->isPointer()) {
		retVal = RValue(new LoadInst(retVal, "", context), retVal.stype()->subType());
		if (!recursive)
			break;
	}
	return retVal;
}

RValue Inst::SizeOf(CodeContext& context, Token* token, SType* type)
{
	if (!type) {
		return RValue();
	} else if (type->isAuto() || type->isVoid()) {
		context.addError("size of " + type->str(&context) + " is invalid", token);
		return RValue();
	}
	auto itype = SType::getInt(context, 64, true);
	auto size = ConstantInt::get(*itype, SType::allocSize(context, type));
	return RValue(size, itype);
}

RValue Inst::SizeOf(CodeContext& context, Token* token, NDataType* type)
{
	return SizeOf(context, token, type->getType(context));
}

RValue Inst::SizeOf(CodeContext& context, Token* token, NExpression* exp)
{
	return SizeOf(context, token, exp->genValue(context).stype());
}

RValue Inst::SizeOf(CodeContext& context, Token* token, const string& name)
{
	auto isType = SUserType::lookup(context, name);
	auto isVar = context.loadSymbol(name);
	SType* stype = nullptr;

	if (isType && isVar) {
		context.addError(name + " is ambigious, both a type and a variable", token);
	} else if (isType) {
		stype = isType;
	} else if (isVar) {
		stype = isVar.stype();
	} else {
		context.addError("type " + name + " is not declared", token);
	}
	return SizeOf(context, token, stype);
}
