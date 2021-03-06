-include Configfile

# only set CXX if it's not defined
ifeq "$(origin CXX)" "default"
	CXX = clang++
endif

# build in release mode unless debug is true
ifeq ($(DEBUG),true)
	O_LEVEL = -ggdb -O0
else
	O_LEVEL = -O3
endif

# set coverage only if using g++
ifeq ($(COVERAGE),true)
	COV_CXX = --coverage
	COV_LD  = -coverage
endif

# set LLVM_VER to build with a non-default llvm version
# example: export LLVM_VER="-3.5"

WARNINGS = -Wall -Wextra -pedantic -Wno-unused-parameter
CXXFLAGS = -std=c++11 `llvm-config$(LLVM_VER) --cxxflags` $(O_LEVEL) $(COV_CXX) $(WARNINGS) -frtti -fexceptions -D__STRICT_ANSI__
LDFLAGS = -lboost_program_options -lboost_filesystem -lboost_system $(COV_LD)
COMPILER_LDFLAGS = $(LDFLAGS) `llvm-config$(LLVM_VER) --ldflags` -lLLVM-`llvm-config$(LLVM_VER) --version`
COMPILER = ../saphyr
FORMATTER = ../syfmt

objs = parser.o scanner.o Util.o

compiler_objs = $(objs) Type.o Value.o Instructions.o Builder.o CGNInt.o CGNDataType.o \
	CGNVariable.o CGNExpression.o CGNStatement.o CGNImportStm.o Pass.o ModuleWriter.o \
	CGNImportList.o main.o

fmt_objs = $(objs) format/WriterUtil.o format/FMNDataType.o format/FMNExpression.o \
	format/FMNStatement.o format/fmtMain.o

all : formatter compiler

compiler : frontend $(compiler_objs)
	$(CXX) $(compiler_objs) -o $(COMPILER) $(COMPILER_LDFLAGS)

formatter : frontend $(fmt_objs)
	$(CXX) $(fmt_objs) -o $(FORMATTER) $(LDFLAGS)

frontend : parser.cpp scanner.cpp

frontend-docker :
	sudo docker run --rm -v $(PWD):/usr/src/saphyr -w /usr/src/saphyr jdm64/saphyr make frontend

parser.cpp : Parser.y
	rm -f parser*
	bisonc++ -V Parser.y
	sed -i -e '/Scanner d_scanner;/c\	unique_ptr<Scanner> d_scanner;' parser.h
	sed -i -e '/scannerobject/a\	unique_ptr<NStatementList> root;' parser.h
	sed -i -e '/public:/a\	Token getError() { string token = d_scanner->matched().size()? d_scanner->matched() : "<EOF>"; return Token("Syntax error on: " + token, d_scanner->filename(), d_scanner->lineNr(), d_scanner->colNr()); }' parser.h
	sed -i -e '/public:/a\	NStatementList* getRoot() { return root.get(); }' parser.h
	sed -i -e '/public:/a\	Parser(string filename){ d_scanner = unique_ptr<Scanner>(new Scanner(filename, "-")); d_scanner->setSval(&d_val__); }' parser.h
	sed -i -e '/return d_scanner.lex();/c\	return d_scanner->lex();' parser.ih
	sed -i -e '/Syntax error/d' parser.cpp
	sed -i -e '/Syntax error/d' parser.ih

scanner.cpp : Scanner.l parser.cpp
	rm -f scanner*
	flexc++ Scanner.l
	sed -i -e '/insert lexFunctionDecl/a\void setSval(ParserBase::STYPE__ *dval){ sval = dval; } ParserBase::STYPE__* sval;' scanner.h
	sed -i -e '/nsert baseclass_h/a\#include "parserbase.h"' scanner.h
	sed -i -e '/insert class_h/a\#include "parserbase.h"' scanner.ih
	sed -i -e '/insert class_h/a\#define SAVE_TOKEN sval->t_tok = new Token(matched(), filename(), lineNr(), colNr() - matched().length());' scanner.ih
	sed -i -e '/d_lineNr(1)/id_col(1),' scanner.cpp
	sed -i -e '/d_lineNr(lineNr)/id_col(1),' scanner.cpp
	sed -i -e '/++d_lineNr/acol_max = d_col; d_col = 0;' scanner.cpp
	sed -i -e '/default:/ad_col++;' scanner.cpp
	sed -i -e '/ch < 0x100/i--d_col;' scanner.cpp
	sed -i -e '/--d_lineNr;/c{ --d_lineNr; d_col = col_max; }' scanner.cpp
	sed -i -e '/size_t d_lineNr;/asize_t d_col; size_t col_max;' scannerbase.h
	sed -i -e '/size_t lineNr()/isize_t colNr() const { return d_col; }' scannerbase.h
	sed -i -e '/insert interactiveDecl/isize_t colNr() { return d_input.colNr(); }' scannerbase.h

clean :
	rm -f $(COMPILER) $(FORMATTER) *.o *~ format/*.o format/*~

frontend-clean :
	rm -f parser* scanner*

fullclean : clean frontend-clean

%.o : %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

cppcheck :
	cppcheck --enable=all --inconclusive ./ 1> /dev/null

analyze : clean
	scan-build$(LLVM_VER) make compiler

tests : all
	cd ../tests; ./unitTest.py $(UNITTEST_ARG)

docker-dev :
	sudo docker run -it --rm -v $(PWD)/../:/usr/src/saphyr -w /usr/src/saphyr/src jdm64/saphyr bash

appimage :
	sudo docker run -it --rm -v $(PWD)/../:/usr/src/saphyr -w /usr/src/saphyr/src jdm64/saphyr make appimage-filecopy
	cd ../scripts; ./buildAppImage-create.sh

appimage-filecopy : compiler
	cd ../scripts; ./buildAppImage-filecopy.sh
