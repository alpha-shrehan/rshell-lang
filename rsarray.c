#include "rsarray.h"
#include "expr.h"
#include "rstack.h"

array_t **array_stack;
size_t ar_stack_count;

int *ar_bpt_stack;
size_t arbpt_count;

void _rs_as_realloc (void);
void _rs_as_append (array_t *);
void _rs_asbpt_realloc ();
void _rs_asbpt_append (int);
int _rs_asbpt_pop ();

RSHELL_API void
rshell_seq_init (void)
{
  array_stack = NULL;
  ar_stack_count = 0;
  ar_bpt_stack = NULL;
  arbpt_count = 0;
}

RSHELL_API array_t *
rshell_seq_new (void)
{
  array_t *t = RSHELL_Malloc (sizeof (array_t));
  t->size = 0;
  t->vals = NULL;
  t->id = -1;
  t->obj_count = 0;

  return t;
}

RSHELL_API void
rshell_seq_free (array_t *t)
{
  // here;
  for (size_t i = 0; i < t->size; i++)
    rshell_rst_esModifyObjCount (t->vals[i], -1);

  RSHELL_Free (t->vals);
  RSHELL_Free (t);
}

void
_rs_as_realloc (void)
{
  array_stack = RSHELL_Realloc (array_stack,
                                (ar_stack_count + 1) * sizeof (array_t *));
}

void
_rs_as_append (array_t *t)
{
  if (arbpt_count)
    {
      int p = _rs_asbpt_pop ();
      t->id = p;
      array_stack[p] = t;
    }
  else
    {
      _rs_as_realloc ();
      t->id = ar_stack_count;
      array_stack[t->id] = t;
      ar_stack_count++;
    }
}

RSHELL_API array_t *
rshell_seq_append (array_t *t)
{
  _rs_as_append (t);

  return t;
}

RSHELL_API array_t *
rshell_seq_get (int id)
{
  if (id < 0 || id > ar_stack_count)
    return NULL;
  return array_stack[id];
}

RSHELL_API array_t ***
rshell_seq_getStack (void)
{
  return &array_stack;
}

RSHELL_API size_t *
rshell_seq_getStackCount (void)
{
  return &ar_stack_count;
}

RSHELL_API int
rshell_seq_add (array_t *t, struct _rshell_expr_stru *v)
{
  if (!t)
    return -1;

  t->vals = RSHELL_Realloc (t->vals, (t->size + 1) * sizeof (expr_t *));
  t->vals[t->size++] = rshell_rst_esAppend (v);
  rshell_rst_esModifyObjCount (v, 1);

  return 0; /* ok */
}

RSHELL_API int
rshell_seq_insert (array_t *t, struct _rshell_expr_stru *v, int i)
{
  if (!t)
    return -1;

  i %= t->size;
  t->vals[i] = v;
  rshell_rst_esModifyObjCount (v, 1);

  return 0;
}

void
_rs_asbpt_realloc ()
{
  ar_bpt_stack
      = RSHELL_Realloc (ar_bpt_stack, (arbpt_count + 1) * sizeof (int));
}

void
_rs_asbpt_append (int i)
{
  _rs_asbpt_realloc ();
  ar_bpt_stack[arbpt_count++] = i;
}

int
_rs_asbpt_pop ()
{
  if (!arbpt_count)
    return -1;

  int p = ar_bpt_stack[arbpt_count - 1];
  ar_bpt_stack = RSHELL_Realloc (ar_bpt_stack, (--arbpt_count) * sizeof (int));

  return p;
}

RSHELL_API void
rshell_seq_addToObjCount (array_t *t, int i)
{
  t->obj_count += i;

  if (t->obj_count < 1)
    {
      _rs_asbpt_append (t->id);
      rshell_seq_free (t);
    }
}