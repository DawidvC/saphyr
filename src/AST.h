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
#include <llvm/ADT/APSInt.h>
#include <llvm/IR/Instructions.h>
#include "Token.h"
#include "Value.h"
#include "Function.h"
#include "Node.h"

// forward declaration
class CodeContext;

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

class NExpression : public Node
{
public:

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

class NExpressionStm : public NStatement
{
	NExpression* exp;

public:
	NExpressionStm(NExpression* exp)
	: exp(exp) {}

	static NStatementList* convert(NExpressionList* other)
	{
		auto ret = new NStatementList;
		for (auto item : *other)
			ret->add(new NExpressionStm(item));
		other->setDelete(false);
		delete other;
		return ret;
	}

	void genCode(CodeContext& context)
	{
		exp->genValue(context);
	}

	NExpression* getExp() const
	{
		return exp;
	}

	~NExpressionStm()
	{
		delete exp;
	}

	ADD_ID(NExpressionStm)
};

class NConstant : public NExpression
{
protected:
	Token* value;

public:
	explicit NConstant(Token* token)
	: value(token) {}

	Token* getToken() const
	{
		return value;
	}

	bool isConstant() const
	{
		return true;
	}

	bool isComplex() const
	{
		return false;
	}

	virtual bool isIntConst() const
	{
		return false;
	}

	const string& getStrVal() const
	{
		return value->str;
	}

	static vector<string> getValueAndSuffix(const string& value)
	{
		auto pos = value.find('_');
		if (pos == string::npos)
			return {value};
		else
			return {value.substr(0, pos), value.substr(pos + 1)};
	}

	static string unescape(const string &val)
	{
		string str;
		str.reserve(val.size());

		for (int i = 0;;) {
			auto idx = val.find('\\', i);
			if (idx == string::npos) {
				str.append(val, i, string::npos);
				break;
			} else {
				char c = val.at(++idx);
				switch (c) {
				case '0': c = '\0'; break;
				case 'a': c = '\a'; break;
				case 'b': c = '\b'; break;
				case 'e': c =   27; break;
				case 'f': c = '\f'; break;
				case 'n': c = '\n'; break;
				case 'r': c = '\r'; break;
				case 't': c = '\t'; break;
				case 'v': c = '\v'; break;
				default:
					break;
				}
				str.append(val, i, idx - i - 1);
				str += c;
				i = idx + 1;
			}
		}
		return str;
	}

	static void remove(string& val, char c = '\'')
	{
		val.erase(std::remove(val.begin(), val.end(), c), val.end());
	}

	~NConstant()
	{
		delete value;
	}
};

class NNullPointer : public NConstant
{
public:
	explicit NNullPointer(Token* token)
	: NConstant(token) {}

	RValue genValue(CodeContext& context);

	ADD_ID(NNullPointer)
};

class NStringLiteral : public NConstant
{
public:
	explicit NStringLiteral(Token* str)
	: NConstant(str)
	{
		value->str = unescape(value->str.substr(1, value->str.size() - 2));
	}

	RValue genValue(CodeContext& context);

	ADD_ID(NStringLiteral)
};

class NIntLikeConst : public NConstant
{
public:
	explicit NIntLikeConst(Token* token)
	: NConstant(token) {}

	RValue genValue(CodeContext& context) final;

	bool isIntConst() const
	{
		return true;
	}
};

class NBoolConst : public NIntLikeConst
{
	bool bvalue;

public:
	NBoolConst(Token* token, bool value)
	: NIntLikeConst(token), bvalue(value) {}

	bool getValue() const
	{
		return bvalue;
	}

	ADD_ID(NBoolConst)
};

class NCharConst : public NIntLikeConst
{
public:
	explicit NCharConst(Token* charStr)
	: NIntLikeConst(charStr)
	{
		value->str = value->str.substr(1, value->str.length() - 2);
	}

	ADD_ID(NCharConst)
};

class NIntConst : public NIntLikeConst
{
	int base;

public:
	NIntConst(Token* value, int base = 10)
	: NIntLikeConst(value), base(base)
	{
		remove(value->str);
	}

	int getBase() const
	{
		return base;
	}

	ADD_ID(NIntConst)
};

class NFloatConst : public NConstant
{
public:
	explicit NFloatConst(Token* value)
	: NConstant(value)
	{
		remove(value->str);
	}

	RValue genValue(CodeContext& context);

