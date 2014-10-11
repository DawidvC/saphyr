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
#ifndef __AST_H__
#define __AST_H__

#include <vector>
#include <set>
#include <llvm/IR/Instructions.h>
#include "Value.h"
#include "Function.h"

// forward declaration
class CodeContext;

class Node
{
public:
	virtual ~Node() {};
};

template<typename NType>
class NodeList : public Node
{
	typedef typename vector<NType*>::iterator NTypeIter;

protected:
	vector<NType*> list;

public:
	template<typename OtherList>
	OtherList* copy()
	{
		auto other = new OtherList;
		for (const auto item : *this)
			other->addItem(item);
		return other;
	}

	bool empty() const
	{
		return list.empty();
	}

	int size() const
	{
		return list.size();
	}

	NTypeIter begin()
	{
		return list.begin();
	}

	NTypeIter end()
	{
		return list.end();
	}

	void addItem(NType* item)
	{
		list.push_back(item);
	}

	NType* at(int i)
	{
		return list.at(i);
	}

	NType* front()
	{
		return list.empty()? nullptr : list.front();
	}

	NType* back()
	{
		return list.empty()? nullptr : list.back();
	}
};

class NStatement : public Node
{
public:
	virtual void genCode(CodeContext& context) = 0;

	virtual bool isTerminator() const
	{
		return false;
	}
};

class NStatementList : public NodeList<NStatement>
{
public:
	void genCode(CodeContext& context) const
	{
		for (auto item : list)
			item->genCode(context);
	}
};
extern NStatementList* programBlock;

class NExpression : public NStatement
{
public:
	void genCode(CodeContext& context) final
	{
		genValue(context);
	};

	virtual bool isConstant() const
	{
		return false;
	}

	virtual bool isComplex() const
	{
		return true;
	}

	virtual RValue genValue(CodeContext& context) = 0;
};

class NExpressionList : public NodeList<NExpression>
{
public:
	void genCode(CodeContext& context) const
	{
		for (auto item : list)
			item->genValue(context);
	}
};

class NConstant : public NExpression
{
public:
	bool isConstant() const
	{
		return true;
	}

	bool isComplex() const
	{
		return false;
	}
};

class NBoolConst : public NConstant
{
	bool value;

public:
	NBoolConst(bool value)
	: value(value) {}

	RValue genValue(CodeContext& context);
};

class NNumberConst : public NConstant
{
	string* value;

public:
	NNumberConst(string* value)
	: value(value) {}

	vector<string> getValueAndSuffix()
	{
		auto pos = value->find('_');
		if (pos == string::npos)
			return {*value};
		else
			return {value->substr(0, pos), value->substr(pos + 1)};
	}

	~NNumberConst()
	{
		delete value;
	}
};

class NIntConst : public NNumberConst
{
	int base;

public:
	NIntConst(string* value, int base = 10)
	: NNumberConst(value), base(base) {}

	RValue genValue(CodeContext& context);

	ConstantInt* getConstInt(CodeContext& context)
	{
		return static_cast<ConstantInt*>(genValue(context).value());
	}

	int64_t getInt(CodeContext& context)
	{
		return getConstInt(context)->getSExtValue();
	}
};

class NFloatConst : public NNumberConst
{
public:
	NFloatConst(string* value)
	: NNumberConst(value) {}

	RValue genValue(CodeContext& context);
};

class NCharConst : public NConstant
{
	string* value;

public:
	NCharConst(string* charStr)
	{
		value = new string(charStr->substr(1, charStr->length() - 2));
	}

	RValue genValue(CodeContext& context);

	~NCharConst()
	{
		delete value;
	}
};

class NDeclaration : public NStatement
{
protected:
	string* name;

public:
	NDeclaration(string* name)
	: name(name) {}

	string* getName()
	{
		return name;
	}

	~NDeclaration()
	{
		delete name;
	}
};

class NDataType : public Node
{
public:
	virtual SType* getType(CodeContext& context) = 0;
};
typedef NodeList<NDataType> NDataTypeList;

class NBaseType : public NDataType
{
	int type;

public:
	NBaseType(int type)
	: type(type) {}

	SType* getType(CodeContext& context);
};

