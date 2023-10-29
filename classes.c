#include "classes.h"
#include "parser.h"

class_t **class_stack;
size_t class_stack_count;
int *cs_bpt;
size_t csbpt_count;

cobj_t **cobj_stack;
size_t cobj_stack_count;
int *cobj_bpt;
size_t cobjbpt_count;

void _rs_cs_realloc (void);

void _rs_csbpt_realloc (void);
void _rs_csbpt_append (int);
int _rs_csbpt_pop ();

void _rs_cobj_realloc (void);
void _rs_cobjbpt_realloc (void);
void _rs_cobjbpt_append (int);
int _rs_cobjbpt_pop ();

RSHELL_API void
rshell_class_init (void)
{
  class_stack = NULL;
  class_stack_count = 0;

  cs_bpt = NULL;
  csbpt_count = 0;
}

RSHELL_API class_t *
rshell_class_new (char *n, struct _rshell_mod_stru *m)
{
  class_t *t = RSHELL_Malloc (sizeof (class_t));
  t->name = RSHELL_Strdup (n);
  t->mod = m;
  t->meta.id = -1;
  t->meta.obj_count = 0;

  return t;
}

void
_rs_cs_realloc (void)
{
  class_stack = RSHELL_Realloc (class_stack,
                                (class_stack_count + 1) * sizeof (class_t *));
}

RSHELL_API void
rshell_class_append (class_t *t)
{
  if (csbpt_count)
    {
      int p = _rs_csbpt_pop ();
      t->meta.id = p;
      class_stack[t->meta.id] = t;
    }
  else
    {
      _rs_cs_realloc ();
      t->meta.id = class_stack_count;
      class_stack[t->meta.id] = t;
      class_stack_count++;
    }
}

RSHELL_API class_t ***
rshell_class_getStack (void)
{
  return &class_stack;
}

RSHELL_API size_t *
rshell_class_getStackCount (void)
{
  return &class_stack_count;
}

RSHELL_API class_t *
rshell_class_get (int i)
{
  if (i < 0 || i >= class_stack_count)
    return NULL;
  return class_stack[i];
}

void
_rs_csbpt_realloc (void)
{
  cs_bpt = RSHELL_Realloc (cs_bpt, (csbpt_count + 1) * sizeof (int));
}

void
_rs_csbpt_append (int i)
{
  _rs_csbpt_realloc ();
  cs_bpt[csbpt_count++] = i;
}

int
_rs_csbpt_pop ()
{
  if (!csbpt_count)
    return -1;

  int p = cs_bpt[csbpt_count - 1];
  cs_bpt = RSHELL_Realloc (cs_bpt, (--csbpt_count) * sizeof (int));

  return p;
}

RSHELL_API void
rshell_class_addToObjCount (class_t *c, int i)
{
  c->meta.obj_count += i;

  if (c->meta.obj_count < 1)
    {
      _rs_csbpt_append (c->meta.id);
      rshell_class_free (c);
    }
}

RSHELL_API void
rshell_class_free (class_t *c)
{
  RSHELL_Free (c->name);
  rshell_parser_mod_free (c->mod);
  RSHELL_Free (c);
}

RSHELL_API void
rshell_cobj_init (void)
{
  cobj_stack = NULL;
  cobj_stack_count = 0;
  cobj_bpt = NULL;
  cobjbpt_count = 0;
}

void
_rs_cobj_realloc (void)
{
  cobj_stack = RSHELL_Realloc (cobj_stack,
                               (cobj_stack_count + 1) * sizeof (cobj_t *));
}

void
_rs_cobjbpt_realloc (void)
{
  cobj_bpt = RSHELL_Realloc (cobj_bpt, (cobjbpt_count + 1) * sizeof (int));
}

void
_rs_cobjbpt_append (int i)
{
  _rs_cobjbpt_realloc ();
  cobj_bpt[cobjbpt_count++] = i;
}

int
_rs_cobjbpt_pop ()
{
  if (!cobjbpt_count)
    return -1;

  int p = cobj_bpt[cobjbpt_count - 1];
  cobj_bpt = RSHELL_Realloc (cobj_bpt, (--cobjbpt_count) * sizeof (int));

  return p;
}

RSHELL_API cobj_t *
rshell_cobj_new (class_t *ref, struct _rshell_mod_stru *md)
{
  cobj_t *t = RSHELL_Malloc (sizeof (cobj_t));
  t->ref = ref;
  t->mod = md;
  t->meta.id = -1;
  t->meta.obj_count = 0;

  return t;
}

RSHELL_API void
rshell_cobj_append (cobj_t *obj)
{
  if (cobjbpt_count)
    {
      int p = _rs_cobjbpt_pop ();
      obj->meta.id = p;
      cobj_stack[obj->meta.id] = obj;
    }
  else
    {
      _rs_cobj_realloc ();
      obj->meta.id = cobj_stack_count;
      cobj_stack[obj->meta.id] = obj;
      cobj_stack_count++;
    }
}

RSHELL_API cobj_t ***
rshell_cobj_getStack (void)
{
  return &cobj_stack;
}

RSHELL_API size_t *
rshell_cobj_getStackCount (void)
{
  return &cobj_stack_count;
}

RSHELL_API cobj_t *
rshell_cobj_get (int i)
{
  if (i < 0 || i >= cobj_stack_count)
    return NULL;
  return cobj_stack[i];
}

RSHELL_API void
rshell_cobj_addToObjCount (cobj_t *obj, int i)
{
  obj->meta.obj_count += i;

  if (obj->meta.obj_count < 1)
    {
      _rs_cobjbpt_append (obj->meta.id);
      rshell_cobj_free (obj);
    }
}

RSHELL_API void
rshell_cobj_free (cobj_t *obj)
{
  rshell_parser_mod_free (obj->mod);
  RSHELL_Free (obj);
}