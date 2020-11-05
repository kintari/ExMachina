#pragma

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

typedef uint32_t u32;
typedef int32_t s32;
typedef uint8_t u8;
typedef int8_t s8;

#define NORETURN __attribute__((__noreturn__))
#define DECLARE_TYPE(T); typedef struct T T;

#ifndef countof
#define countof(x) (sizeof(x) / sizeof((x)[0]))
#endif
