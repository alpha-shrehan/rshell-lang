#include "trie.h"

RSHELL_API rstrie_t *
rshell_trie_new (bool is_top)
{
  rstrie_t *t = RSHELL_Malloc (sizeof (rstrie_t));
  t->children = NULL;
  t->count = 0;
  t->key = NULL;
  t->marked = false;
  t->val = NULL;
  t->allkeys = NULL;
  t->aklen = 0;
  t->istop = is_top;

  return t;
}

RSHELL_API void
rshell_trie_add (rstrie_t *t, char *k, void *val)
{
  if (k && *k == '\0')
    {
      t->marked = true;
      t->val = val;
      return;
    }

  if (t->istop && !rshell_trie_key_exists (t, k))
    {
      t->allkeys
          = RSHELL_Realloc (t->allkeys, (t->aklen + 1) * sizeof (char *));

      t->allkeys[t->aklen++] = RSHELL_Strdup (k);
    }

  if (t->key)
    {
      char *d = t->key ? strstr (t->key, (char *)(char[]){ *k, '\0' }) : NULL;

      if (!d)
        {
          size_t tl = t->count;
          t->key = RSHELL_Realloc (t->key, (tl + 2) * sizeof (char));
          t->key[tl] = *k;
          t->key[tl + 1] = '\0';

          t->children
              = RSHELL_Realloc (t->children, (tl + 1) * sizeof (rstrie_t *));
          t->children[tl] = rshell_trie_new (false);
          t->count++;

          rshell_trie_add (t->children[tl], k + 1, val);
        }
      else
        {
          rshell_trie_add (t->children[d - t->key], k + 1, val);
        }
    }
  else
    {
      t->key = RSHELL_Malloc (2 * sizeof (char));
      t->key[0] = *k;
      t->key[1] = '\0';

      t->children = RSHELL_Malloc (sizeof (rstrie_t *));
      t->children[0] = rshell_trie_new (false);
      t->count++;

      rshell_trie_add (t->children[0], k + 1, val);
    }
}

RSHELL_API void *
rshell_trie_get (rstrie_t *t, char *k)
{
  if (k && *k == '\0')
    {
      assert (t->marked);
      return t->val;
    }

  char *d = t->key ? strstr (t->key, (char *)(char[]){ *k, '\0' }) : NULL;
  assert (d);

  return rshell_trie_get (t->children[d - t->key], k + 1);
}

RSHELL_API bool
rshell_trie_key_exists (rstrie_t *t, char *k)
{
  if (k && *k == '\0')
    {
      if (t->marked)
        return true;
      return false;
    }

  char *d = t->key ? strstr (t->key, (char *)(char[]){ *k, '\0' }) : NULL;
  if (!d)
    return false;

  return rshell_trie_key_exists (t->children[d - t->key], k + 1);
}

RSHELL_API void
rshell_trie_free (rstrie_t *t)
{
  RSHELL_Free (t->key);

  for (size_t i = 0; i < t->aklen; i++)
    RSHELL_Free (t->allkeys[i]);
  RSHELL_Free (t->allkeys);

  for (size_t i = 0; i < t->count; i++)
    rshell_trie_free (t->children[i]);

  RSHELL_Free (t->children);
  RSHELL_Free (t);
}