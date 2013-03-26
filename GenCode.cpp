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
#include <fstream>
#include <llvm/Constants.h>
#include <llvm/PassManager.h>
#include <llvm/Analysis/Verifier.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Assembly/PrintModulePass.h>
#include "parserbase.h"
#include "AST.h"
#include "Util.h"

NStatementList* programBlock;

void CodeContext::genCode(NStatementList stms)
{
	stms.genCode(*this);

	if (errors) {
		cout << "found " << errors << " errors" << endl;
		return;
	}
	verifyModule(*module);

	fstream file(filename.substr(0, filename.rfind('.')) + ".ll", fstream::out);
	raw_os_ostream stream(file);

	PassManager pm;
	pm.add(createPrintModulePass(&stream));
	pm.run(*module);
}

Type* NQualifier::getVarType(CodeContext& context)
{
	switch (type) {
	case QualifierType::VOID:
		return Type::getVoidTy(context.getContext());
	case QualifierType::BOOL:
		return Type::getInt1Ty(context.getContext());
	case QualifierType::INT8:
		return Type::getInt8Ty(context.getContext());
	case QualifierType::INT16:
		return Type::getInt16Ty(context.getContext());
	case QualifierType::INT:
	case QualifierType::INT32:
		return Type::getInt32Ty(context.getContext());
	case QualifierType::INT64:
		return Type::getInt64Ty(context.getContext());
	case QualifierType::FLOAT:
		return Type::getFloatTy(context.getContext());
	case QualifierType::DOUBLE:
		return Type::getDoubleTy(context.getContext());
	case QualifierType::AUTO:
	default:
		return nullptr;
	}
}

Value* NVariable::genCode(CodeContext& context)
{
	auto var = context.loadVar(name);

	if (!var) {
		cout << "error: variable " << *name << " not declared" << endl;
		context.incErrCount();
		return nullptr;
	}
	return new LoadInst(var, "", context.currBlock());
}

Value* NParameter::genCode(CodeContext& context)
{
	auto stackAlloc = new AllocaInst(type->getVarType(context), "", context.currBlock());
	auto storeParam = new StoreInst(arg, stackAlloc, context.currBlock());
	context.storeLocalVar(stackAlloc, name);

	return storeParam;
}

Value* NVariableDecl::genCode(CodeContext& context)
{
	auto initValue = initExp? initExp->genCode(context) : nullptr;
	auto varType = type->getVarType(context);

	if (!varType) { // auto type
		if (!initValue) { // auto type requires initialization
			cout << "error: auto variable type requires initialization" << endl;
			context.incErrCount();
			return nullptr;
		}
		varType = initValue->getType();
	}

	auto name = getName();
	if (context.loadVarCurr(name)) {
		cout << "error: variable " << *name << " already defined" << endl;
		context.incErrCount();
		return nullptr;
	}

	auto var = new AllocaInst(varType, *name, context.currBlock());
	context.storeLocalVar(var, name);

	if (initValue) {
		typeCastMatch(initValue, var->getType()->getPointerElementType(), context);
		new StoreInst(initValue, var, context.currBlock());
	}
	return nullptr;
}

Value* NGlobalVariableDecl::genCode(CodeContext& context)
{
	if (initExp && initExp->getNodeType() != NodeType::IntConst && initExp->getNodeType() != NodeType::FloatConst) {
		cout << "error: global variables only support constant value initializer" << endl;
		context.incErrCount();
		return nullptr;
	}
	auto initValue = initExp? initExp->genCode(context) : nullptr;
	auto varType = type->getVarType(context);

	if (!varType) { // auto type
		if (!initValue) { // auto type requires initialization
			cout << "error: auto variable type requires initialization" << endl;
			context.incErrCount();
			return nullptr;
		}
		varType = initValue->getType();
	}
	if (initValue && varType != initValue->getType()) {
		cout << "error: global variable initialization requires exact type matching" << endl;
		context.incErrCount();
		return nullptr;
	}

	auto name = getName();
	if (context.loadVarCurr(name)) {
		cout << "error: variable " << *name << " already defined" << endl;
		context.incErrCount();
		return nullptr;
	}

	auto var = new GlobalVariable(*context.getModule(), varType, false, GlobalValue::ExternalLinkage, (Constant*) initValue, *name);
	context.storeGlobalVar(var, name);
	return nullptr;
}

