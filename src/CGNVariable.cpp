/* Saphyr, a C++ style compiler using LLVM
 * Copyright (C) 2009-2017, Justin Madru (justin.jdm64@gmail.com)
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
#include "Value.h"
#include "AST.h"
#include "CGNVariable.h"
#include "Instructions.h"
#include "CGNExpression.h"

#define TABLE_ADD(ID) table[NODEID_DIFF(NodeId::ID, NodeId::StartExpression)] = reinterpret_cast<classPtr>(&CGNVariable::visit##ID)

CGNVariable::classPtr* CGNVariable::buildVTable()
{
	auto table = new CGNVariable::classPtr[NODEID_DIFF(NodeId::EndExpression, NodeId::StartExpression)];
	TABLE_ADD(NAddressOf);
	TABLE_ADD(NArrayVariable);
	TABLE_ADD(NBaseVariable);
	TABLE_ADD(NDereference);
	TABLE_ADD(NExprVariable);
	TABLE_ADD(NFunctionCall);
	TABLE_ADD(NMemberFunctionCall);
	TABLE_ADD(NMemberVariable);
	return table;
}

CGNVariable::classPtr* CGNVariable::vtable = CGNVariable::buildVTable();

RValue CGNVariable::visit(NVariable* type)
{
	return (this->*vtable[NODEID_DIFF(type->id(), NodeId::StartExpression)])(type);
}

RValue CGNVariable::visitNBaseVariable(NBaseVariable* baseVar)
{
	auto varName = baseVar->getName()->str;

	// check current function
	auto var = context.loadSymbolLocal(varName);
	if (var)
		return var;

	// check class variables
	auto currClass = context.getClass();
	if (currClass) {
		auto item = currClass->getItem(varName);
		if (item) {
			if (context.currFunction().isStatic()) {
				context.addError("use of class member invalid in static function", *baseVar);
				return RValue();
			}
			return Inst::LoadMemberVar(context, varName);
		}
	}

	// check global variables
	var = context.loadSymbolGlobal(varName);
	if (var)
		return var;

	// check enums
	auto userVar = SUserType::lookup(context, varName);
	if (!userVar) {
		if (context.currFunction().isStatic()) {
			context.addError("use of class member invalid in static function", *baseVar);
		} else {
			context.addError("variable " + varName + " not declared", *baseVar);
		}
		return var;
	}
	return RValue(ConstantInt::getFalse(context), userVar);
}

RValue CGNVariable::visitNArrayVariable(NArrayVariable* nArrVar)
{
	auto indexVal = CGNExpression::run(context, nArrVar->getIndex());

	if (!indexVal) {
		return indexVal;
	} else if (!indexVal.stype()->isNumeric()) {
		context.addError("array index is not able to be cast to an int", *nArrVar->getIndex());
		return RValue();
	}

	auto var = visit(nArrVar->getArrayVar());
	if (!var)
		return var;
	var = Inst::Deref(context, var, true);

	if (!var.stype()->isSequence()) {
		context.addError(var.stype()->str(&context) + " is not an array or vec", *nArrVar->getArrayVar());
		return RValue();
	}
	Inst::CastTo(context, *nArrVar->getIndex(), indexVal, SType::getInt(context, 64));

	vector<Value*> indexes;
	indexes.push_back(RValue::getZero(context, SType::getInt(context, 32)));
	indexes.push_back(indexVal);

	return Inst::GetElementPtr(context, var, indexes, var.stype()->subType());
}

RValue CGNVariable::visitNMemberVariable(NMemberVariable* memVar)
{
	auto var = visit(memVar->getBaseVar());
	if (!var)
		return RValue();

	var = Inst::Deref(context, var, true);
	return Inst::LoadMemberVar(context, var, *memVar->getBaseVar(), memVar->getMemberName());
}

RValue CGNVariable::visitNDereference(NDereference* nVar)
{
	auto var = visit(nVar->getVar());
	if (!var) {
		return var;
	} else if (!var.stype()->isPointer()) {
		context.addError("cannot dereference " + var.stype()->str(&context), *nVar);
		return RValue();
	}
	return Inst::Deref(context, var);
}

RValue CGNVariable::visitNAddressOf(NAddressOf* var)
{
	return visit(var->getVar());
}

RValue CGNVariable::visitNFunctionCall(NFunctionCall* var)
{
	return Inst::StoreTemporary(context, var);
}

RValue CGNVariable::visitNExprVariable(NExprVariable* var)
{
	return Inst::StoreTemporary(context, var->getExp());
}

RValue CGNVariable::visitNMemberFunctionCall(NMemberFunctionCall* var)
{
	return Inst::StoreTemporary(context, var);
}
