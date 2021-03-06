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
#ifndef __CODE_CONTEXT_H__
#define __CODE_CONTEXT_H__

#include <stack>
#include <list>
#include <iostream>
#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/LLVMContext.h>
#include "BaseNodes.h"
#include "Value.h"

using namespace boost::program_options;
using namespace boost::filesystem;

enum BranchType { BREAK = 1, CONTINUE = 1 << 1, REDO = 1 << 2 };

struct LabelBlock
{
	BasicBlock* block;
	Token token;
	bool isPlaceholder;

	LabelBlock(BasicBlock* block, Token* token, bool isPlaceholder)
	: block(block), token(*token), isPlaceholder(isPlaceholder) {}
};

typedef unique_ptr<LabelBlock> LabelBlockPtr;
#define smart_label(block, token, placeholder) unique_ptr<LabelBlock>(new LabelBlock(block, token, placeholder))

class ScopeTable
{
	map<string, RValue> table;

public:
	void storeSymbol(RValue var, const string& name)
	{
		table[name] = var;
	}

	RValue loadSymbol(const string& name) const
	{
		auto varData = table.find(name);
		return varData != table.end()? varData->second : RValue();
	}
};

class SymbolTable
{
	ScopeTable globalTable;
	vector<ScopeTable> localTable;

public:
	void storeGlobalSymbol(RValue var, const string& name)
	{
		globalTable.storeSymbol(var, name);
	}

	void storeLocalSymbol(RValue var, const string& name)
	{
		localTable.back().storeSymbol(var, name);
	}

	void pushLocalTable()
	{
		localTable.push_back(ScopeTable());
	}

	void popLocalTable()
	{
		localTable.pop_back();
	}

	void clearLocalTable()
	{
		localTable.clear();
	}

	RValue loadSymbolLocal(const string& name) const
	{
		for (auto it = localTable.rbegin(); it != localTable.rend(); it++) {
			auto var = it->loadSymbol(name);
			if (var)
				return var;
		}
		return {};
	}

	RValue loadSymbolGlobal(const string& name) const
	{
		return globalTable.loadSymbol(name);
	}

	RValue loadSymbol(const string& name) const
	{
		auto var = loadSymbolLocal(name);
		return var? var : globalTable.loadSymbol(name);
	}

	RValue loadSymbolCurr(const string& name) const
	{
		return localTable.empty()? globalTable.loadSymbol(name) : localTable.back().loadSymbol(name);
	}
};

class CodeContext : public SymbolTable
{
	friend class SType;
	friend class SFunction;
	friend class SUserType;
	friend class SStructType;
	friend class SUnionType;
	friend class SEnumType;
	friend class SOpaqueType;

	typedef vector<llvm::BasicBlock*> BlockVector;
	typedef BlockVector::iterator block_iterator;

	BlockVector funcBlocks;
	BlockVector continueBlocks;
	BlockVector breakBlocks;
	BlockVector redoBlocks;
	map<string, LabelBlockPtr> labelBlocks;

	vector<pair<Token,string>> errors;
	list<unique_ptr<NAttributeList>> attrs;

	Module* module;
	TypeManager typeManager;
	SFunction currFunc;
	SClassType* currClass;
	set<path> allFiles;
	vector<path> filesStack;

	void validateFunction()
	{
		for (auto& item : labelBlocks) {
			if (item.second->isPlaceholder)
				addError("label " + item.first + " not defined", &item.second->token);
		}
	}

	BasicBlock* loopBranchLevel(const BlockVector& branchBlocks, size_t level) const
	{
		auto idx = level > 0? branchBlocks.size() - level : abs((int) level) - 1;
		return (idx >= 0 && idx < branchBlocks.size())? branchBlocks[idx] : nullptr;
	}

public:
	explicit CodeContext(Module* module)
	: module(module), typeManager(module), currClass(nullptr)
	{
	}