Value* NVariableDeclGroup::genCode(CodeContext& context)
{
	for (auto variable : *variables) {
		variable->setQualifier(type);
		variable->genCode(context);
	}
	return nullptr;
}

Value* NFunctionPrototype::genCode(CodeContext& context)
{
	auto function = context.getFunction(name);
	if (function)
		return function;

	auto funcType = getFunctionType(context);
	return Function::Create(funcType, GlobalValue::ExternalLinkage, *name, context.getModule());
}

void NFunctionPrototype::genCodeParams(Function* function, CodeContext& context)
{
	int i = 0;
	for (auto arg = function->arg_begin(); arg != function->arg_end(); arg++) {
		auto param = params->getItem(i++);
		param->setArgument(arg);
		arg->setName(*param->getName());
	}
	params->genCode(context);
}

Value* NFunctionDeclaration::genCode(CodeContext& context)
{
	auto function = static_cast<Function*>(prototype->genCode(context));
	if (!function)
		return nullptr;

	if (!body) {
		// no body means only function prototype
		return function;
	} else if (function->size()) {
		cout << "error: function " << *prototype->getName() << " already declared" << endl;
		context.incErrCount();
		return nullptr;
	}

	if (body->getLast()->getNodeType() != NodeType::ReturnStm) {
		auto returnType = function->getFunctionType()->getReturnType();
		if (returnType->isVoidTy()) {
			body->addItem(new NReturnStatement);
		} else {
			cout << "error: no return for a non-void function" << endl;
			context.incErrCount();
		}
	}

	if (prototype->getFunctionType(context) != function->getFunctionType()) {
		cout << "error: function type for " << *prototype->getName() << " doesn't match definition" << endl;
		context.incErrCount();
		return nullptr;
	}

	context.startFuncBlock(function);
	prototype->genCodeParams(function, context);
	body->genCode(context);
	context.endFuncBlock();

	return function;
}

Value* NReturnStatement::genCode(CodeContext& context)
{
	auto func = context.currBlock()->getParent();
	auto funcReturn = func->getReturnType();

	if (funcReturn->isVoidTy()) {
		if (value) {
			cout << "error: " << func->getName().str() << " function declared void,"
				<< " but non-void return found" << endl;
			context.incErrCount();
			return nullptr;
		}
	} else if (!value) {
		cout << "error: " << func->getName().str() << " function declared non-void,"
			<< " but void return found" << endl;
		context.incErrCount();
		return nullptr;
	}
	auto returnVal = value? value->genCode(context) : nullptr;
	if (returnVal)
		typeCastMatch(returnVal, funcReturn, context);
	return ReturnInst::Create(context.getContext(), returnVal, context.currBlock());
}

Value* NWhileStatement::genCode(CodeContext& context)
{
	BasicBlock* condBlock;
	BasicBlock* bodyBlock;

	if (isDoWhile) {
		bodyBlock = context.createBlock();
		condBlock = context.createBlock();
	} else {
		condBlock = context.createBlock();
		bodyBlock = context.createBlock();
	}
	auto endBlock = context.createBlock();
	auto startBlock = isDoWhile? bodyBlock : condBlock;
	auto trueBlock = isUntil? endBlock : bodyBlock;
	auto falseBlock = isUntil? bodyBlock : endBlock;

	context.pushLocalTable();
	context.pushContinueBlock(condBlock);
	context.pushRedoBlock(bodyBlock);
	context.pushBreakBlock(endBlock);

	BranchInst::Create(startBlock, context.currBlock());

	context.pushBlock(condBlock);
	auto condValue = condition? condition->genCode(context) : ConstantInt::getTrue(context.getContext());;
	typeCastMatch(condValue, Type::getInt1Ty(context.getContext()), context);
	BranchInst::Create(trueBlock, falseBlock, condValue, context.currBlock());

	context.pushBlock(bodyBlock);
	body->genCode(context);
	BranchInst::Create(condBlock, context.currBlock());

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popContinueBlock();
	context.popRedoBlock();
	context.popBreakBlock();

	return nullptr;
}

