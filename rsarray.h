#include "header.h"

struct _rshell_expr_stru;
struct _rshell_seq_stru
{
  struct _rshell_expr_stru **vals;
  size_t size;

  int id;
  int obj_count;
};

typedef struct _rshell_seq_stru array_t;

#ifdef __cplusplus
extern "C"
{
#endif

  RSHELL_API void rshell_seq_init (void);
  RSHELL_API array_t *rshell_seq_new (void);
  RSHELL_API void rshell_seq_free (array_t *);
  RSHELL_API array_t *rshell_seq_append (array_t *);
  RSHELL_API array_t *rshell_seq_get (int);
  RSHELL_API array_t ***rshell_seq_getStack (void);
  RSHELL_API size_t *rshell_seq_getStackCount (void);
  RSHELL_API void rshell_seq_addToObjCount (array_t *, int);

  /* array manipulation methods */
  RSHELL_API int rshell_seq_add (array_t *, struct _rshell_expr_stru *);
  RSHELL_API int rshell_seq_insert (array_t *, struct _rshell_expr_stru *,
                                    int);

#ifdef __cplusplus
}
#endif
