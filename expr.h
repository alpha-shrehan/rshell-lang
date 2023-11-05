#pragma once

#include "header.h"

enum
{
  RSHELL_EXPR_NOTYPE,
  RSHELL_EXPR_INT,
  RSHELL_EXPR_STRING,
  RSHELL_EXPR_FLOAT,
  RSHELL_EXPR_SEQUENCE,
  RSHELL_EXPR_FUNCTION,
  RSHELL_EXPR_ARITH,
  RSHELL_EXPR_CLASS,
  RSHELL_EXPR_COBJ,
};

struct _rshell_expr_stru
{
  int type;

  union
  {
    struct
    {
      char *val;
    } e_notype;

    struct
    {
      int32_t val;
    } e_int;

    struct
    {
      char *val;
    } e_string;

    struct
    {
      float val;
    } e_float;

    struct
    {
      int id;
    } e_seq;

    struct
    {
      int id;
    } e_func;

    struct
    {
      char op; /* +, -, *, /, % */
      struct _rshell_expr_stru *lval, *rval;
    } e_arith;

    struct
    {
      int id;
    } e_class;

    struct
    {
      int id;
    } e_cobj;

  } v;

  struct
  {
    bool isallocated;
    int id;
    int objcount;
    struct _rshell_expr_stru *
        *arg_pass; /* argument array to pass internally, like $this. */
    size_t ap_count;
  } meta;
};

typedef struct _rshell_expr_stru expr_t;

#define RSEXPR_IS_NUM(X)                                                      \
  ((X).type == RSHELL_EXPR_INT || (X).type == RSHELL_EXPR_FLOAT)

#ifdef __cplusplus
extern "C"
{
#endif

  RSHELL_API expr_t *rshell_expr_new (int);
  RSHELL_API expr_t *rshell_expr_fromExpr (expr_t);
  RSHELL_API void rshell_expr_free (expr_t *);
  RSHELL_API char *rshell_expr_toStr (expr_t *);
  RSHELL_API bool rshell_expr_toBool (expr_t *);
  RSHELL_API expr_t *rshell_expr_copy (expr_t *);
  RSHELL_API void
  rshell_expr_addToArgPass (expr_t *, expr_t *); /* only copies .v and .type */

#ifdef __cplusplus
}
#endif
