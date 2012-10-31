/*      Saphyr, a C++ style compiler using LLVM
        Copyright (C) 2012, Justin Madru (justin.jdm64@gmail.com)

        This program is free software: you can redistribute it and/or modify
        it under the terms of the GNU General Public License as published by
        the Free Software Foundation, either version 3 of the License, or
        (at your option) any later version.

        This program is distributed in the hope that it will be useful,
        but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
        GNU General Public License for more details.

        You should have received a copy of the GNU General Public License
        along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <iostream>
#include "Util.h"
#include "parserbase.h"

typedef llvm::CmpInst::Predicate Predicate;

void typeCastUp(Value*& lhs, Value*& rhs, CodeContext& context)
{
	auto ltype = lhs->getType();
	auto rtype = rhs->getType();

	if (ltype == rtype)
		return;

	if (ltype->isFloatingPointTy()) {
		if (rtype->isFloatingPointTy()) {
			if (ltype->isDoubleTy())
				rhs = CastInst::CreateFPCast(rhs, ltype, "", context.currBlock());
			else
				lhs = CastInst::CreateFPCast(lhs, rtype, "", context.currBlock());
		} else {
			rhs = new SIToFPInst(rhs, ltype, "", context.currBlock());
		}
	} else if (rtype->isFloatingPointTy()) {
		if (ltype->isFloatingPointTy()) {
			if (rtype->isDoubleTy())
				lhs = CastInst::CreateFPCast(lhs, rtype, "", context.currBlock());
			else
				rhs = CastInst::CreateFPCast(rhs, ltype, "", context.currBlock());
		} else {
			lhs = new SIToFPInst(lhs, rtype, "", context.currBlock());
		}
	} else {
		if (rtype->getIntegerBitWidth() > ltype->getIntegerBitWidth())
			lhs = CastInst::CreateIntegerCast(lhs, rtype, true, "", context.currBlock());
		else if (rtype->getIntegerBitWidth() < ltype->getIntegerBitWidth())
			rhs = CastInst::CreateIntegerCast(rhs, ltype, true, "", context.currBlock());
	}
	return;
}

void typeCastMatch(Value*& value, Type* type, CodeContext& context)
{
	if (type == value->getType())
		return;

	if (type->isFloatingPointTy()) {
		if (value->getType()->isFloatingPointTy())
			value = CastInst::CreateFPCast(value, type, "", context.currBlock());
		else
			value = new SIToFPInst(value, type, "", context.currBlock());
	} else if (type->isIntegerTy(1)) {
		// cast to bool is value != 0
		auto pred = getPredicate(ParserBase::TT_NEQ, value->getType(), context);
		auto op = value->getType()->isFloatingPointTy()? Instruction::FCmp : Instruction::ICmp;
		value = CmpInst::Create(op, pred, value, Constant::getNullValue(value->getType()), "", context.currBlock());
	} else if (value->getType()->isFloatingPointTy()) {
		value = new FPToSIInst(value, type, "", context.currBlock());
	} else {
		value = CastInst::CreateIntegerCast(value, type, true, "", context.currBlock());
	}
}

Instruction::BinaryOps getOperator(int oper, Type* type, CodeContext& context)
{
	switch (oper) {
	case '*':
		return type->isFloatingPointTy()? Instruction::FMul : Instruction::Mul;
	case '/':
		return type->isFloatingPointTy()? Instruction::FDiv : Instruction::SDiv;
	case '%':
		return type->isFloatingPointTy()? Instruction::FRem : Instruction::SRem;
	case '+':
		return type->isFloatingPointTy()? Instruction::FAdd : Instruction::Add;
	case '-':
		return type->isFloatingPointTy()? Instruction::FSub : Instruction::Sub;
	case ParserBase::TT_LSHIFT:
		if (type->isFloatingPointTy()) {
			cout << "error: shift operator invalid for float types" << endl;
			context.incErrCount();
		}
		return Instruction::Shl;
	case ParserBase::TT_RSHIFT:
		if (type->isFloatingPointTy()) {
			cout << "error: shift operator invalid for float types" << endl;
			context.incErrCount();
		}
		return Instruction::LShr;
	case '&':
		if (type->isFloatingPointTy()) {
			cout << "error: AND operator invalid for float types" << endl;
			context.incErrCount();
		}
		return Instruction::And;
	case '|':
		if (type->isFloatingPointTy()) {
			cout << "error: OR operator invalid for float types" << endl;
			context.incErrCount();
		}
		return Instruction::Or;
	case '^':
		if (type->isFloatingPointTy()) {
			cout << "error: XOR operator invalid for float types" << endl;
			context.incErrCount();
		}
		return Instruction::Xor;
	default:
		cout << "error: unrecognized operator " << oper << endl;
		context.incErrCount();
		return Instruction::Add;
	}
}

Predicate getPredicate(int oper, Type* type, CodeContext& context)
{
	switch (oper) {
	case '<':
		return type->isFloatingPointTy()? Predicate::FCMP_OLT : Predicate::ICMP_SLT;
	case '>':
		return type->isFloatingPointTy()? Predicate::FCMP_OGT : Predicate::ICMP_SGT;
	case ParserBase::TT_LEQ:
		return type->isFloatingPointTy()? Predicate::FCMP_OLE : Predicate::ICMP_SLE;
	case ParserBase::TT_GEQ:
		return type->isFloatingPointTy()? Predicate::FCMP_OGE : Predicate::ICMP_SGE;
	case ParserBase::TT_NEQ:
		return type->isFloatingPointTy()? Predicate::FCMP_ONE : Predicate::ICMP_NE;
	case ParserBase::TT_EQ:
		return type->isFloatingPointTy()? Predicate::FCMP_OEQ : Predicate::ICMP_EQ;
	default:
		cout << "error: unrecognized predicate " << oper << endl;
		context.incErrCount();
		return Predicate::ICMP_EQ;
	}
}

bool isComplexExp(NodeType type)
{
	switch (type) {
	case NodeType::IntConst:
	case NodeType::FloatConst:
	case NodeType::Variable:
		return false;
	default:
		return true;
	}
}
