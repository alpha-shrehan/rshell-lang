#include "nativeroutines.h"
#include "parser.h"

expr_t *
rshell_native_routine_len (mod_t *mod, pret_t *pret)
{
  expr_t *itr = rshell_vtable_get (mod->vt, "iterable");
  expr_t *res = rshell_expr_new (RSHELL_EXPR_INT);
  res->v.e_int.val = 0;
  int rl = 0;

  switch (itr->type)
    {
    case RSHELL_EXPR_SEQUENCE:
      {
        rl = rshell_seq_get (itr->v.e_seq.id)->size;
      }
      break;

    default:
      break;
    }

  res->v.e_int.val = rl;
  return res;
}