class NArrayType : public NDataType
{
	NDataType* baseType;
	NIntConst* size;

public:
	NArrayType(NIntConst* size, NDataType* baseType)
	: baseType(baseType), size(size) {}

	SType* getType(CodeContext& context);

	~NArrayType()
	{
		delete baseType;
		delete size;
	}
};

class NVecType : public NDataType
{
	NBaseType* baseType;
	NIntConst* size;

public:
	NVecType(NIntConst* size, NBaseType* baseType)
	: baseType(baseType), size(size) {}

	SType* getType(CodeContext& context);

	~NVecType()
	{
		delete baseType;
		delete size;
	}
};

class NUserType : public NDataType
{
	string* name;

public:
	NUserType(string* name)
	: name(name) {}

	SType* getType(CodeContext& context);

	~NUserType()
	{
		delete name;
	}
};

class NPointerType : public NDataType
{
	NDataType* baseType;

public:
	NPointerType(NDataType* baseType)
	: baseType(baseType) {}

	SType* getType(CodeContext& context);

	~NPointerType()
	{
		delete baseType;
	}
};

class NFuncPointerType : public NDataType
{
	NDataType* returnType;
	NDataTypeList* params;

public:
	NFuncPointerType(NDataType* returnType, NDataTypeList* params)
	: returnType(returnType), params(params) {}

	SType* getType(CodeContext& context)
	{
		auto ptr = NFuncPointerType::getType(context, returnType, params);
		return ptr? SType::getPointer(context, ptr) : nullptr;
	}

	static SFunctionType* getType(CodeContext& context, NDataType* retType, NDataTypeList* params)
	{
		bool valid = true;
		vector<SType*> args;
		for (auto item : *params) {
			auto param = item->getType(context);
			if (param)
				args.push_back(param);
			else
				valid = false;
		}

		auto returnType = retType->getType(context);
		return (returnType && valid)? SType::getFunction(context, returnType, args) : nullptr;
	}

	~NFuncPointerType()
	{
		delete returnType;
		delete params;
	}
};

class NVariableDecl : public NDeclaration
{
protected:
	NExpression* initExp;
	NDataType* type;

public:
	NVariableDecl(string* name, NExpression* initExp = nullptr)
	: NDeclaration(name), initExp(initExp), type(nullptr) {}

	// NOTE: must be called before genCode()
	void setDataType(NDataType* qtype)
	{
		type = qtype;
	}

	bool hasInit() const
	{
		return initExp;
	}

	void genCode(CodeContext& context);

	~NVariableDecl()
	{
		delete initExp;
		delete type;
	}
};

class NVariableDeclList : public NodeList<NVariableDecl>
{
public:
	void genCode(NDataType* type, CodeContext& context) const
	{
		for (auto variable : list) {
			variable->setDataType(type);
			variable->genCode(context);
		}
	}
};

class NGlobalVariableDecl : public NVariableDecl
{
public:
	NGlobalVariableDecl(string* name, NExpression* initExp = nullptr)
	: NVariableDecl(name, initExp) {}

	void genCode(CodeContext& context);
};

class NVariable : public NExpression
{
public:
	RValue genValue(CodeContext& context)
	{
		return genValue(context, loadVar(context));
	}

	virtual RValue genValue(CodeContext& context, RValue var);

	virtual RValue loadVar(CodeContext& context) = 0;

	virtual string* getName() const = 0;
};

class NBaseVariable : public NVariable
{
	string* name;

public:
	NBaseVariable(string* name)
	: name(name) {}

	RValue loadVar(CodeContext& context);

	string* getName() const
	{
		return name;
	}

	bool isComplex() const
	{
		return false;
	}

	~NBaseVariable()
	{
		delete name;
	}
};

class NArrayVariable : public NVariable
{
	NVariable* arrVar;
	NExpression* index;

public:
	NArrayVariable(NVariable* arrVar, NExpression* index)
	: arrVar(arrVar), index(index) {}

	RValue loadVar(CodeContext& context);

	string* getName() const
	{
		return arrVar->getName();
	}

	~NArrayVariable()
	{
		delete arrVar;
		delete index;
	}
};