	ADD_ID(NFloatConst)
};

class NDeclaration : public NStatement
{
protected:
	Token* name;

public:
	explicit NDeclaration(Token* name)
	: name(name) {}

	Token* getNameToken() const
	{
		return name;
	}

	virtual const string& getName() const
	{
		return name->str;
	}

	~NDeclaration()
	{
		delete name;
	}
};

class NDataType : public Node {};
typedef NodeList<NDataType> NDataTypeList;

class NNamedType : public NDataType
{
protected:
	Token* token;

public:
	explicit NNamedType(Token* token)
	: token(token) {}

	Token* getToken() const
	{
		return token;
	}

	const string& getName() const
	{
		return token->str;
	}

	~NNamedType()
	{
		delete token;
	}
};

class NBaseType : public NNamedType
{
	int type;

public:
	NBaseType(Token* token, int type)
	: NNamedType(token), type(type) {}

	int getType() const
	{
		return type;
	}

	ADD_ID(NBaseType)
};

class NArrayType : public NDataType
{
	NDataType* baseType;
	NIntConst* size;

public:
	NArrayType(NDataType* baseType, NIntConst* size = nullptr)
	: baseType(baseType), size(size) {}

	NDataType* getBaseType() const
	{
		return baseType;
	}

	NIntConst* getSize() const
	{
		return size;
	}

	~NArrayType()
	{
		delete baseType;
		delete size;
	}

	ADD_ID(NArrayType)
};

class NVecType : public NDataType
{
	NDataType* baseType;
	NIntConst* size;
	Token* vecToken;

public:
	NVecType(Token* vecToken, NIntConst* size, NDataType* baseType)
	: baseType(baseType), size(size), vecToken(vecToken) {}

	NIntConst* getSize() const
	{
		return size;
	}

	NDataType* getBaseType() const
	{
		return baseType;
	}

	Token* getToken() const
	{
		return vecToken;
	}

	~NVecType()
	{
		delete baseType;
		delete size;
	}

	ADD_ID(NVecType)
};

class NUserType : public NNamedType
{
public:
	explicit NUserType(Token* name)
	: NNamedType(name) {}

	ADD_ID(NUserType)
};

class NPointerType : public NDataType
{
	NDataType* baseType;

public:
	explicit NPointerType(NDataType* baseType)
	: baseType(baseType) {}

	NDataType* getBaseType() const
	{
		return baseType;
	}

	~NPointerType()
	{
		delete baseType;
	}

	ADD_ID(NPointerType)
};

class NFuncPointerType : public NDataType
{
	NDataType* returnType;
	NDataTypeList* params;
	Token* atTok;

public:
	NFuncPointerType(Token* atTok, NDataType* returnType, NDataTypeList* params)
	: returnType(returnType), params(params), atTok(atTok) {}

	Token* getToken() const
	{
		return atTok;
	}

	NDataType* getReturnType() const
	{
		return returnType;
	}

	NDataTypeList* getParams() const
	{
		return params;
	}

	~NFuncPointerType()
	{
		delete returnType;
		delete params;
		delete atTok;
	}

	ADD_ID(NFuncPointerType)
};

class NVariableDecl : public NDeclaration
{
protected:
	NExpression* initExp;
	NExpressionList* initList;
	NDataType* type;
	Token* eqToken;

public:
	NVariableDecl(Token* name, Token* eqToken = nullptr, NExpression* initExp = nullptr)
	: NDeclaration(name), initExp(initExp), initList(nullptr), type(nullptr), eqToken(eqToken) {}

	NVariableDecl(Token* name, NExpressionList* initList)
	: NDeclaration(name), initExp(nullptr), initList(initList), type(nullptr), eqToken(nullptr) {}

	NDataType* getType() const
	{
		return type;
	}

	// NOTE: must be called before genCode()
	void setDataType(NDataType* qtype)
	{
		type = qtype;
	}

	bool hasInit() const
	{
		return initExp;
	}

	NExpression* getInitExp() const
	{
		return initExp;
	}

	NExpressionList* getInitList() const
	{
		return initList;
	}

	Token* getEqToken() const
	{
		return eqToken;
	}

	void genCode(CodeContext& context);

	~NVariableDecl()
	{
		delete initExp;
		delete eqToken;
		delete initList;
	}

	ADD_ID(NVariableDecl)
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
	NGlobalVariableDecl(Token* name, Token* eqToken = nullptr, NExpression* initExp = nullptr)
	: NVariableDecl(name, eqToken, initExp) {}

