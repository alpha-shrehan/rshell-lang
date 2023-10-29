#pragma once

#include "expr.h"
#include "header.h"

#ifdef __cplusplus
extern "C"
{
#endif

  RSHELL_API void rshell_rst_exprStackInit (void);
  RSHELL_API expr_t ***rshell_rst_getExprStack (void);
  RSHELL_API size_t *rshell_rst_getExprStackSize (void);
  RSHELL_API expr_t *rshell_rst_esAppend (expr_t *);
  RSHELL_API void rshell_rst_esModifyObjCount (expr_t *, int);

#ifdef __cplusplus
}
#endif
