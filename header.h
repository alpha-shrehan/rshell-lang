#pragma once

#include <assert.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#include "conio.h"
#include "windows.h"
#endif

#include "mem.h"

#define RSHELL_API

#ifdef DEBUG
#define dprintf
#else
#define dprintf
#endif

#define here printf ("%d: %s\n", __LINE__, __FILE__)

#ifdef __cplusplus
extern "C"
{
#endif

  RSHELL_API char *rshell_trim (char *);

#ifdef __cplusplus
}
#endif
