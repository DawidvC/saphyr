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

#include "ModuleWriter.h"

#include <fstream>
#include <iostream>
#include <sstream>

#include <llvm/IR/LegacyPassManager.h>
#include <llvm/ADT/Triple.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/FormattedStream.h>
#include <llvm/Support/Host.h>
#include <llvm/Support/TargetRegistry.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_os_ostream.h>
#include <llvm/Target/TargetOptions.h>

#include "Pass.h"

using namespace llvm::legacy;

bool ModuleWriter::validModule()
{
	ostringstream buff;
	raw_os_ostream out(buff);
	if (verifyModule(module, &out)) {
		cout << "compiler error: broken module" << endl << endl
			<< buff.str() << endl;
		return true;
	}
	return false;
}

#if LLVM_VERSION_MAJOR >= 6
ToolOutputFile* ModuleWriter::getOutFile(const string& name)
#else
tool_output_file* ModuleWriter::getOutFile(const string& name)
#endif
{
	sys::fs::OpenFlags OpenFlags = sys::fs::F_None;

	error_code error;

#if LLVM_VERSION_MAJOR >= 6
	auto outFile = new ToolOutputFile(name, error, OpenFlags);
#else
	auto outFile = new tool_output_file(name, error, OpenFlags);
#endif

	if (error) {
		delete outFile;
		cout << "compiler error: error opening file" << endl << error << endl;
		return nullptr;
	}
	return outFile;
}

void ModuleWriter::initTarget()
{
	InitializeAllTargets();
	InitializeAllTargetMCs();
	InitializeAllAsmPrinters();
	InitializeAllAsmParsers();
}

TargetMachine* ModuleWriter::getMachine()
{
	TargetOptions options;
	string err, features;
	Triple triple;

	triple.setTriple(sys::getDefaultTargetTriple());
	auto target = TargetRegistry::lookupTarget(triple.getTriple(), err);
	return target->createTargetMachine(triple.getTriple(), sys::getHostCPUName(), features, options, Reloc::Model::Static, CodeModel::Medium, CodeGenOpt::Default);
}

int ModuleWriter::run()
{
	llvm::legacy::PassManager clean;
	clean.add(new SimpleBlockClean());
	clean.run(module);

	if (validModule())
		return 1;

	if (config.count("llvmir"))
		outputIR();
	else
		outputNative();
	return 0;
}

void ModuleWriter::outputIR()
{
	llvm::legacy::PassManager pm;

	fstream irFile(filename.substr(0, filename.rfind('.')) + ".ll", fstream::out);
	raw_os_ostream irStream(irFile);

	pm.add(createPrintModulePass(irStream));

	pm.run(module);
}

void ModuleWriter::outputNative()
{
	llvm::legacy::PassManager pm;

	initTarget();
	auto objFile = getOutFile(filename.substr(0, filename.rfind('.')) + ".o");

	buffer_ostream objStream(objFile->os());
	unique_ptr<TargetMachine> machine(getMachine());
	machine->addPassesToEmitFile(pm, objStream, TargetMachine::CGFT_ObjectFile);

	pm.run(module);
}
