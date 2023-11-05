#include "../rsshell.h"
#define TEST(X) test##X ()

expr_t *
fun_test (mod_t *m, pret_t *p)
{
  printf ("Inside function\n");

  expr_t *e = rshell_expr_new (RSHELL_EXPR_INT);
  e->v.e_int.val = 0;

  return e;
}

void
test1 ()
{
  char *fr = rshell_file_read ("../../tests/test.shell");
  if (*fr == '\0')
    return;

  size_t sps = 0;
  char **spl = rshell_str_split_delim (fr, &sps, ';');
  stmt_t *sar = RSHELL_Malloc (sps * sizeof (stmt_t));

  for (size_t i = 0; i < sps; i++)
    {
      // printf ("[%s]\n", spl[i]);
      stmt_t c = rshell_ast_gen (spl[i]);

      // if (c.type != -1)
      //   rshell_ast_print_stmt (c);
      // else
      //   printf ("[stmt -1]\n");
      sar[i] = c;
    }

  rshell_ast_preprocess_tree (&sar, &sps);

  // for (size_t i = 0; i < sps; i++)
  //   {
  //     rshell_ast_print_stmt (sar[i]);
  //   }

  mod_t *m = rshell_parser_mod_new (sar, sps, NULL);

  rshell_fun_addNativeFunctions (m, -1);

  // while (1)
  rshell_parser_parse_tree (m);

  // rshell_vtable_dbgprint (m->vt);
}

void
test2 ()
{
  rstrie_t *t = rshell_trie_new (true);
  rshell_trie_add (t, "hello", "helloval");
  rshell_trie_add (t, "hell", "hellval");
  rshell_trie_add (t, "test", "testval");

  printf ("%s\n", (char *)rshell_trie_get (t, "hell"));
  printf ("%s\n", (char *)rshell_trie_get (t, "hello"));
  printf ("%s\n", (char *)rshell_trie_get (t, "test"));
}

cmd_ret_t
echo (char **args, int argc, fs_t *fs)
{
  cmd_ret_t c;
  c.code = 0;

  for (size_t i = 0; i < argc; i++)
    {
      char *_curr_arg = NULL;
      size_t d = strlen (args[i]), ca_count = 0;

      for (size_t j = 0; j < d; j++)
        {
          char p = args[i][j];

          if (p == '%')
            {
              _curr_arg
                  = RSHELL_Realloc (_curr_arg, (ca_count + 2) * sizeof (char));
              _curr_arg[ca_count++] = '%';
              _curr_arg[ca_count++] = '%';
            }
          else
            {
              _curr_arg
                  = RSHELL_Realloc (_curr_arg, (ca_count + 1) * sizeof (char));
              _curr_arg[ca_count++] = p;
            }
        }

      _curr_arg = RSHELL_Realloc (_curr_arg, (ca_count + 1) * sizeof (char));
      _curr_arg[ca_count++] = '\0';

      if (fs->use_standard)
        {
          if (i == argc - 1)
            /* c.code +=  */ fprintf (fs->_stdout, "%s\n", _curr_arg);
          else
            /* c.code +=  */ fprintf (fs->_stdout, "%s ", _curr_arg);
        }
      else
        {
          if (fs->buf)
            {
              fs->buf = RSHELL_Realloc (
                  fs->buf,
                  (strlen (fs->buf) + strlen (_curr_arg) + 2) * sizeof (char));

              if (i == argc - 1)
                /* c.code +=  */ sprintf (fs->buf, "%s%s\n", fs->buf,
                                          _curr_arg);
              else
                /* c.code +=  */ sprintf (fs->buf, "%s%s ", fs->buf,
                                          _curr_arg);
            }
          else
            {
              fs->buf
                  = RSHELL_Malloc ((strlen (_curr_arg) + 2) * sizeof (char));

              if (i == argc - 1)
                /* c.code +=  */ sprintf (fs->buf, "%s\n", _curr_arg);
              else
                /* c.code +=  */ sprintf (fs->buf, "%s ", _curr_arg);
            }
        }

      RSHELL_Free (_curr_arg);
    }

  return c;
}

cmd_ret_t
grch (char **args, int arg_count, fs_t *fs)
{
  cmd_ret_t c;

  assert (arg_count < 2);

  if (arg_count)
    printf ("%s", *args);

  c.code = _getch ();

  return c;
}

int
main (int argc, char const *argv[])
{
  rshell_rst_exprStackInit ();
  rshell_cmd_init ();
  rshell_class_init ();
  rshell_fun_init ();
  rshell_seq_init ();
  rshell_cobj_init ();

  rshell_cmd_add (rshell_cmd_new ("echo", echo));
  rshell_cmd_add (rshell_cmd_new ("grch", grch));

  TEST (1);

  return printf ("Program Ended.\n") && 0;
}