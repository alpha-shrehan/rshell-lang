#include "mem.h"

/* DONT USE GC NOW */
#define USE_GC 0

RSHELL_API void *
rshell_malloc (size_t size)
{
#if USE_GC == 1
  void *p = GC_MALLOC (size);
#else
  void *p = malloc (size);
#endif
  assert (!!p);

  return p;
}

RSHELL_API void *
rshell_realloc (void *old_ptr, size_t size)
{
  if (!size)
    {
      RSHELL_Free (old_ptr);
      return NULL;
    }
#if USE_GC == 1
  void *n = GC_REALLOC (old_ptr, size);
#else
  void *n = realloc (old_ptr, size);
#endif

  return n;
}

RSHELL_API void
rshell_free (void *ptr)
{
  if (!ptr)
    return;

#if USE_GC == 1
  GC_FREE (ptr);
#else
  free (ptr);
#endif
}

RSHELL_API char *
rshell_strdup (char *s)
{
  if (s == NULL)
    s = "";

  size_t d = strlen (s);
  char *m = RSHELL_Malloc ((d + 1) * sizeof (char));
  // sprintf (m, s);
  strncpy (m, s, d);
  m[d] = '\0';

  return m;
}