	void genCode(CodeContext& context);

	ADD_ID(NGlobalVariableDecl)
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

	virtual const string& getName() const = 0;
};

class NBaseVariable : public NVariable
{
	Token* name;

public:
	explicit NBaseVariable(Token* name)
	: name(name) {}

	RValue loadVar(CodeContext& context);

	Token* getToken() const
	{
		return name;
	}

	const string& getName() const
	{
		return name->str;
	}

	bool isComplex() const
	{
		return false;
	}

	~NBaseVariable()
	{
		delete name;
	}

	ADD_ID(NBaseVariable)
};

class NArrayVariable : public NVariable
{
	NVariable* arrVar;
	NExpression* index;
	Token* brackTok;

public:
	NArrayVariable(NVariable* arrVar, Token* brackTok, NExpression* index)
	: arrVar(arrVar), index(index), brackTok(brackTok) {}

	RValue loadVar(CodeContext& context);

	NExpression* getIndex() const
	{
		return index;
	}

	Token* getLBrack() const
	{
		return brackTok;
	}

	NVariable* getArrayVar() const
	{
		return arrVar;
	}

	const string& getName() const
	{
		return arrVar->getName();
	}

	~NArrayVariable()
	{
		delete arrVar;
		delete index;
		delete brackTok;
	}

	ADD_ID(NArrayVariable)
};

class NMemberVariable : public NVariable
{
	NVariable* baseVar;
	Token* memberName;
	Token* dotToken;

public:
	NMemberVariable(NVariable* baseVar, Token* memberName, Token* dotToken)
	: baseVar(baseVar), memberName(memberName), dotToken(dotToken) {}

	RValue loadVar(CodeContext& context);

	NVariable* getBaseVar() const
	{
		return baseVar;
	}

	Token* getDotToken() const
	{
		return dotToken;
	}

	Token* getMemberToken() const
	{
		return memberName;
	}

	const string& getName() const
	{
		return baseVar->getName();
	}

	const string& getMemberName() const
	{
		return memberName->str;
	}

	~NMemberVariable()
	{
		delete baseVar;
		delete memberName;
		delete dotToken;
	}

	ADD_ID(NMemberVariable)
};

class NExprVariable : public NVariable
{
	const static string STR_TMP_EXP;

	NExpression* expr;

public:
	explicit NExprVariable(NExpression* expr)
	: expr(expr) {}

	RValue loadVar(CodeContext& context)
	{
		return expr->genValue(context);
	}

	const string& getName() const
	{
		return STR_TMP_EXP;
	}

	~NExprVariable()
	{
		delete expr;
	}

	ADD_ID(NExprVariable)
};

class NDereference : public NVariable
{
	NVariable* derefVar;
	Token* atTok;

public:
	NDereference(NVariable* derefVar, Token* atTok)
	: derefVar(derefVar), atTok(atTok) {}

	RValue loadVar(CodeContext& context);

	NVariable* getVar() const
	{
		return derefVar;
	}

	Token* getAtToken() const
	{
		return atTok;
	}

	const string& getName() const
	{
		return derefVar->getName();
	}

	~NDereference()
	{
		delete derefVar;
		delete atTok;
	}

	ADD_ID(NDereference)
};

class NAddressOf : public NVariable
{
	NVariable* addVar;

public:
	explicit NAddressOf(NVariable* addVar)
	: addVar(addVar) {}

	RValue genValue(CodeContext& context, RValue var)
	{
		return var? RValue(var, SType::getPointer(context, var.stype())) : var;
	}

	RValue loadVar(CodeContext& context);

	NVariable* getVar() const
	{
		return addVar;
	}

	const string& getName() const
	{
		return addVar->getName();
	}

	~NAddressOf()
	{
		delete addVar;
	}

	ADD_ID(NAddressOf)
};

class NParameter : public NDeclaration
{
	NDataType* type;
	RValue arg; // NOTE: not owned by NParameter

public:
	NParameter(NDataType* type, Token* name)
	: NDeclaration(name), type(type) {}

	// NOTE: this must be called before genCode()
	void setArgument(RValue argument)
	{
		arg = argument;
	}

	RValue getArg() const
	{
		return arg;
	}

	NDataType* getType() const
	{
		return type;
	}

	void genCode(CodeContext& context);

	~NParameter()
	{
		delete type;
	}