class NMemberVariable : public NVariable
{
	NVariable* baseVar;
	string* memberName;

public:
	NMemberVariable(NVariable* baseVar, string* memberName)
	: baseVar(baseVar), memberName(memberName) {}

	RValue loadVar(CodeContext& context);

	RValue loadStruct(CodeContext& context, RValue& baseValue, SStructType* structType);

	RValue loadUnion(CodeContext& context, RValue& baseValue, SUnionType* unionType);

	string* getName() const
	{
		return baseVar->getName();
	}

	~NMemberVariable()
	{
		delete baseVar;
		delete memberName;
	}
};

class NExprVariable : public NVariable
{
	NExpression* expr;

public:
	NExprVariable(NExpression* expr)
	: expr(expr) {}

	RValue loadVar(CodeContext& context)
	{
		return expr->genValue(context);
	}

	string* getName() const
	{
		static string name = "temp expression";
		return &name;
	}

	~NExprVariable()
	{
		delete expr;
	}
};

class NDereference : public NVariable
{
	NVariable* derefVar;

public:
	NDereference(NVariable* derefVar)
	: derefVar(derefVar) {}

	RValue loadVar(CodeContext& context);

	string* getName() const
	{
		return derefVar->getName();
	}
};

class NAddressOf : public NVariable
{
	NVariable* addVar;

public:
	NAddressOf(NVariable* addVar)
	: addVar(addVar) {}

	RValue genValue(CodeContext& context, RValue var)
	{
		return var? RValue(var, SType::getPointer(context, var.stype())) : var;
	}

	RValue loadVar(CodeContext& context);

	string* getName() const
	{
		return addVar->getName();
	}
};

class NParameter : public NDeclaration
{
	NDataType* type;
	RValue arg; // NOTE: not owned by NParameter

public:
	NParameter(NDataType* type, string* name)
	: NDeclaration(name), type(type) {}

	// NOTE: this must be called before genCode()
	void setArgument(RValue argument)
	{
		arg = argument;
	}

	SType* getType(CodeContext& context)
	{
		return type->getType(context);
	}

	NDataType* getTypeNode() const
	{
		return type;
	}

	void genCode(CodeContext& context);

	~NParameter()
	{
		delete type;
	}
};
typedef NodeList<NParameter> NParameterList;

class NVariableDeclGroup : public NStatement
{
	NDataType* type;
	NVariableDeclList* variables;

public:
	NVariableDeclGroup(NDataType* type, NVariableDeclList* variables)
	: type(type), variables(variables) {}

	void genCode(CodeContext& context)
	{
		variables->genCode(type, context);
	}

	bool addMembers(vector<pair<string, SType*> >& structVector, set<string>& memberNames, CodeContext& context);

	~NVariableDeclGroup()
	{
		delete variables;
		delete type;
	}
};
typedef NodeList<NVariableDeclGroup> NVariableDeclGroupList;

class NStructDeclaration : public NDeclaration
{
	NVariableDeclGroupList* list;

protected:
	virtual void createUserType(vector<pair<string, SType*> > structVars, CodeContext& context)
	{
		SUserType::createStruct(context, name, structVars);
	}

public:
	NStructDeclaration(string* name, NVariableDeclGroupList* list)
	: NDeclaration(name), list(list) {}

	void genCode(CodeContext& context);

	~NStructDeclaration()
	{
		delete list;
	}
};

class NUnionDeclaration : public NStructDeclaration
{
protected:
	void createUserType(vector<pair<string, SType*> > structVars, CodeContext& context)
	{
		SUserType::createUnion(context, name, structVars);
	}

public:
	NUnionDeclaration(string* name, NVariableDeclGroupList* list)
	: NStructDeclaration(name, list) {}
};

class NFunctionPrototype : public NDeclaration
{
	NDataType* rtype;
	NParameterList* params;

public:
	NFunctionPrototype(string* name, NDataType* rtype, NParameterList* params)
	: NDeclaration(name), rtype(rtype), params(params) {}

	void genCode(CodeContext& context) final;

	SFunction genFunction(CodeContext& context);

	void genCodeParams(SFunction function, CodeContext& context) const;

	SFunctionType* getFunctionType(CodeContext& context)
	{
		NDataTypeList typeList;
		for (auto item : *params) {
			typeList.addItem(item->getTypeNode());
		}
		return NFuncPointerType::getType(context, rtype, &typeList);
	}