	operator LLVMContext&()
	{
		return module->getContext();
	}

	operator BasicBlock*() const
	{
		return currBlock();
	}

	Module* getModule() const
	{
		return module;
	}

	path currFile() const
	{
		return filesStack.back();
	}

	void pushFile(const path& filename)
	{
		filesStack.push_back(filename);
		allFiles.insert(filename);
	}

	void popFile()
	{
		filesStack.pop_back();
	}

	bool fileLoaded(const path& filename)
	{
		return allFiles.find(filename) != allFiles.end();
	}

	SFunction currFunction() const
	{
		return currFunc;
	}

	NAttributeList* storeAttr(NAttributeList* list)
	{
		if (list) {
			list = list->move<NAttributeList>(false);
			attrs.push_back(unique_ptr<NAttributeList>(list));
		}
		return list;
	}

	void addError(string error, Token* token)
	{
		if (token)
			errors.push_back({*token, error});
		else
			errors.push_back({Token(), error});
	}

	BasicBlock* currBlock() const
	{
		return funcBlocks.back();
	}

	void setClass(SClassType* classType)
	{
		currClass = classType;
	}

	SClassType* getClass() const
	{
		return currClass;
	}

	void startFuncBlock(SFunction function)
	{
		pushLocalTable();
		funcBlocks.clear();
		funcBlocks.push_back(BasicBlock::Create(module->getContext(), "", function));
		currFunc = function;
	}

	void endFuncBlock()
	{
		validateFunction();

		clearLocalTable();
		funcBlocks.clear();
		continueBlocks.clear();
		breakBlocks.clear();
		redoBlocks.clear();
		labelBlocks.clear();

		currFunc = SFunction();
	}

	void pushBlock(BasicBlock* block)
	{
		block->moveAfter(currBlock());
		funcBlocks.push_back(block);
	}

	BasicBlock* createContinueBlock()
	{
		auto block = createBlock();
		continueBlocks.push_back(block);
		return block;
	}

	BasicBlock* getContinueBlock(int level = 1) const
	{
		return loopBranchLevel(continueBlocks, level);
	}

	BasicBlock* createBreakBlock()
	{
		auto block = createBlock();
		breakBlocks.push_back(block);
		return block;
	}

	BasicBlock* getBreakBlock(int level = 1) const
	{
		return loopBranchLevel(breakBlocks, level);
	}

	BasicBlock* createRedoBlock()
	{
		auto block = createBlock();
		redoBlocks.push_back(block);
		return block;
	}

	BasicBlock* getRedoBlock(int level = 1) const
	{
		return loopBranchLevel(redoBlocks, level);
	}

	void popLoopBranchBlocks(int type)
	{
		if (type & BranchType::BREAK)
			breakBlocks.pop_back();
		if (type & BranchType::CONTINUE)
			continueBlocks.pop_back();
		if (type & BranchType::REDO)
			redoBlocks.pop_back();
	}

	LabelBlock* getLabelBlock(const string& name)
	{
		return labelBlocks[name].get();
	}
	LabelBlock* createLabelBlock(Token* name, bool isPlaceholder)
	{
		LabelBlockPtr &item = labelBlocks[name->str];
		if (!item.get()) {
			item = smart_label(createBlock(), name, isPlaceholder);
			item.get()->block->setName(name->str);
		}
		return item.get();
	}

	// NOTE: can only be used inside a function to add a new block
	BasicBlock* createBlock() const
	{
		return BasicBlock::Create(module->getContext(), "", currBlock()->getParent());
	}

	/*
	 * Returns true on errors
	 */
	bool handleErrors() const
	{
		if (errors.empty())
			return false;

		for (auto& error : errors) {
			cout << error.first.filename << ":" << error.first.line << ":" << error.first.col << ": " << error.second << endl;
		}
		cout << "found " << errors.size() << " errors" << endl;
		return true;
	}
};

#endif
