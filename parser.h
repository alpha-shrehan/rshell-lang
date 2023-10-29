#pragma once

#include "ast.h"
#include "cmds.h"
#include "expr.h"
#include "header.h"
#include "rfunction.h"
#include "rsarray.h"
#include "rstack.h"
#include "trie.h"
#include "vtable.h"

struct _rshell_mod_stru
{
  stmt_t *st_arr;
  size_t st_count;

  vtable_t *vt;
  struct _rshell_mod_stru *parent;

  char **use_globals;
  size_t ug_size;
};

struct _rshell_parser_ret_stru /* data which a parser routine returns after
                                  execution */
{
  bool sig_return;
  bool sig_break;
  bool sig_continue;
  bool sig_jump;

  bool consider_globals;

  struct
  {
    char *label_name;
  } vjmp;
};

typedef struct _rshell_mod_stru mod_t;
typedef struct _rshell_parser_ret_stru pret_t;

#ifdef __cplusplus
extern "C"
{
#endif

  RSHELL_API mod_t *rshell_parser_mod_new (stmt_t *, size_t, mod_t *);
  RSHELL_API void rshell_parser_mod_free (mod_t *);
  RSHELL_API pret_t rshell_parser_parse_tree (mod_t *);
  RSHELL_API pret_t rshell_parser_pret_new (void);
  RSHELL_API expr_t *rshell_parser_expr_eval (char *, mod_t *, pret_t *);
  RSHELL_API void rshell_parser_mod_var_add (mod_t *, char *, expr_t *);
  RSHELL_API expr_t *rshell_parser_mod_var_get (mod_t *, char *);
  RSHELL_API mod_t *rshell_parser_mod_copy_deep (mod_t *);

#ifdef __cplusplus
}
#endif
