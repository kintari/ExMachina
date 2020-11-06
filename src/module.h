#pragma once

#include "types.h"
#include "function.h"

typedef enum {
    CONSTANT_NONE,
    CONSTANT_INT,
    CONSTANT_DOUBLE,
    CONSTANT_STRING,
    CONSTANT_METHOD,
    CONSTANT_CLASS,
    CONSTANT_BYTEARRAY,
} ConstantType;

typedef struct Constant {
    ConstantType Type;
} Constant;

typedef struct {
    ConstantType Type;
    const char *Value;
    size_t Length;
} StringConstant;

typedef struct {
    ConstantType Type;
    u8 *Bytes;
    size_t Count;
} ByteArrayConstant;

typedef struct {
    ConstantType Type;
    u32 ClassInfo, Name; // indices into constant table
    u32 NumParams; // just a value
    u32 Body; // index to byte array
} MethodInfo;

typedef struct {
    ConstantType Type;
    u32 Name; // index into constant table
} ClassInfo;

typedef struct ConstantTable {
    Constant **Entries;
    u32 Count;
} ConstantTable;

typedef struct Module {
	const Function *Functions;
	u32 NumFunctions;
    ConstantTable *Constants;
} Module;

const Module *LoadModule();