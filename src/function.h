#pragma once

#include "types.h"

DECLARE_TYPE(Function);
DECLARE_TYPE(VM);

#define FF_NATIVE 0x01

struct Function {
	const char *Name; // for debug purposes only
	u32 NumArgs;
	u32 Flags;
	union {
		struct {
			const u8 *Bytes;
			u32 Length;
		} Body;
		void (*Native)(VM *);
	};
};