Value* NForStatement::genCode(CodeContext& context)
{
	auto condBlock = context.createBlock();
	auto bodyBlock = context.createBlock();
	auto postBlock = context.createBlock();
	auto endBlock = context.createBlock();

	context.pushLocalTable();
	context.pushContinueBlock(postBlock);
	context.pushRedoBlock(bodyBlock);
	context.pushBreakBlock(endBlock);

	preStm->genCode(context);
	BranchInst::Create(condBlock, context.currBlock());

	context.pushBlock(condBlock);
	auto condValue = condition? condition->genCode(context) : ConstantInt::getTrue(context.getContext());
	typeCastMatch(condValue, Type::getInt1Ty(context.getContext()), context);
	BranchInst::Create(bodyBlock, endBlock, condValue, context.currBlock());

	context.pushBlock(bodyBlock);
	body->genCode(context);
	BranchInst::Create(postBlock, context.currBlock());

	context.pushBlock(postBlock);
	postExp->genCode(context);
	BranchInst::Create(condBlock, context.currBlock());

	context.pushBlock(endBlock);
	context.popLocalTable();
	context.popContinueBlock();
	context.popRedoBlock();
	context.popBreakBlock();

	return nullptr;
}

Value* NIfStatement::genCode(CodeContext& context)
{
	auto ifBlock = context.createBlock();
	auto elseBlock = context.createBlock();
	auto endBlock = context.createBlock();

	context.pushLocalTable();

	auto condValue = condition->genCode(context);
	typeCastMatch(condValue, Type::getInt1Ty(context.getContext()), context);
	BranchInst::Create(ifBlock, elseBlock, condValue, context.currBlock());

	context.pushBlock(ifBlock);
	ifBody->genCode(context);
	BranchInst::Create(endBlock, context.currBlock());

	context.popLocalTable();
	context.pushLocalTable();

	context.pushBlock(elseBlock);
	if (elseBody)
		elseBody->genCode(context);
	BranchInst::Create(endBlock, context.currBlock());

	context.pushBlock(endBlock);
	context.popLocalTable();

	return nullptr;
}

Value* NLabelStatement::genCode(CodeContext& context)
{
	// check if label already declared
	auto label = context.getLabelBlock(name);
	if (label) {
		if (!label->isPlaceholder) {
			cout << "error: label \"" << *name << "\" already defined" << endl;
			context.incErrCount();
			return nullptr;
		}
		// a used label is no longer a placeholder
		label->isPlaceholder = false;
	} else {
		label = new LabelBlock(context.createBlock(), false);
		context.setLabelBlock(name, label);
	}
	BranchInst::Create(label->block, context.currBlock());
	context.pushBlock(label->block);
	return nullptr;
}

Value* NGotoStatement::genCode(CodeContext& context)
{
	auto skip = context.createBlock();
	auto label = context.getLabelBlock(name);
	if (!label) {
		// trying to jump to a non-existant label. create place holder and
		// later check if it's used at the end of the function.
		label = new LabelBlock(context.createBlock(), true);
		context.setLabelBlock(name, label);
	}
	BranchInst::Create(label->block, context.currBlock());
	context.pushBlock(skip);
	return nullptr;
}

Value* NLoopBranch::genCode(CodeContext& context)
{
	BasicBlock* block;
	string typeName;

	switch (type) {
	case ParserBase::TT_CONTINUE:
		block = context.getContinueBlock();
		if (!block) {
			typeName = "continue";
			goto ret;
		}
		break;
	case ParserBase::TT_REDO:
		block = context.getRedoBlock();
		if (!block) {
			typeName = "redo";
			goto ret;
		}
		break;
	case ParserBase::TT_BREAK:
		block = context.getBreakBlock();
		if (!block) {
			typeName = "break";
			goto ret;
		}
		break;
	default:
		cout << "error: undefined loop branch type " << type << endl;
		return nullptr;
	}
	BranchInst::Create(block, context.currBlock());
	context.pushBlock(context.createBlock());
	return nullptr;
ret:
	cout << "error: no valid context for " << typeName << " statement" << endl;
	context.incErrCount();
	return nullptr;
}

Value* NAssignment::genCode(CodeContext& context)
{
	auto lhsVar = context.loadVar(lhs->getName());
	auto rhsExp = rhs->genCode(context);

	if (!lhsVar) {
		cout << "error: variable " << *lhs->getName() << " not declared" << endl;
		context.incErrCount();
	}
	if (!(lhsVar && rhsExp))
		return nullptr;

	if (oper != '=') {
		Value* lhsLocal = new LoadInst(lhsVar, "", context.currBlock());
		typeCastUp(lhsLocal, rhsExp, context);
		rhsExp = BinaryOperator::Create(getOperator(oper, lhsLocal->getType(), context), lhsLocal, rhsExp, "", context.currBlock());
	}
	typeCastMatch(rhsExp, lhsVar->getType()->getPointerElementType(), context);
	new StoreInst(rhsExp, lhsVar, context.currBlock());

	return rhsExp;
}

