#pragma once

#include <assert.h>
#include <gc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RSHELL_Malloc(X) rshell_malloc ((X))
#define RSHELL_Realloc(X, Y) rshell_realloc ((X), (Y))
#define RSHELL_Free(X) rshell_free ((X))
#define RSHELL_Strdup(X) rshell_strdup ((X))

#ifndef RSHELL_API
#define RSHELL_API
#endif

#ifdef __cplusplus
extern "C"
{
#endif

  RSHELL_API void *rshell_malloc (size_t);
  RSHELL_API void *rshell_realloc (void *, size_t);
  RSHELL_API void rshell_free (void *);
  RSHELL_API char *rshell_strdup (char *);

#ifdef __cplusplus
}
#endif
