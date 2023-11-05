#include "rfunction.h"
#include "nativeroutines.h"
#include "parser.h"

/* TODO: implement function stack */
fun_t **fun_stack;
size_t fn_count;

void _fn_realloc (void);

RSHELL_API void
rshell_fun_init (void)
{
  fun_stack = NULL;
  fn_count = 0;
}

RSHELL_API fun_t *
rshell_fun_new (char *_Name, char **_Args, size_t _ArgCount, bool _IsNative,
                struct _rshell_mod_stru *_Parent)
{
  fun_t *f = RSHELL_Malloc (sizeof (fun_t));

  f->name = _Name;
  f->args = _Args;
  f->arg_count = _ArgCount;
  f->isnative = _IsNative;
  f->id = -1;
  f->parent = _Parent;

  return f;
}

void
_fn_realloc (void)
{
  fun_stack = RSHELL_Realloc (fun_stack, (fn_count + 1) * sizeof (fun_t *));
}

RSHELL_API fun_t *
rshell_fun_add (fun_t *f)
{
  _fn_realloc ();
  fun_stack[fn_count] = f;
  f->id = fn_count;

  fn_count++;
  return f;
}

RSHELL_API fun_t *
rshell_fun_get_byID (int id)
{
  if (id < 0 || id > fn_count)
    return NULL;

  return fun_stack[id];
}

RSHELL_API fun_t *
rshell_fun_get_byName (char *name)
{
  for (size_t i = 0; i < fn_count; i++)
    {
      if (!strcmp (fun_stack[i]->name, name))
        return fun_stack[i];
    }

  return NULL;
}

int
in_str_array (char **a, char *v, int l)
{
  for (size_t i = 0; i < l; i++)
    if (!strcmp (a[i], v))
      return i;
  return -1;
}

RSHELL_API int
rshell_fun_addNativeFunctions (mod_t *m, int _fc, ...)
{
  if (_fc == -1)
    {
      /* add all */
      /* len */
      {
        char **arglist = RSHELL_Malloc (sizeof (char *));
        *arglist = RSHELL_Strdup ("iterable");
        fun_t *f = rshell_fun_new ("len", arglist, 1, 1, NULL);
        f->v.f_native = rshell_native_routine_len;

        rshell_fun_add (f);
        rshell_parser_mod_var_add (
            m, f->name,
            rshell_expr_fromExpr ((expr_t){ .type = RSHELL_EXPR_FUNCTION,
                                            .v.e_func.id = f->id }));
      }
    }
  else
    {
      assert (0 && "Pending. Consider setting _fc = -1");
      va_list va;
      va_start (va, _fc);

      char *flist[_fc];
      for (size_t i = 0; i < _fc; i++)
        flist[i] = va_arg (va, char *);

      va_end (va);
    }
}