	~NFunctionPrototype()
	{
		delete rtype;
		delete params;
	}
};

class NFunctionDeclaration : public NDeclaration
{
	NFunctionPrototype* prototype;
	NStatementList* body;

public:
	NFunctionDeclaration(NFunctionPrototype* prototype, NStatementList* body)
	: NDeclaration(prototype->getName()), prototype(prototype), body(body) {}

	void genCode(CodeContext& context);

	~NFunctionDeclaration()
	{
		delete prototype;
		delete body;
	}
};

class NConditionStmt : public NStatement
{
protected:
	NExpression* condition;
	NStatementList* body;

public:
	NConditionStmt(NExpression* condition, NStatementList* body)
	: condition(condition), body(body) {}

	~NConditionStmt()
	{
		delete condition;
		delete body;
	}
};

class NWhileStatement : public NConditionStmt
{
	bool isDoWhile;
	bool isUntil;

public:
	NWhileStatement(NExpression* condition, NStatementList* body, bool isDoWhile = false, bool isUntil = false)
	: NConditionStmt(condition, body), isDoWhile(isDoWhile), isUntil(isUntil) {}

	void genCode(CodeContext& context);
};

class NSwitchCase : public NStatement
{
	NIntConst* value;
	NStatementList* body;

public:
	// used for default case
	NSwitchCase(NStatementList* body)
	: value(nullptr), body(body) {}

	NSwitchCase(NIntConst* value, NStatementList* body)
	: value(value), body(body) {}

	void genCode(CodeContext& context)
	{
		body->genCode(context);
	}

	ConstantInt* getValue(CodeContext& context)
	{
		return value->getConstInt(context);
	}

	bool isValueCase() const
	{
		return value != nullptr;
	}

	bool isLastStmBranch() const
	{
		auto last = body->back();
		if (!last)
			return false;
		return last->isTerminator();
	}

	~NSwitchCase()
	{
		delete value;
		delete body;
	}
};
typedef NodeList<NSwitchCase> NSwitchCaseList;

class NSwitchStatement : public NStatement
{
	NExpression* value;
	NSwitchCaseList* cases;

public:
	NSwitchStatement(NExpression* value, NSwitchCaseList* cases)
	: value(value), cases(cases) {}

	void genCode(CodeContext& context);

	~NSwitchStatement()
	{
		delete value;
		delete cases;
	}
};

class NForStatement : public NConditionStmt
{
	NStatementList* preStm;
	NExpressionList* postExp;

public:
	NForStatement(NStatementList* preStm, NExpression* condition, NExpressionList* postExp, NStatementList* body)
	: NConditionStmt(condition, body), preStm(preStm), postExp(postExp) {}

	void genCode(CodeContext& context);

	~NForStatement()
	{
		delete preStm;
		delete postExp;
	}
};

class NIfStatement : public NConditionStmt
{
	NStatementList* elseBody;

public:
	NIfStatement(NExpression* condition, NStatementList* ifBody, NStatementList* elseBody)
	: NConditionStmt(condition, ifBody), elseBody(elseBody) {}

	void genCode(CodeContext& context);

	~NIfStatement()
	{
		delete elseBody;
	}
};

class NLabelStatement : public NStatement
{
	string* name;

public:
	NLabelStatement(string* name)
	: name(name) {}

	void genCode(CodeContext& context);

	~NLabelStatement()
	{
		delete name;
	}
};

class NJumpStatement : public NStatement
{
public:
	bool isTerminator() const
	{
		return true;
	}
};

class NReturnStatement : public NJumpStatement
{
	NExpression* value;

public:
	NReturnStatement(NExpression* value = nullptr)
	: value(value) {}

	void genCode(CodeContext& context);

	~NReturnStatement()
	{
		delete value;
	}
};

class NGotoStatement : public NJumpStatement
{
	string* name;

public:
	NGotoStatement(string* name)
	: name(name) {}

	void genCode(CodeContext& context);

	~NGotoStatement()
	{
		delete name;
	}
};

