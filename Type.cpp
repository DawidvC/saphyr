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
#include "CodeContext.h"

SType* SType::getVoid(CodeContext& context)
{
	return context.typeManager.getVoid();
}

SType* SType::getBool(CodeContext& context)
{
	return context.typeManager.getBool();
}

SType* SType::getInt(CodeContext& context, int bitWidth)
{
	return context.typeManager.getInt(bitWidth);
}

SType* SType::getFloat(CodeContext& context, bool doubleType)
{
	return context.typeManager.getFloat(doubleType);
}

SType* SType::getArray(CodeContext& context, SType* arrType, uint64_t size)
{
	return context.typeManager.getArray(arrType, size);
}

SFunctionType* SType::getFunction(CodeContext& context, SType* returnTy, vector<SType*> params)
{
	return context.typeManager.getFunction(returnTy, params);
}

SType* SType::opType(CodeContext& context, SType* ltype, SType* rtype, bool int32min)
{
	auto btype = ltype->tclass | rtype->tclass;
	if (btype & DOUBLE)
		return SType::getFloat(context, true);
	else if (btype & FLOATING)
		return SType::getFloat(context);

	auto lbits = ltype->intSize();
	auto rbits = rtype->intSize();
	if (lbits > rbits)
		return (int32min && lbits < 32)? SType::getInt(context, 32) : ltype;
	else
		return (int32min && rbits < 32)? SType::getInt(context, 32) : rtype;
}

TypeManager::TypeManager(LLVMContext& ctx)
: context(ctx)
{
	voidTy = smart_stype(SType::VOID, Type::getVoidTy(context), 0, nullptr);
	boolTy = smart_stype(SType::INTEGER, Type::getInt1Ty(context), 1, nullptr);
	int8Ty = smart_stype(SType::INTEGER, Type::getInt8Ty(context), 8, nullptr);
	int16Ty = smart_stype(SType::INTEGER, Type::getInt16Ty(context), 16, nullptr);
	int32Ty = smart_stype(SType::INTEGER, Type::getInt32Ty(context), 32, nullptr);
	int64Ty = smart_stype(SType::INTEGER, Type::getInt64Ty(context), 64, nullptr);
	floatTy = smart_stype(SType::FLOATING, Type::getFloatTy(context), 0, nullptr);
	doubleTy = smart_stype(SType::FLOATING | SType::DOUBLE, Type::getDoubleTy(context), 0, nullptr);
}
