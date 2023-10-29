#pragma once

#include "header.h"

struct _rshell_mod_stru;

struct _rshell_class_stru
{
  char *name;
  struct _rshell_mod_stru *mod;

  struct
  {
    int obj_count;
    int id;
  } meta;
};

typedef struct _rshell_class_stru class_t;

struct _rshell_cobj_stru
{
  class_t *ref;
  struct _rshell_mod_stru *mod;

  struct
  {
    int obj_count;
    int id;
  } meta;
};

typedef struct _rshell_cobj_stru cobj_t;

#define CLASS_CONSTRUCTOR_NAME "init"

#ifdef __cplusplus
extern "C"
{
#endif

  RSHELL_API void rshell_class_init (void);
  RSHELL_API class_t *rshell_class_new (char *, struct _rshell_mod_stru *);
  RSHELL_API void rshell_class_append (class_t *);
  RSHELL_API class_t ***rshell_class_getStack (void);
  RSHELL_API size_t *rshell_class_getStackCount (void);
  RSHELL_API class_t *rshell_class_get (int);
  RSHELL_API void rshell_class_addToObjCount (class_t *, int);
  RSHELL_API void rshell_class_free (class_t *);

  RSHELL_API void rshell_cobj_init (void);
  RSHELL_API cobj_t *rshell_cobj_new (class_t *, struct _rshell_mod_stru *);
  RSHELL_API void rshell_cobj_append (cobj_t *);
  RSHELL_API cobj_t ***rshell_cobj_getStack (void);
  RSHELL_API size_t *rshell_cobj_getStackCount (void);
  RSHELL_API cobj_t *rshell_cobj_get (int);
  RSHELL_API void rshell_cobj_addToObjCount (cobj_t *, int);
  RSHELL_API void rshell_cobj_free (cobj_t *);

#ifdef __cplusplus
}
#endif
