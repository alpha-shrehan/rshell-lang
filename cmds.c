#include "cmds.h"

cmd_t **cmd_stack; /* consider using tries: TODO */
size_t cs_count;

void _cmd_s_realloc (void);

RSHELL_API fs_t *
rshell_cmd_fs_new (bool u)
{
  fs_t *t = RSHELL_Malloc (sizeof (fs_t));
  t->buf = NULL;
  t->buf_err = NULL;
  t->use_standard = u;
  t->_stdout = stdout;
  t->_stdin = stdin;
  t->_stderr = stderr;

  return t;
}

RSHELL_API void
rshell_cmd_fs_printf (fs_t *fs, char *data, ...)
{
  va_list vl;
  va_start (vl, data);

  char buf[1024];
  int d = vsprintf (buf, data, vl);

  if (!fs->buf)
    fs->buf = RSHELL_Strdup (buf);
  else
    {
      fs->buf = RSHELL_Realloc (fs->buf,
                                (strlen (fs->buf) + d + 1) * sizeof (char));
      sprintf (fs->buf, "%s%s", fs->buf, buf);
    }

  va_end (vl);
}

RSHELL_API void
rshell_cmd_fs_errprintf (fs_t *fs, char *data, ...)
{
  va_list vl;
  va_start (vl, data);

  char buf[1024];
  int d = vsprintf (buf, data, vl);

  if (!fs->buf_err)
    fs->buf_err = RSHELL_Strdup (buf);
  else
    {
      fs->buf_err = RSHELL_Realloc (fs->buf_err, (strlen (fs->buf_err) + d + 1)
                                                     * sizeof (char));
      sprintf (fs->buf_err, "%s%s", fs->buf_err, buf);
    }

  va_end (vl);
}

RSHELL_API void
rshell_cmd_init (void)
{
  cmd_stack = NULL;
  cs_count = 0;
}

void
_cmd_s_realloc (void)
{
  cmd_stack = RSHELL_Realloc (cmd_stack, (cs_count + 1) * sizeof (cmd_t *));
}

RSHELL_API cmd_t *
rshell_cmd_new (char *n, cmd_ret_t (*f) (char **, int, fs_t *))
{
  cmd_t *c = RSHELL_Malloc (sizeof (cmd_t));
  c->id = -1;
  c->name = n;
  c->routine = f;

  return c;
}

RSHELL_API cmd_t *
rshell_cmd_add (cmd_t *c)
{
  _cmd_s_realloc ();
  cmd_stack[cs_count] = c;
  c->id = cs_count;

  cs_count++;
  return c;
}

RSHELL_API cmd_t *
rshell_cmd_get (char *name)
{
  for (size_t i = 0; i < cs_count; i++)
    {
      if (!strcmp (cmd_stack[i]->name, name))
        return cmd_stack[i];
    }

  return NULL;
}

RSHELL_API void
rshell_cmd_fs_free (fs_t *fs)
{
  RSHELL_Free (fs->buf);
  RSHELL_Free (fs->buf_err);
  RSHELL_Free (fs);
}