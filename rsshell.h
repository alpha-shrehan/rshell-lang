#pragma once

#include "ast.h"
#include "classes.h"
#include "cmds.h"
#include "expr.h"
#include "header.h"
#include "parser.h"
#include "rstack.h"
#include "vtable.h"

#ifdef __cpluscplus
extern "C"
{
#endif

  RSHELL_API char *rshell_file_read (char *_FName);
  RSHELL_API char **rshell_str_split_delim (char *_Data, size_t *_Rp, char);
  RSHELL_API char *rshell_unescape_str (const char *input);

#ifdef __cplusplus
}
#endif
