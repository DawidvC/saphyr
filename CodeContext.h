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
#ifndef __CODE_CONTEXT_H__
#define __CODE_CONTEXT_H__

#include <stack>
#include <llvm/Instructions.h>
#include <llvm/BasicBlock.h>
#include "Value.h"
#include "Function.h"

// forward declarations
class NStatement;
class NStatementList;

struct LabelBlock
{
	BasicBlock* block;
	bool isPlaceholder;

	LabelBlock(BasicBlock* block, bool isPlaceholder)
	: block(block), isPlaceholder(isPlaceholder) {}
};

typedef unique_ptr<LabelBlock> LabelBlockPtr;
#define smart_label(block, placeholder) unique_ptr<LabelBlock>(new LabelBlock(block, placeholder))

class VarTable
{
	map<string, LValue> table;

public:
	void storeVar(LValue var, string* name)
	{
		table[*name] = var;
	}

	LValue loadVar(string* name) const
	{
		auto varData = table.find(*name);
		return varData != table.end()? varData->second : LValue();
	}
};

class SymbolTable
{
	VarTable globalTable;
	vector<VarTable> localTable;

public:
	void storeGlobalVar(LValue var, string* name)
	{
		globalTable.storeVar(var, name);
	}

	void storeLocalVar(LValue var, string* name)
	{
		localTable.back().storeVar(var, name);
	}

	void pushLocalTable()
	{
		localTable.push_back(VarTable());
	}

	void popLocalTable()
	{
		localTable.pop_back();
	}

	void clearLocalTable()
	{
		localTable.clear();
	}

	LValue loadVar(string* name) const
	{
		for (auto it = localTable.rbegin(); it != localTable.rend(); it++) {
			auto var = it->loadVar(name);
			if (var)
				return var;
		}
		return globalTable.loadVar(name);
	}

	LValue loadVarCurr(string* name)
	{
		return localTable.empty()? globalTable.loadVar(name) : localTable.back().loadVar(name);
	}
};

class CodeContext : public SymbolTable
{
	friend class SType;
	friend class SFunction;
	friend class SUserType;

	vector<BasicBlock*> funcBlocks;
	vector<BasicBlock*> continueBlocks;
	vector<BasicBlock*> breakBlocks;
	vector<BasicBlock*> redoBlocks;
	map<string, LabelBlockPtr> labelBlocks;

	string filename;
	vector<string> errors;
	int returncode;

	Module* module;
	TypeManager typeManager;
	FunctionManager funcManager;

	void validateFunction()
	{
		for (auto& item : labelBlocks) {
			if (item.second->isPlaceholder)
				addError("label \"" + item.first + "\" not defined");
		}
	}

	BasicBlock* loopBranchLevel(const vector<BasicBlock*>& branchBlocks, size_t level) const
	{
		auto idx = level > 0? branchBlocks.size() - level : abs(level) - 1;
		return (idx >= 0 && idx < branchBlocks.size())? branchBlocks[idx] : nullptr;
	}

public:
	CodeContext(string& filename)
	: filename(filename), returncode(0), module(new Module(filename, getGlobalContext())),
	typeManager(module), funcManager(module)
	{
	}

	~CodeContext()
	{
		delete module;
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

	SFunction* getFunction(string* name)
	{
		return funcManager.getFunction(name);
	}

	SFunction* currFunction() const
	{
		return funcManager.current();
	}

	void addError(string error)
	{
		errors.push_back(error);
	}

	BasicBlock* currBlock() const
	{
		return funcBlocks.back();
	}

	void startFuncBlock(SFunction* function)
	{
		pushLocalTable();
		funcBlocks.clear();
		funcBlocks.push_back(BasicBlock::Create(module->getContext(), "", *function));
		funcManager.setCurrent(function);
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

		funcManager.setCurrent(nullptr);
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
	void popContinueBlock()
	{
		continueBlocks.pop_back();
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
	void popBreakBlock()
	{
		breakBlocks.pop_back();
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
	void popRedoBlock()
	{
		redoBlocks.pop_back();
	}
	BasicBlock* getRedoBlock(int level = 1) const
	{
		return loopBranchLevel(redoBlocks, level);
	}

	void popLoopBranchBlocks()
	{
		popBreakBlock();
		popContinueBlock();
		popRedoBlock();
	}

	LabelBlock* getLabelBlock(string* name)
	{
		return labelBlocks[*name].get();
	}
	LabelBlock* createLabelBlock(string* name, bool isPlaceholder)
	{
		LabelBlockPtr &item = labelBlocks[*name];
		if (!item.get()) {
			item = smart_label(createBlock(), isPlaceholder);
			item.get()->block->setName(*name);
		}
		return item.get();
	}

	// NOTE: can only be used inside a function to add a new block
	BasicBlock* createBlock() const
	{
		return BasicBlock::Create(module->getContext(), "", currBlock()->getParent());
	}

	RValue errValue() const
	{
		return RValue::getZero(typeManager.getInt(32));
	}

	void genCode(NStatementList stms);

	int returnCode() const
	{
		return returncode;
	}
};

#endif
