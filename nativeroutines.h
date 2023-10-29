#pragma once

#include "header.h"

struct _rshell_expr_stru;
struct _rshell_mod_stru;
struct _rshell_parser_ret_stru;

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief $len($iterable)
   *
   * @return struct _rshell_expr_stru*
   */
  struct _rshell_expr_stru *
  rshell_native_routine_len (struct _rshell_mod_stru *,
                             struct _rshell_parser_ret_stru *);

#ifdef __cplusplus
}
#endif
