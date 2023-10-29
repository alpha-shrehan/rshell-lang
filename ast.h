#pragma once

#include "header.h"

enum
{
  RSL_STMT_VAR_DECL,
  RSL_STMT_BLOCK_IF,
  RSL_STMT_FUN_CALL,
  RSL_STMT_FUN_DECL,
  RSL_STMT_STRING,
  RSL_STMT_COMMAND,
  RSL_STMT_VAR,
  RSL_STMT_GOTO,
  RSL_STMT_DECREMENT,
  RSL_STMT_INCREMENT,
  RSL_STMT_USE,
  RSL_STMT_CLASS_DECL,
  RSL_STMT_MEM_ACCESS,
  RSL_STMT_COMMENT,
};

struct _rshell_expr_stru;
struct _rshell_stmt_stru
{
  int type;

  union
  {
    struct
    {
      char *name;
      char *val;
    } var_decl;

    struct
    {
      char *cond;
      char *body;

      char **elif_conds;
      char **elif_bodies;
      size_t elif_count;

      char *body_else;

      bool ast_ed; /* ast generated? */
      struct _rshell_stmt_stru *body_ast;
      size_t body_ast_count;

      struct _rshell_stmt_stru **elif_bodies_ast;
      size_t *elif_body_ast_size_counts;
      size_t elif_body_ast_count;

      struct _rshell_stmt_stru *body_else_ast;
      size_t body_else_ast_count;

    } block_if;

    struct
    {
      char *callee;
      char **args;
      size_t arg_count;

      bool eval_callee;
    } fun_call;

    struct
    {
      char *fname;
      char **args;
      size_t arg_count;

      struct _rshell_stmt_stru *body;
      size_t body_count;
    } fun_decl;

    struct
    {
      char *val;
    } stmt_string;

    struct
    {
      char *name;
      char *args;
    } stmt_cmd;

    struct
    {
      char *name;
    } stmt_var;

    struct
    {
      char *name;
    } stmt_goto;

    struct
    {
      char *name;
    } stmt_dec;

    struct
    {
      char *name;
    } stmt_inc;

    struct
    {
      char *name;
    } stmt_use;

    struct
    {
      char *name;
      struct _rshell_stmt_stru *body;
      size_t body_count;
    } stmt_class_decl;

    struct
    {
      char *attr;
      struct _rshell_stmt_stru *parent;
    } mem_access;

    struct
    {
      char *val;
    } stmt_cmt;

  } v;

  struct
  {
    bool is_labelled;
    char *label_name;
  } meta;
};

typedef struct _rshell_stmt_stru stmt_t;

#ifdef __cplusplus
extern "C"
{
#endif

  RSHELL_API stmt_t *rshell_ast_stmt_new (int);
  /* evaluates single statements */
  RSHELL_API stmt_t rshell_ast_gen (char *);
  RSHELL_API void rshell_ast_print_stmt (stmt_t);

  RSHELL_API int rget_next_chr (char *data, char tar, int st, bool case_str,
                                bool case_brack);
  RSHELL_API void rshell_ast_preprocess_tree (stmt_t **, size_t *);

#ifdef __cplusplus
}
#endif