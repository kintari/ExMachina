#pragma once

#define MNEMONICS \
	X(NOP) \
	X(PUSH) \
	X(POP) \
	X(DUP) \
	X(XCHG) \
	X(CALL) \
	X(RET) \
	X(ADD) \
	X(SUB) \
	X(MUL) \
	X(DIV) \
	X(BZ) \
	X(BNZ) \
	X(BNE) \
	X(LT) \
	X(LTE) \
	X(HALT) \
	X(PANIC)

#define X(m) m,
typedef enum {
	MNEMONICS
} Opcode;
#undef X

/*
enum Opcode {
	PUSH = 0x01,
	POP = 0x02,
	DUP = 0x03,
	XCHG = 0x04,

	call = 0x08,
	ret = 0x09,

	add = 0x11,
	sub = 0x12,
	mul = 0x13,
	DIV = 0x14,

	bz = 0x21,
	bnz = 0x22,
	bne = 0x23,

	halt = 0x31,
	panic = 0x32,

	lt = 0x40,
	lte,
};*/