	ADD_ID(NParameter)
};
typedef NodeList<NParameter> NParameterList;

class NVariableDeclGroup : public NStatement
{
	NDataType* type;
	NVariableDeclList* variables;

public:
	NVariableDeclGroup(NDataType* type, NVariableDeclList* variables)
	: type(type), variables(variables) {}

	NDataType* getType() const
	{
		return type;
	}

	NVariableDeclList* getVars() const
	{
		return variables;
	}

	void genCode(CodeContext& context)
	{
		variables->genCode(type, context);
	}

	~NVariableDeclGroup()
	{
		delete variables;
		delete type;
	}

	ADD_ID(NVariableDeclGroup)
};
typedef NodeList<NVariableDeclGroup> NVariableDeclGroupList;

class NAliasDeclaration : public NDeclaration
{
	NDataType* type;

public:
	NAliasDeclaration(Token* name, NDataType* type)
	: NDeclaration(name), type(type) {}

	void genCode(CodeContext& context);

	NDataType* getType() const
	{
		return type;
	}

	~NAliasDeclaration()
	{
		delete type;
	}

	ADD_ID(NAliasDeclaration)
};

class NStructDeclaration : public NDeclaration
{
public:
	enum class CreateType {STRUCT, UNION, CLASS};

private:
	NVariableDeclGroupList* list;
	CreateType ctype;

public:
	NStructDeclaration(Token* name, NVariableDeclGroupList* list, CreateType ctype = CreateType::STRUCT)
	: NDeclaration(name), list(list), ctype(ctype) {}

	void genCode(CodeContext& context);

	CreateType getType() const
	{
		return ctype;
	}

	NVariableDeclGroupList* getVars() const
	{
		return list;
	}

	~NStructDeclaration()
	{
		delete list;
	}

	ADD_ID(NStructDeclaration)
};

class NEnumDeclaration : public NDeclaration
{
	NVariableDeclList* variables;
	Token* lBrac;
	NDataType* baseType;

public:
	NEnumDeclaration(Token* name, NVariableDeclList* variables, Token* lBrac = nullptr, NDataType* baseType = nullptr)
	: NDeclaration(name), variables(variables), lBrac(lBrac), baseType(baseType) {}

	void genCode(CodeContext& context);

	NVariableDeclList* getVarList() const
	{
		return variables;
	}

	NDataType* getBaseType() const
	{
		return baseType;
	}

	Token* getLBrac() const
	{
		return lBrac;
	}

	~NEnumDeclaration()
	{
		delete variables;
		delete lBrac;
		delete baseType;
	}

	ADD_ID(NEnumDeclaration)
};

class NFunctionDeclaration : public NDeclaration
{
	NDataType* rtype;
	NParameterList* params;
	NStatementList* body;
	SFunction function;

public:
	NFunctionDeclaration(Token* name, NDataType* rtype, NParameterList* params, NStatementList* body)
	: NDeclaration(name), rtype(rtype), params(params), body(body) {}

	void genCode(CodeContext& context) final;

	NDataType* getRType() const
	{
		return rtype;
	}

	NParameterList* getParams() const
	{
		return params;
	}

	NStatementList* getBody() const
	{
		return body;
	}

	~NFunctionDeclaration()
	{
		delete rtype;
		delete params;
		delete body;
	}

	ADD_ID(NFunctionDeclaration)
};

// forward declaration
class NClassDeclaration;

class NClassMember : public NDeclaration
{
protected:
	NClassDeclaration* theClass;

public:
	enum class MemberType { CONSTRUCTOR, DESTRUCTOR, STRUCT, FUNCTION };

	explicit NClassMember(Token* name)
	: NDeclaration(name), theClass(nullptr) {}

	void setClass(NClassDeclaration* cl)
	{
		theClass = cl;
	}

	NClassDeclaration* getClass() const
	{
		return theClass;
	}

	virtual MemberType memberType() const = 0;
};
typedef NodeList<NClassMember> NClassMemberList;

class NMemberInitializer : public NStatement
{
	Token* name;
	NExpressionList *expression;

public:
	NMemberInitializer(Token* name, NExpressionList* expression)
	: name(name), expression(expression) {}

	Token* getNameToken() const
	{
		return name;
	}

	NExpressionList* getExp() const
	{
		return expression;
	}

	void genCode(CodeContext& context);

	ADD_ID(NMemberInitializer)
};
typedef NodeList<NMemberInitializer> NInitializerList;

class NClassDeclaration : public NDeclaration
{
	NClassMemberList* list;

public:
	NClassDeclaration(Token* name, NClassMemberList* list)
	: NDeclaration(name), list(list)
	{
		for (auto i : *list)
			i->setClass(this);
	}

