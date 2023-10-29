#pragma once

#include "header.h"

#define RSHELL_VTABLE_SIZE 50000

struct _rshell_vt_val_stru
{
  char *name;
  void *val;
};

struct _rshell_vtable_stru
{
  struct _rshell_vt_val_stru **elems;
  size_t *lens;
};

struct _rshell_vtable_add_meta_stru
{
  bool var_exists;
  int ve_idx;
};

typedef struct _rshell_vt_val_stru vtval_t;
typedef struct _rshell_vtable_stru vtable_t;

#ifdef __cplusplus
extern "C"
{
#endif

  RSHELL_API vtable_t *rshell_vtable_new (void);
  RSHELL_API vtable_t *rshell_vtable_copy_deep (vtable_t *);
  RSHELL_API void rshell_vtable_add (vtable_t *, char *, void *);
  RSHELL_API void
  rshell_vtable_add_withMeta (vtable_t *, char *, void *,
                              struct _rshell_vtable_add_meta_stru);
  RSHELL_API void rshell_vtable_remove (vtable_t *, char *);
  RSHELL_API void *rshell_vtable_get (vtable_t *, char *);
  RSHELL_API void rshell_vtable_free (vtable_t *);
  RSHELL_API void rshell_vtable_dbgprint (vtable_t *);

#ifdef __cplusplus
}
#endif
