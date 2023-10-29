#include "expr.h"
#include "classes.h"
#include "rfunction.h"
#include "rsarray.h"

RSHELL_API expr_t *
rshell_expr_new (int type)
{
  expr_t *t = RSHELL_Malloc (sizeof (expr_t));

  t->type = type;
  t->meta.id = -1;
  t->meta.isallocated = false;
  t->meta.objcount = 0;

  return t;
}

RSHELL_API expr_t *
rshell_expr_fromExpr (expr_t e)
{
  expr_t *t = rshell_expr_new (e.type);
  t->v = e.v;

  return t;
}

RSHELL_API void
rshell_expr_free (expr_t *e)
{
  assert (!!e);

  switch (e->type)
    {
    case RSHELL_EXPR_STRING:
      RSHELL_Free (e->v.e_string.val);
      break;
    case RSHELL_EXPR_NOTYPE:
      RSHELL_Free (e->v.e_notype.val);
      break;
    case RSHELL_EXPR_SEQUENCE:
      rshell_seq_addToObjCount (rshell_seq_get (e->v.e_seq.id), -1);
      break;

    default:
      break;
    }

  RSHELL_Free (e);
}

RSHELL_API char *
rshell_expr_toStr (expr_t *e)
{
  char *res = RSHELL_Malloc (512 * sizeof (char));

  switch (e->type)
    {
    case RSHELL_EXPR_FLOAT:
      sprintf (res, "%.3f", e->v.e_float.val);
      break;
    case RSHELL_EXPR_INT:
      sprintf (res, "%i", e->v.e_int.val);
      break;
    case RSHELL_EXPR_NOTYPE:
      sprintf (res, "%s", e->v.e_notype.val);
      break;
    case RSHELL_EXPR_STRING:
      sprintf (res, "%s", e->v.e_string.val);
      break;
    case RSHELL_EXPR_SEQUENCE:
      {
        int c_count = 0;
        array_t *t = rshell_seq_get (e->v.e_seq.id);
        assert (!!t);

        res[0] = '[';
        res[1] = '\0';

        for (size_t i = 0; i < t->size; i++)
          {
            char *r = rshell_expr_toStr (t->vals[i]);
            size_t d = strlen (r);

            if (c_count > 512)
              res = RSHELL_Realloc (res, (c_count + d + 2) * sizeof (char));

            if (i == t->size - 1)
              sprintf (res, "%s%s]", res, r);
            else
              sprintf (res, "%s%s,", res, r);

            c_count += d;
          }
      }
      break;
    case RSHELL_EXPR_FUNCTION:
      {
        sprintf (res, "<function '%s'>",
                 rshell_fun_get_byID (e->v.e_func.id)->name);
      }
      break;
    case RSHELL_EXPR_CLASS:
      {
        sprintf (res, "<class '%s'>",
                 rshell_class_get (e->v.e_class.id)->name);
      }
      break;
    case RSHELL_EXPR_COBJ:
      {
        sprintf (res, "<cobj '%s'>",
                 rshell_cobj_get (e->v.e_cobj.id)->ref->name);
      }
      break;

    default:
      sprintf (res, "<unknown_type '%d'>", e->type);
      break;
    }

  char *pres_res = res;
  res = RSHELL_Strdup (res);
  RSHELL_Free (pres_res);

  return res;
}

RSHELL_API bool
rshell_expr_toBool (expr_t *e)
{
  bool r = false;

  switch (e->type)
    {
    case RSHELL_EXPR_NOTYPE:
      r = !!strlen (e->v.e_notype.val);
      break;
    case RSHELL_EXPR_FLOAT:
      r = e->v.e_float.val != 0;
      break;
    case RSHELL_EXPR_INT:
      r = e->v.e_int.val != 0;
      break;
    case RSHELL_EXPR_STRING:
      r = !!strlen (e->v.e_string.val);
      break;
    case RSHELL_EXPR_SEQUENCE:
      {
        /* TODO */
      }
      break;
    case RSHELL_EXPR_FUNCTION:
      r = true;
      break;

    default:
      break;
    }

  return r;
}

RSHELL_API expr_t *
rshell_expr_copy (expr_t *e)
{
  expr_t *t = rshell_expr_new (e->type);

  switch (t->type)
    {
    case RSHELL_EXPR_FLOAT:
    case RSHELL_EXPR_INT:
    case RSHELL_EXPR_FUNCTION:
    case RSHELL_EXPR_CLASS:
    case RSHELL_EXPR_COBJ:
      t->v = e->v;
      break;
    case RSHELL_EXPR_SEQUENCE:
      {
        rshell_seq_addToObjCount (rshell_seq_get (e->v.e_seq.id), 1);
        t->v = e->v;
      }
      break;
    case RSHELL_EXPR_STRING:
      t->v.e_string.val = RSHELL_Strdup (e->v.e_string.val);
      break;
    case RSHELL_EXPR_NOTYPE:
      t->v.e_notype.val = RSHELL_Strdup (e->v.e_notype.val);
      break;

    default:
      printf ("%d\n", t->type);
      assert (0 && "Copy operation not implemented.");
      break;
    }

  return t;
}