class NLoopBranch : public NJumpStatement
{
	int type;
	NIntConst* level;

public:
	NLoopBranch(int type, NIntConst* level = nullptr)
	: type(type), level(level) {}

	void genCode(CodeContext& context);
};

class NAssignment : public NExpression
{
	int oper;
	NVariable* lhs;
	NExpression* rhs;

public:
	NAssignment(int oper, NVariable* lhs, NExpression* rhs)
	: oper(oper), lhs(lhs), rhs(rhs) {}

	RValue genValue(CodeContext& context);

	~NAssignment()
	{
		delete lhs;
		delete rhs;
	}
};

class NTernaryOperator : public NExpression
{
	NExpression* condition;
	NExpression* trueVal;
	NExpression* falseVal;

public:
	NTernaryOperator(NExpression* condition, NExpression* trueVal, NExpression* falseVal)
	: condition(condition), trueVal(trueVal), falseVal(falseVal) {}

	RValue genValue(CodeContext& context);

	~NTernaryOperator()
	{
		delete condition;
		delete trueVal;
		delete falseVal;
	}
};

class NBinaryOperator : public NExpression
{
protected:
	int oper;
	NExpression* lhs;
	NExpression* rhs;

public:
	NBinaryOperator(int oper, NExpression* lhs, NExpression* rhs)
	: oper(oper), lhs(lhs), rhs(rhs) {}

	~NBinaryOperator()
	{
		delete lhs;
		delete rhs;
	}
};

class NLogicalOperator : public NBinaryOperator
{
public:
	NLogicalOperator(int oper, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, lhs, rhs) {}

	RValue genValue(CodeContext& context);
};

class NCompareOperator : public NBinaryOperator
{
public:
	NCompareOperator(int oper, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, lhs, rhs) {}

	RValue genValue(CodeContext& context);
};

class NBinaryMathOperator : public NBinaryOperator
{
public:
	NBinaryMathOperator(int oper, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, lhs, rhs) {}

	RValue genValue(CodeContext& context);
};

class NNullCoalescing : public NBinaryOperator
{
public:
	NNullCoalescing(NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(0, lhs, rhs) {}

	RValue genValue(CodeContext& context);
};

class NSizeOfOperator : public NExpression
{
	enum OfType { DATA, EXP, NAME };

	OfType type;
	NDataType* dtype;
	NExpression* exp;
	string* name;

public:
	NSizeOfOperator(NDataType* dtype)
	: type(DATA), dtype(dtype), exp(nullptr), name(nullptr) {}

	NSizeOfOperator(NExpression* exp)
	: type(EXP), dtype(nullptr), exp(exp), name(nullptr) {}

	NSizeOfOperator(string* name)
	: type(NAME), dtype(nullptr), exp(nullptr), name(name) {}

	RValue genValue(CodeContext& context);

	bool isConstant() const
	{
		return type == EXP? exp->isConstant() : true;
	}

	~NSizeOfOperator()
	{
		delete dtype;
		delete exp;
		delete name;
	}
};

class NUnaryOperator : public NExpression
{
protected:
	int oper;
	NExpression* unary;

public:
	NUnaryOperator(int oper, NExpression* unary)
	: oper(oper), unary(unary) {}

	~NUnaryOperator()
	{
		delete unary;
	}
};

class NUnaryMathOperator : public NUnaryOperator
{
public:
	NUnaryMathOperator(int oper, NExpression* unaryExp)
	: NUnaryOperator(oper, unaryExp) {}

	RValue genValue(CodeContext& context);
};

class NFunctionCall : public NVariable
{
	string* name;
	NExpressionList* arguments;

public:
	NFunctionCall(string* name, NExpressionList* arguments)
	: name(name), arguments(arguments) {}

	RValue genValue(CodeContext& context);

	RValue loadVar(CodeContext& context);

	string* getName() const
	{
		return name;
	}

	~NFunctionCall()
	{
		delete arguments;
	}
};

class NIncrement : public NExpression
{
	NVariable* variable;
	int type;
	bool isPostfix;

public:
	NIncrement(NVariable* variable, int type, bool isPostfix)
	: variable(variable), type(type), isPostfix(isPostfix) {}

	RValue genValue(CodeContext& context);

	~NIncrement()
	{
		delete variable;
	}
};

#endif
