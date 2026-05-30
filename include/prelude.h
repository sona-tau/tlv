#pragma once
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

inline static void free_ptr(void* p) { free(*(void**)p); }
#define autofree __attribute__((cleanup(free_ptr)))

#define exists(a)                                                              \
  do {                                                                         \
    if (a == NULL) {                                                           \
      fprintf(stderr, "%s:%s:%d Null pointer encountered: %s\n", __FILE__,     \
              __func__, __LINE__, strerror(errno));                            \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

/// Macro for marking parts that have not been implemented.
#define TODO                                                                   \
  do {                                                                         \
    fprintf(stderr, "%s:%s:%d Not yet implemented\n", __FILE__, __func__,      \
            __LINE__);                                                         \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

/// Macro para crashear el programa si una condicion no se cumple
#define expect(cond, ...)                                                      \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, __VA_ARGS__);                                            \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)

/* ----- Types ----- */

/// Simple Byte type. Guaranteed to be exactly 8 bits.
typedef uint8_t Byte;

/// String type.
typedef const char *Str;
