#pragma once

#include "header.h"

struct _rshell_stmt_stru;
struct _rshell_expr_stru;
struct _rshell_mod_stru;
struct _rshell_parser_ret_stru;

struct _rshell_rfun_stru
{
  char *name;
  char **args;
  size_t arg_count;
  bool isnative;
  int id;
  struct _rshell_mod_stru *parent;

  union
  {
    struct
    {
      struct _rshell_stmt_stru *body;
      size_t body_count;
    } coded;

    struct _rshell_expr_stru *(*f_native) (struct _rshell_mod_stru *,
                                           struct _rshell_parser_ret_stru *);

  } v;
};

typedef struct _rshell_rfun_stru fun_t;

#ifdef __cplusplus
extern "C"
{
#endif

  RSHELL_API void rshell_fun_init (void);
  RSHELL_API fun_t *rshell_fun_new (char *_Name, char **_Args,
                                    size_t _ArgCount, bool _IsNative,
                                    struct _rshell_mod_stru *_Parent);
  RSHELL_API fun_t *rshell_fun_add (fun_t *);
  RSHELL_API fun_t *rshell_fun_get_byID (int);
  RSHELL_API fun_t *rshell_fun_get_byName (char *);
  RSHELL_API int
  rshell_fun_addNativeFunctions (struct _rshell_mod_stru *, int _FunCount,
                                 ... /* speficy functions to add */);

#ifdef __cplusplus
}
#endif