	void genCode(CodeContext& context);

	NClassMemberList* getList() const
	{
		return list;
	}

	~NClassDeclaration()
	{
		delete list;
	}

	ADD_ID(NClassDeclaration)
};

class NClassStructDecl : public NClassMember
{
	NVariableDeclGroupList* list;

public:
	NClassStructDecl(Token* name, NVariableDeclGroupList* list)
	: NClassMember(name), list(list) {}

	void genCode(CodeContext& context);

	MemberType memberType() const
	{
		return MemberType::STRUCT;
	}

	NVariableDeclGroupList* getVarList() const
	{
		return list;
	}

	~NClassStructDecl()
	{
		delete list;
	}

	ADD_ID(NClassStructDecl)
};

class NClassFunctionDecl : public NClassMember
{
	NDataType* rtype;
	NParameterList* params;
	NStatementList* body;

public:
	NClassFunctionDecl(Token* name, NDataType* rtype, NParameterList* params, NStatementList* body)
	: NClassMember(name), rtype(rtype), params(params), body(body) {}

	void genCode(CodeContext& context);

	MemberType memberType() const
	{
		return MemberType::FUNCTION;
	}

	NParameterList* getParams() const
	{
		return params;
	}

	NDataType* getRType() const
	{
		return rtype;
	}

	NStatementList* getBody() const
	{
		return body;
	}

	~NClassFunctionDecl()
	{
		delete rtype;
		delete params;
		delete body;
	}

	ADD_ID(NClassFunctionDecl)
};

class NClassConstructor : public NClassMember
{
	NParameterList* params;
	NInitializerList* initList;
	NStatementList* body;

public:
	NClassConstructor(Token* name, NParameterList* params, NInitializerList* initList, NStatementList* body)
	: NClassMember(name), params(params), initList(initList), body(body) {}

	void genCode(CodeContext& context);

	MemberType memberType() const
	{
		return MemberType::CONSTRUCTOR;
	}

	NParameterList* getParams() const
	{
		return params;
	}

	NInitializerList* getInitList() const
	{
		return initList;
	}

	NStatementList* getBody() const
	{
		return body;
	}

	void setBody(NStatementList* other)
	{
		body = other;
	}

	~NClassConstructor()
	{
		delete params;
		delete initList;
		delete body;
	}

	ADD_ID(NClassConstructor)
};

class NClassDestructor : public NClassMember
{
	NStatementList* body;

public:
	NClassDestructor(Token* name, NStatementList* body)
	: NClassMember(name), body(body) {}

	void genCode(CodeContext& context);

	MemberType memberType() const
	{
		return MemberType::DESTRUCTOR;
	}

	NStatementList* getBody() const
	{
		return body;
	}

	~NClassDestructor()
	{
		delete body;
	}

	ADD_ID(NClassDestructor)
};

class NConditionStmt : public NStatement
{
protected:
	NExpression* condition;
	NStatementList* body;

public:
	NConditionStmt(NExpression* condition, NStatementList* body)
	: condition(condition), body(body) {}

	NExpression* getCond() const
	{
		return condition;
	}

	NStatementList* getBody() const
	{
		return body;
	}

	~NConditionStmt()
	{
		delete condition;
		delete body;
	}

	ADD_ID(NConditionStmt)
};

class NLoopStatement : public NConditionStmt
{
public:
	explicit NLoopStatement(NStatementList* body)
	: NConditionStmt(nullptr, body) {}

	void genCode(CodeContext& context);

	ADD_ID(NLoopStatement)
};

class NWhileStatement : public NConditionStmt
{
	Token* lparen;
	bool isDoWhile;
	bool isUntil;

public:
	NWhileStatement(Token* lparen, NExpression* condition, NStatementList* body, bool isDoWhile = false, bool isUntil = false)
	: NConditionStmt(condition, body), lparen(lparen), isDoWhile(isDoWhile), isUntil(isUntil) {}

	void genCode(CodeContext& context);

	Token* getLParen() const
	{
		return lparen;
	}

	bool doWhile() const
	{
		return isDoWhile;
	}

	bool until() const
	{
		return isUntil;
	}

	~NWhileStatement()
	{
		delete lparen;
	}

