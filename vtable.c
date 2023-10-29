#include "vtable.h"
#include "expr.h"
#include "rstack.h"

static size_t
hash (char *str)
{
  size_t hash = 5381;
  int c;

  while ((c = *str++))
    hash = ((hash << 5) + hash) + c;

  return hash % RSHELL_VTABLE_SIZE;
}

RSHELL_API vtable_t *
rshell_vtable_new (void)
{
  vtable_t *vt = RSHELL_Malloc (sizeof (vtable_t));

  vt->elems = RSHELL_Malloc (RSHELL_VTABLE_SIZE * sizeof (vtval_t *));
  vt->lens = RSHELL_Malloc (RSHELL_VTABLE_SIZE * sizeof (size_t));

  for (size_t i = 0; i < RSHELL_VTABLE_SIZE; i++)
    {
      vt->elems[i] = NULL;
      vt->lens[i] = 0;
    }

  return vt;
}

RSHELL_API void
rshell_vtable_free (vtable_t *vt)
{
  for (size_t i = 0; i < RSHELL_VTABLE_SIZE; i++)
    {
      RSHELL_Free (vt->elems[i]);
    }
  RSHELL_Free (vt->elems);
  RSHELL_Free (vt->lens);
  RSHELL_Free (vt);
}

RSHELL_API void
rshell_vtable_add (vtable_t *vt, char *n, void *v)
{
  size_t h = hash (n);

  vt->elems[h] = RSHELL_Malloc (++vt->lens[h] * sizeof (vtval_t));

  vt->elems[h][vt->lens[h] - 1].name = n;
  vt->elems[h][vt->lens[h] - 1].val = v;
}

RSHELL_API void
rshell_vtable_add_withMeta (vtable_t *vt, char *n, void *v,
                            struct _rshell_vtable_add_meta_stru m)
{
  if (m.var_exists)
    {
      if (m.ve_idx == -1)
        m.ve_idx = hash (n);

      for (size_t i = 0; i < vt->lens[m.ve_idx]; i++)
        {
          if (!strcmp (vt->elems[m.ve_idx][i].name, n))
            {
              RSHELL_Free (n);
              vt->elems[m.ve_idx][i].val = v;
              break;
            }
        }
    }
  else
    {
      size_t h = hash (n);

      vt->elems[h] = RSHELL_Malloc (++vt->lens[h] * sizeof (vtval_t));

      vt->elems[h][vt->lens[h] - 1].name = n;
      vt->elems[h][vt->lens[h] - 1].val = v;
    }
}

RSHELL_API void
rshell_vtable_remove (vtable_t *vt, char *n)
{
  size_t h = hash (n);
  bool shft = false;

  for (size_t i = 0; i < vt->lens[h]; i++)
    {
      if (shft)
        vt->elems[h][i - 1] = vt->elems[h][i];

      if (!strcmp (vt->elems[h][i].name, n))
        {
          RSHELL_Free (vt->elems[h][i].name);
          rshell_rst_esModifyObjCount (vt->elems[h][i].val, -1);
          shft = true;
        }
    }

  vt->lens[h]--;
}

RSHELL_API void *
rshell_vtable_get (vtable_t *vt, char *n)
{
  size_t h = hash (n);

  for (int i = vt->lens[h] - 1; i >= 0; i--)
    {
      if (!strcmp (vt->elems[h][i].name, n))
        return vt->elems[h][i].val;
    }

  return NULL;
}

RSHELL_API void
rshell_vtable_dbgprint (vtable_t *vt)
{
  for (size_t i = 0; i < RSHELL_VTABLE_SIZE; i++)
    {
      for (size_t j = 0; j < vt->lens[i]; j++)
        {
          printf ("%s\n", vt->elems[i][j].name);
        }
    }
}

RSHELL_API vtable_t *
rshell_vtable_copy_deep (vtable_t *vt)
{
  vtable_t *v = rshell_vtable_new ();

  for (size_t i = 0; i < RSHELL_VTABLE_SIZE; i++)
    {
      if (!vt->lens[i])
        continue;

      v->lens[i] = vt->lens[i];
      v->elems[i] = RSHELL_Malloc (v->lens[i] * sizeof (vtval_t));

      for (size_t j = 0; j < v->lens[i]; j++)
        {
          expr_t *e = rshell_expr_copy ((expr_t *)vt->elems[i][j].val);

          v->elems[i][j]
              = (vtval_t){ .name = rshell_strdup (vt->elems[i][j].name),
                           .val = (void *)rshell_rst_esAppend (e) };
        }
    }

  return v;
}