Value* NTernaryOperator::genCode(CodeContext& context)
{
	auto condExp = condition->genCode(context);
	typeCastMatch(condExp, Type::getInt1Ty(context.getContext()), context);

	Value *trueExp, *falseExp, *retVal;
	if (isComplexExp(trueVal->getNodeType()) || isComplexExp(falseVal->getNodeType())) {
		auto trueBlock = context.createBlock();
		auto falseBlock = context.createBlock();
		auto endBlock = context.createBlock();

		BranchInst::Create(trueBlock, falseBlock, condExp, context.currBlock());

		context.pushBlock(trueBlock);
		trueExp = trueVal->genCode(context);
		BranchInst::Create(endBlock, context.currBlock());

		context.pushBlock(falseBlock);
		falseExp = falseVal->genCode(context);
		BranchInst::Create(endBlock, context.currBlock());

		context.pushBlock(endBlock);
		auto result = PHINode::Create(trueExp->getType(), 2, "", context.currBlock());
		result->addIncoming(trueExp, trueBlock);
		result->addIncoming(falseExp, falseBlock);
		retVal = result;
	} else {
		trueExp = trueVal->genCode(context);
		falseExp = falseVal->genCode(context);
		retVal = SelectInst::Create(condExp, trueExp, falseExp, "", context.currBlock());
	}

	if (trueExp->getType() != falseExp->getType()) {
		cout << "error: return types of ternary must match" << endl;
		context.incErrCount();
	}
	return retVal;
}

Value* NLogicalOperator::genCode(CodeContext& context)
{
	auto saveBlock = context.currBlock();
	auto firstBlock = context.createBlock();
	auto secondBlock = context.createBlock();
	auto trueBlock = (oper == ParserBase::TT_LOG_AND)? firstBlock : secondBlock;
	auto falseBlock = (oper == ParserBase::TT_LOG_AND)? secondBlock : firstBlock;

	auto lhsExp = lhs->genCode(context);
	typeCastMatch(lhsExp, Type::getInt1Ty(context.getContext()), context);
	BranchInst::Create(trueBlock, falseBlock, lhsExp, context.currBlock());

	context.pushBlock(firstBlock);
	auto rhsExp = rhs->genCode(context);
	typeCastMatch(rhsExp, Type::getInt1Ty(context.getContext()), context);
	BranchInst::Create(secondBlock, context.currBlock());

	context.pushBlock(secondBlock);
	auto result = PHINode::Create(Type::getInt1Ty(context.getContext()), 2, "", context.currBlock());
	result->addIncoming(lhsExp, saveBlock);
	result->addIncoming(rhsExp, firstBlock);

	return result;
}

Value* NCompareOperator::genCode(CodeContext& context)
{
	auto lhsExp = lhs->genCode(context);
	auto rhsExp = rhs->genCode(context);

	if (!(rhsExp && lhsExp))
		return nullptr;

	typeCastUp(lhsExp, rhsExp, context);
	auto pred = getPredicate(oper, lhsExp->getType(), context);
	auto op = rhsExp->getType()->isFloatingPointTy()? Instruction::FCmp : Instruction::ICmp;

	return CmpInst::Create(op, pred, lhsExp, rhsExp, "", context.currBlock());
}

Value* NBinaryMathOperator::genCode(CodeContext& context)
{
	auto lhsExp = lhs->genCode(context);
	auto rhsExp = rhs->genCode(context);

	if (!(lhsExp && rhsExp))
		return nullptr;

	typeCastUp(lhsExp, rhsExp, context);
	return BinaryOperator::Create(getOperator(oper, lhsExp->getType(), context), lhsExp, rhsExp, "", context.currBlock());
}