	ADD_ID(NWhileStatement)
};

class NSwitchCase : public NStatement
{
	NIntConst* value;
	NStatementList* body;
	Token* token;

public:
	NSwitchCase(Token* token, NStatementList* body, NIntConst* value = nullptr)
	: value(value), body(body), token(token) {}

	void genCode(CodeContext& context)
	{
		body->genCode(context);
	}

	NIntConst* getValue() const
	{
		return value;
	}

	NStatementList* getBody() const
	{
		return body;
	}

	Token* getToken() const
	{
		return token;
	}

	ConstantInt* getValue(CodeContext& context);

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
		delete token;
	}

	ADD_ID(NSwitchCase)
};
typedef NodeList<NSwitchCase> NSwitchCaseList;

class NSwitchStatement : public NStatement
{
	NExpression* value;
	NSwitchCaseList* cases;
	Token* lparen;

public:
	NSwitchStatement(Token* lparen, NExpression* value, NSwitchCaseList* cases)
	: value(value), cases(cases), lparen(lparen) {}

	void genCode(CodeContext& context);

	Token* getLParen() const
	{
		return lparen;
	}

	NExpression* getValue() const
	{
		return value;
	}

	NSwitchCaseList* getCases() const
	{
		return cases;
	}

	~NSwitchStatement()
	{
		delete value;
		delete cases;
		delete lparen;
	}

	ADD_ID(NSwitchStatement)
};

class NForStatement : public NConditionStmt
{
	NStatementList* preStm;
	NExpressionList* postExp;
	Token* semiCol2;

public:
	NForStatement(NStatementList* preStm, NExpression* condition, Token* semiCol2, NExpressionList* postExp, NStatementList* body)
	: NConditionStmt(condition, body), preStm(preStm), postExp(postExp), semiCol2(semiCol2) {}

	void genCode(CodeContext& context);

	NStatementList* getPreStm() const
	{
		return preStm;
	}

	NExpressionList* getPostExp() const
	{
		return postExp;
	}

	Token* getSemiCol2() const
	{
		return semiCol2;
	}

	~NForStatement()
	{
		delete preStm;
		delete postExp;
		delete semiCol2;
	}

	ADD_ID(NForStatement)
};

class NIfStatement : public NConditionStmt
{
	NStatementList* elseBody;
	Token* lparen;

public:
	NIfStatement(Token* lparen, NExpression* condition, NStatementList* ifBody, NStatementList* elseBody)
	: NConditionStmt(condition, ifBody), elseBody(elseBody), lparen(lparen) {}

	void genCode(CodeContext& context);

	Token* getToken() const
	{
		return lparen;
	}

	NStatementList* getElseBody() const
	{
		return elseBody;
	}

	~NIfStatement()
	{
		delete elseBody;
		delete lparen;
	}

	ADD_ID(NIfStatement)
};

class NLabelStatement : public NDeclaration
{
public:
	explicit NLabelStatement(Token* name)
	: NDeclaration(name) {}

	void genCode(CodeContext& context);

	ADD_ID(NLabelStatement)
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
	Token* retToken;

public:
	NReturnStatement(Token* retToken = nullptr, NExpression* value = nullptr)
	: value(value), retToken(retToken) {}

	void genCode(CodeContext& context);

	NExpression* getValue() const
	{
		return value;
	}

	Token* getToken() const
	{
		return retToken;
	}

	~NReturnStatement()
	{
		delete value;
		delete retToken;
	}

	ADD_ID(NReturnStatement)
};

class NGotoStatement : public NJumpStatement
{
	Token* name;

public:
	explicit NGotoStatement(Token* name)
	: name(name) {}

	void genCode(CodeContext& context);

	Token* getNameToken() const
	{
		return name;
	}

	const string& getName() const
	{
		return name->str;
	}

	~NGotoStatement()
	{
		delete name;
	}

	ADD_ID(NGotoStatement)
};

class NLoopBranch : public NJumpStatement
{
	Token* token;
	int type;
	NIntConst* level;

public:
	NLoopBranch(Token* token, int type, NIntConst* level = nullptr)
	: token(token), type(type), level(level) {}

	void genCode(CodeContext& context);

	Token* getToken() const
	{
		return token;
	}

	int getType() const
	{
		return type;
	}

	NIntConst* getLevel() const
	{
		return level;
	}

	~NLoopBranch()
	{
		delete token;
		delete level;
	}

	ADD_ID(NLoopBranch)
};

class NDeleteStatement : public NStatement
{
	NVariable *variable;
	Token* token;

public:
	NDeleteStatement(Token* token, NVariable* variable)
	: variable(variable), token(token) {}

