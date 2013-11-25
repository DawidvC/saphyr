#!/usr/bin/env python3
#
# Saphyr, a C++ style compiler using LLVM
# Copyright (C) 2012, Justin Madru (justin.jdm64@gmail.com)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import os, sys
from subprocess import call, Popen, PIPE
from glob import glob

SAPHYR_BIN = "../saphyr"
ENCODING = "utf-8"
TEST_EXT = ".tst"
SYP_EXT = ".syp"
LL_EXT = ".ll"
ERR_EXT = ".err"
EXP_EXT = ".exp"
PADDING = 0

def runCmd(cmd):
	p = Popen(cmd, stdout=PIPE, stderr=PIPE)
	p.wait()
	out, err = p.communicate()
	return [p.returncode, out.decode(ENCODING), err.decode(ENCODING)]

def getFiles(files):
	if not isinstance(files, type([])):
		files = [files]
	elif not files:
		return glob("*" + TEST_EXT)
	fileList = []
	for f in files:
		dot = f.rfind('.')
		if dot > 0:
			f = f[0:dot]
		fileList.extend(glob("*" + f + "*" + TEST_EXT))
	return fileList

def createTestFiles(filename):
	basename = filename[0 : filename.rfind(".")]
	with open(filename) as testFile:
		data = testFile.read().split("========")
		if len(data) != 2:
			return True
		with open(basename + SYP_EXT, "w") as sourceFile:
			sourceFile.write(data[0])
		with open(basename + EXP_EXT, "w") as asmFile:
			asmFile.write(data[1])

def updateTest(file):
	basename = file[0 : file.rfind(".")]
	with open(basename + SYP_EXT) as sourceF, open(basename + LL_EXT) as llvmF, open(file, "w") as tstF:
		tstF.write(sourceF.read())
		tstF.write("========\n\n")
		tstF.write(llvmF.read())

def cleanSingleTest(file):
	basename = file[0 : file.rfind(".")]
	runCmd(["rm", basename + SYP_EXT, basename + LL_EXT, basename + EXP_EXT, basename + ERR_EXT])

def runSingleTest(file, update=False):
	basename = file[0 : file.rfind(".")]
	p = runCmd([SAPHYR_BIN, basename + SYP_EXT])
	if p[0] != 0:
		print(file.ljust(PADDING) + " = [compile error]")
		with open(basename + ERR_EXT, "w") as log:
			log.write(p[2])
			log.write(p[1])
		return True
	p = runCmd(["diff", "-ubB", basename + EXP_EXT, basename + LL_EXT])
	if p[0] != 0:
		if update:
			print(file.ljust(PADDING) + " = [updated]")
			updateTest(file)
			return False
		print(file.ljust(PADDING) + " = [output differs]")
		with open(basename + ERR_EXT, "w") as log:
			log.write(p[2])
			log.write(p[1])
		return True
	print(file.ljust(PADDING) + " = [ok]")
	return False

def cleanTests(files):
	files = getFiles(files)
	for file in files:
		cleanSingleTest(file)

def runTests(files, clean=True, update=False):
	global PADDING
	files = getFiles(files)
	if not files:
		return
	PADDING = len(max(files, key=len))
	failed = 0
	total = len(files)
	for file in files:
		if createTestFiles(file):
			print(file.ljust(PADDING) + " = [missing section]")
			failed += 1
			continue
		error = runSingleTest(file, update)
		if not error and clean:
			cleanSingleTest(file)
		failed += error
	passed = total - failed
	print(str(passed) + " / " + str(total) + " tests passed")

def main():
	args = sys.argv[1:]
	if not args:
		runTests(args)
		return

	{"--clean": lambda: cleanTests(args[1:]),
	      "-c": lambda: cleanTests(args[1:]),
	"--update": lambda: runTests(args[1:], True, True),
	      "-u": lambda: runTests(args[1:], True, True),
	  "--dump": lambda: runTests(args[1:], False, False),
	      "-d": lambda: runTests(args[1:], False, False)
	}.get(args[0], lambda: runTests(args))()

if __name__ == "__main__":
	main()
