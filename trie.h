#pragma once

#include "header.h"

struct _rshell_trie_stru
{
  char *key;
  void *val;
  bool marked;
  struct _rshell_trie_stru **children;
  size_t count;

  char **allkeys;
  size_t aklen;

  bool istop;
};

typedef struct _rshell_trie_stru rstrie_t;

#ifdef __cplusplus
extern "C"
{
#endif

  RSHELL_API rstrie_t *rshell_trie_new (bool is_top);
  RSHELL_API void rshell_trie_add (rstrie_t *, char *, void *);
  RSHELL_API void *rshell_trie_get (rstrie_t *, char *);
  RSHELL_API bool rshell_trie_key_exists (rstrie_t *, char *);
  RSHELL_API void rshell_trie_free (rstrie_t *);

#ifdef __cplusplus
}
#endif