	void genCode(CodeContext& context);

	NVariable* getVar() const
	{
		return variable;
	}

	Token* getToken() const
	{
		return token;
	}

	~NDeleteStatement()
	{
		delete variable;
		delete token;
	}

	ADD_ID(NDeleteStatement)
};

class NDestructorCall : public NStatement
{
	NVariable* baseVar;
	Token* thisToken;

public:
	NDestructorCall(NVariable* baseVar, Token* thisToken)
	: baseVar(baseVar), thisToken(thisToken) {}

	void genCode(CodeContext& context);

	NVariable* getVar() const
	{
		return baseVar;
	}

	Token* getToken() const
	{
		return thisToken;
	}

	~NDestructorCall()
	{
		delete baseVar;
	}

	ADD_ID(NDestructorCall)
};

class NOperatorExpr : public NExpression
{
protected:
	int oper;
	Token* opTok;

public:
	NOperatorExpr(int oper, Token* opTok)
	: oper(oper), opTok(opTok) {}

	int getOp() const
	{
		return oper;
	}

	Token* getToken() const
	{
		return opTok;
	}

	~NOperatorExpr()
	{
		delete opTok;
	}
};

class NAssignment : public NOperatorExpr
{
	NVariable* lhs;
	NExpression* rhs;

public:
	NAssignment(int oper, Token* opToken, NVariable* lhs, NExpression* rhs)
	: NOperatorExpr(oper, opToken), lhs(lhs), rhs(rhs) {}

	RValue genValue(CodeContext& context);

	NVariable* getLhs() const
	{
		return lhs;
	}

	NExpression* getRhs() const
	{
		return rhs;
	}

	~NAssignment()
	{
		delete lhs;
		delete rhs;
	}

	ADD_ID(NAssignment)
};

class NTernaryOperator : public NExpression
{
	NExpression* condition;
	NExpression* trueVal;
	NExpression* falseVal;
	Token* colTok;

public:
	NTernaryOperator(NExpression* condition, NExpression* trueVal, Token *colTok, NExpression* falseVal)
	: condition(condition), trueVal(trueVal), falseVal(falseVal), colTok(colTok) {}

	RValue genValue(CodeContext& context);

	NExpression* getCondition() const
	{
		return condition;
	}

	NExpression* getTrueVal() const
	{
		return trueVal;
	}

	NExpression* getFalseVal() const
	{
		return falseVal;
	}

	Token* getToken() const
	{
		return colTok;
	}

	~NTernaryOperator()
	{
		delete condition;
		delete trueVal;
		delete falseVal;
		delete colTok;
	}

	ADD_ID(NTernaryOperator)
};

class NNewExpression : public NExpression
{
	NDataType* type;
	Token* token;

public:
	NNewExpression(Token* token, NDataType* type)
	: type(type), token(token) {}

	RValue genValue(CodeContext& context);

	NDataType* getType() const
	{
		return type;
	}

	Token* getToken() const
	{
		return token;
	}

	~NNewExpression()
	{
		delete type;
		delete token;
	}

	ADD_ID(NNewExpression)
};

class NBinaryOperator : public NOperatorExpr
{
protected:
	NExpression* lhs;
	NExpression* rhs;

public:
	NBinaryOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NOperatorExpr(oper, opToken), lhs(lhs), rhs(rhs) {}

	NExpression* getLhs() const
	{
		return lhs;
	}

	NExpression* getRhs() const
	{
		return rhs;
	}

	~NBinaryOperator()
	{
		delete lhs;
		delete rhs;
	}
};

class NLogicalOperator : public NBinaryOperator
{
public:
	NLogicalOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, opToken, lhs, rhs) {}

	RValue genValue(CodeContext& context);

	ADD_ID(NLogicalOperator)
};

class NCompareOperator : public NBinaryOperator
{
public:
	NCompareOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, opToken, lhs, rhs) {}

	RValue genValue(CodeContext& context);

	ADD_ID(NCompareOperator)
};

class NBinaryMathOperator : public NBinaryOperator
{
public:
	NBinaryMathOperator(int oper, Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(oper, opToken, lhs, rhs) {}

	RValue genValue(CodeContext& context);

	ADD_ID(NBinaryMathOperator)
};

class NNullCoalescing : public NBinaryOperator
{
public:
	NNullCoalescing(Token* opToken, NExpression* lhs, NExpression* rhs)
	: NBinaryOperator(0, opToken, lhs, rhs) {}