Value* NNullCoalescing::genCode(CodeContext& context)
{
	Value *rhsExp, *retVal;
	auto lhsExp = lhs->genCode(context);
	auto condition = lhsExp;

	typeCastMatch(condition, Type::getInt1Ty(context.getContext()), context);
	if (isComplexExp(rhs->getNodeType())) {
		auto trueBlock = context.currBlock();
		auto falseBlock = context.createBlock();
		auto endBlock = context.createBlock();

		BranchInst::Create(endBlock, falseBlock, condition, context.currBlock());

		context.pushBlock(falseBlock);
		rhsExp = rhs->genCode(context);
		BranchInst::Create(endBlock, context.currBlock());

		context.pushBlock(endBlock);
		auto result = PHINode::Create(lhsExp->getType(), 2, "", context.currBlock());
		result->addIncoming(lhsExp, trueBlock);
		result->addIncoming(rhsExp, falseBlock);

		retVal = result;
	} else {
		rhsExp = rhs->genCode(context);
		retVal = SelectInst::Create(condition, lhsExp, rhsExp, "", context.currBlock());
	}

	if (lhsExp->getType() != rhsExp->getType()) {
		cout << "error: return types of null coalescing operator must match" << endl;
		context.incErrCount();
	}
	return retVal;
}

Value* NUnaryMathOperator::genCode(CodeContext& context)
{
	Value* value;
	Instruction::BinaryOps llvmOp;
	Instruction::OtherOps cmpType;
	CmpInst::Predicate pred;

	auto unaryExp = unary->genCode(context);
	auto type = unaryExp->getType();

	switch (oper) {
	case '+':
	case '-':
		llvmOp = getOperator(oper, type, context);
		return BinaryOperator::Create(llvmOp, Constant::getNullValue(type), unaryExp, "", context.currBlock());
	case '!':
		pred = getPredicate(ParserBase::TT_EQ, type, context);
		cmpType = type->isFloatingPointTy()? Instruction::FCmp : Instruction::ICmp;
		return CmpInst::Create(cmpType, pred, Constant::getNullValue(type), unaryExp, "", context.currBlock());
	case '~':
		llvmOp = getOperator('^', type, context);
		value = Constant::getAllOnesValue(type);
		return BinaryOperator::Create(llvmOp, value, unaryExp, "", context.currBlock());
	default:
		cout << "error: invalid unary operator " << oper << endl;
		context.incErrCount();
		return nullptr;
	}
}

Value* NFunctionCall::genCode(CodeContext& context)
{
	auto func = context.getFunction(name);
	if (!func) {
		cout << "error: function " << *name << " not defined" << endl;
		context.incErrCount();
		return nullptr;
	}
	auto argCount = arguments->size();
	auto paramCount = func->getFunctionType()->getNumParams();
	if (argCount != paramCount) {
		cout << "error: argument count for " << func->getName().str() << " function invalid, "
			<< argCount << " arguments given, but " << paramCount << " required." << endl;
		context.incErrCount();
	}
	auto funcType = func->getFunctionType();
	vector<Value*> exp_list;
	int i = 0;
	for (auto arg : *arguments) {
		auto argExp = arg->genCode(context);
		typeCastMatch(argExp, funcType->getParamType(i++), context);
		exp_list.push_back(argExp);
	}
	return CallInst::Create(func, exp_list, "", context.currBlock());
}

Value* NIncrement::genCode(CodeContext& context)
{
	auto varVal = variable->genCode(context);
	if (!varVal)
		return nullptr;

	auto op = getOperator(isIncrement? '+' : '-', varVal->getType(), context);
	Constant* one = varVal->getType()->isFloatingPointTy()?
		ConstantFP::get(varVal->getType(), "1.0") :
		ConstantInt::getSigned(varVal->getType(), 1);
	auto result = BinaryOperator::Create(op, varVal, one, "", context.currBlock());
	new StoreInst(result, context.loadVar(variable->getName()), context.currBlock());

	return isPostfix? varVal : result;
}

Value* NIntConst::genCode(CodeContext& context)
{
	int bits;
	switch (type) {
	case QualifierType::BOOL:
		bits = 1;
		break;
	case QualifierType::INT8:
		bits = 8;
		break;
	case QualifierType::INT16:
		bits = 16;
		break;
	case QualifierType::INT64:
		bits = 64;
		break;
	case QualifierType::INT32:
	case QualifierType::INT:
	default:
		bits = 32;
		break;
	}
	return ConstantInt::get(Type::getIntNTy(context.getContext(), bits), *value, 10);
}

Value* NFloatConst::genCode(CodeContext& context)
{
	Type* llvmType;
	switch (type) {
	default:
	case QualifierType::FLOAT:
		llvmType = Type::getFloatTy(context.getContext());
		break;
	case QualifierType::DOUBLE:
		llvmType = Type::getDoubleTy(context.getContext());
		break;
	}
	return ConstantFP::get(llvmType, *value);
}