	RValue genValue(CodeContext& context);

	ADD_ID(NNullCoalescing)
};

class NSizeOfOperator : public NExpression
{
public:
	enum OfType { DATA, EXP, NAME };

private:
	OfType type;
	NDataType* dtype;
	NExpression* exp;
	Token* name;
	Token* sizeTok;

public:
	NSizeOfOperator(Token* sizeTok, NDataType* dtype)
	: type(DATA), dtype(dtype), exp(nullptr), name(nullptr), sizeTok(sizeTok) {}

	NSizeOfOperator(Token* sizeTok, NExpression* exp)
	: type(EXP), dtype(nullptr), exp(exp), name(nullptr), sizeTok(sizeTok) {}

	NSizeOfOperator(Token* sizeTok, Token* name)
	: type(NAME), dtype(nullptr), exp(nullptr), name(name), sizeTok(sizeTok) {}

	RValue genValue(CodeContext& context);

	OfType getType() const
	{
		return type;
	}

	NExpression* getExp() const
	{
		return exp;
	}

	NDataType* getDataType() const
	{
		return dtype;
	}

	Token* getName() const
	{
		return name;
	}

	Token* getToken() const
	{
		return sizeTok;
	}

	bool isConstant() const
	{
		return type == EXP? exp->isConstant() : true;
	}

	~NSizeOfOperator()
	{
		delete dtype;
		delete exp;
		delete name;
		delete sizeTok;
	}

	ADD_ID(NSizeOfOperator)
};

class NUnaryOperator : public NOperatorExpr
{
protected:
	NExpression* unary;

public:
	NUnaryOperator(int oper, Token* opToken, NExpression* unary)
	: NOperatorExpr(oper, opToken), unary(unary) {}

	NExpression* getExp() const
	{
		return unary;
	}

	~NUnaryOperator()
	{
		delete unary;
	}
};

class NUnaryMathOperator : public NUnaryOperator
{
public:
	NUnaryMathOperator(int oper, Token* opToken, NExpression* unaryExp)
	: NUnaryOperator(oper, opToken, unaryExp) {}

	RValue genValue(CodeContext& context);

	ADD_ID(NUnaryMathOperator)
};

class NFunctionCall : public NVariable
{
	Token* name;
	NExpressionList* arguments;

public:
	NFunctionCall(Token* name, NExpressionList* arguments)
	: name(name), arguments(arguments) {}

	RValue genValue(CodeContext& context);

	RValue loadVar(CodeContext& context);

	Token* getToken() const
	{
		return name;
	}

	NExpressionList* getArguments() const
	{
		return arguments;
	}

	const string& getName() const
	{
		return name->str;
	}

	~NFunctionCall()
	{
		delete name;
		delete arguments;
	}

	ADD_ID(NFunctionCall)
};

class NMemberFunctionCall : public NVariable
{
	NVariable* baseVar;
	Token* dotToken;
	Token* funcName;
	NExpressionList* arguments;

public:
	NMemberFunctionCall(NVariable* baseVar, Token* dotToken, Token* funcName, NExpressionList* arguments)
	: baseVar(baseVar), dotToken(dotToken), funcName(funcName), arguments(arguments) {}

	RValue genValue(CodeContext& context);

	RValue loadVar(CodeContext& context);

	NVariable* getBaseVar() const
	{
		return baseVar;
	}

	Token* getDotToken() const
	{
		return dotToken;
	}

	Token* getFuncName() const
	{
		return funcName;
	}

	NExpressionList* getArguments() const
	{
		return arguments;
	}

	const string& getName() const
	{
		return funcName->str;
	}

	~NMemberFunctionCall()
	{
		delete baseVar;
		delete dotToken;
		delete funcName;
		delete arguments;
	}

	ADD_ID(NMemberFunctionCall)
};

class NIncrement : public NOperatorExpr
{
	NVariable* variable;
	bool isPostfix;

public:
	NIncrement(int oper, Token* opToken, NVariable* variable, bool isPostfix)
	: NOperatorExpr(oper, opToken), variable(variable), isPostfix(isPostfix) {}

	RValue genValue(CodeContext& context);

	NVariable* getVar() const
	{
		return variable;
	}

	bool postfix() const
	{
		return isPostfix;
	}

	~NIncrement()
	{
		delete variable;
	}

	ADD_ID(NIncrement)
};

#endif
