#include "ast.h"
#include "rsshell.h"

bool is_rss_identifier (char *);

RSHELL_API stmt_t *
rshell_ast_stmt_new (int type)
{
  stmt_t *t = RSHELL_Malloc (sizeof (stmt_t));
  t->type = type;

  return t;
}

RSHELL_API stmt_t
rshell_ast_gen (char *data)
{
  size_t i = 0;
  size_t dl = strlen (data);
  stmt_t res;
  res.type = -1;
  res.meta.is_labelled = false;
  res.meta.label_name = NULL;

  while (i < dl)
    {
      char c = data[i];

      if (c == '$')
        {
          res.type = RSL_STMT_VAR;
          int nidx = i + 1;
          char *vn = NULL;

          assert ((isalpha (data[nidx]) || data[nidx] == '_')
                  && "Syntax Error.");

          for (size_t j = nidx; j < dl; j++)
            {
              char d = data[j];

              if (isalnum (d) || d == '_')
                ;
              else
                {
                  nidx = j;
                  break;
                }
            }

          if (nidx == i + 1)
            nidx = dl;

          int p = nidx - i - 1;

          vn = RSHELL_Malloc ((p + 1) * sizeof (char));

          strncpy (vn, data + i + 1, p);
          vn[p] = '\0';

          // dprintf ("[varname: %s]\n", vn);

          res.v.stmt_var.name = vn;
          i = nidx - 1;
          goto end;
        }
      else if (c == '=')
        {
          /* var decl */
          stmt_t res_pres = res;

          res.type = RSL_STMT_VAR_DECL;

          switch (res_pres.type)
            {
            case RSL_STMT_VAR:
              res.v.var_decl.name = res_pres.v.stmt_var.name;
              break;

            default:
              {
                char *dt = RSHELL_Strdup (data);
                dt[i] = '\0';

                char *_d = dt;
                dt = rshell_trim (dt);
                RSHELL_Free (_d);

                res.v.var_decl.name = dt;
              }
              break;
            }

          res.v.var_decl.val = RSHELL_Strdup (data + i + 1);

          goto lo;
        }
      else if (c == '(')
        {
          stmt_t res_pres = res;
          res.type = RSL_STMT_FUN_CALL;
          res.v.fun_call.arg_count = 0;
          res.v.fun_call.args = NULL;
          res.v.fun_call.callee = NULL;
          res.v.fun_call.eval_callee = false;

          if (res_pres.type == RSL_STMT_VAR)
            res.v.fun_call.callee = res_pres.v.stmt_var.name;
          else
            {
              res.v.fun_call.callee = RSHELL_Strdup (data);
              res.v.fun_call.callee[i] = '\0';
              res.v.fun_call.eval_callee = true;
            }

          // char *callee = RSHELL_Malloc ((i + 1) * sizeof (char));
          // strncpy (callee, data, i);
          // callee[i] = '\0';
          // dprintf ("[callee: %s]\n", callee);

          char *dd = RSHELL_Strdup (data + i + 1);
          size_t ddl = strlen (dd);

          int endbr = -1;
          int gb = 0;
          int in_str = 0;

          for (size_t j = 0; j < ddl; j++)
            {
              char d = dd[j];

              if (d == '\'' || d == '\"' || d == '`')
                {
                  if (j)
                    {
                      if (dd[j - 1] != '\\')
                        in_str = !in_str;
                      else
                        {
                          if (j > 1 && dd[j - 2] == '\\')
                            in_str = !in_str;
                        }
                    }
                  else
                    in_str = !in_str;
                }

              if (!in_str)
                {
                  if (d == ')' && !gb)
                    {
                      endbr = j;
                      break;
                    }

                  if (d == '(' || d == '{' || d == '[')
                    gb++;

                  if (d == ')' || d == '}' || d == ']')
                    gb--;
                }
            }

          dd[endbr] = '\0';

          // res.v.fun_call.callee = RSHELL_Strdup (callee);

          if (dd[0] != '\0')
            res.v.fun_call.args
                = rshell_str_split_delim (dd, &res.v.fun_call.arg_count, ',');

          i += (endbr + 1);
          RSHELL_Free (dd);

          goto end;
        }
      else if (c == '{')
        {
          if (res.type == -1)
            goto end;

          if (res.type == RSL_STMT_FUN_CALL)
            {
              /* function decl */

              stmt_t rpres = res;

              res.type = RSL_STMT_FUN_DECL;
              res.v.fun_decl.arg_count = rpres.v.fun_call.arg_count;
              res.v.fun_decl.args = rpres.v.fun_call.args;
              res.v.fun_decl.fname = rpres.v.fun_call.callee;

              int gb = 0;
              int body_end_idx = -1;
              int in_str = 0;

              for (size_t j = i + 1; j < dl; j++)
                {
                  char d = data[j];

                  if (d == '\'' || d == '\"' || d == '`')
                    {
                      if (j)
                        {
                          if (data[j - 1] != '\\')
                            in_str = !in_str;
                          else
                            {
                              if (j > 1 && data[j - 2] == '\\')
                                in_str = !in_str;
                            }
                        }
                      else
                        in_str = !in_str;
                    }

                  if (!in_str)
                    {
                      if (d == '}' && !gb)
                        {
                          body_end_idx = j;
                          break;
                        }

                      if (d == '(' || d == '{' || d == '[')
                        gb++;

                      if (d == ')' || d == '}' || d == ']')
                        gb--;
                    }
                }

              assert (body_end_idx != -1);

              char *bcpy = RSHELL_Strdup (data + i + 1);
              bcpy[body_end_idx - i - 1] = '\0';

              size_t bsps = 0;
              char **bspl = rshell_str_split_delim (
                  bcpy, &bsps, ';'); /* split function body by statements */

              res.v.fun_decl.body = RSHELL_Malloc (
                  (res.v.fun_decl.body_count = bsps) * sizeof (stmt_t));

              for (size_t j = 0; j < bsps; j++)
                {
                  res.v.fun_decl.body[j] = rshell_ast_gen (bspl[j]);
                }

              rshell_ast_preprocess_tree (&res.v.fun_decl.body,
                                          &res.v.fun_decl.body_count);

              for (size_t j = 0; j < bsps; j++)
                RSHELL_Free (bspl[j]);
              RSHELL_Free (bspl);
              RSHELL_Free (bcpy);

              // for (size_t j = 0; j < res.v.fun_decl.arg_count; j++)
              //   {
              //     printf ("arg: %s\n", res.v.fun_decl.args[j]);
              //   }

              i = body_end_idx;
              goto end;
            }
        }
      else if (c == '\'' || c == '\"')
        {
          int snext = -1;

          for (size_t j = i + 1; j < dl; j++)
            {
              char d = data[j];

              if (d == c && data[j - 1] != '\\')
                {
                  snext = j;
                  break;
                }
            }

          int p = snext - i - 1;
          char *sc = RSHELL_Malloc ((p + 1) * sizeof (char));
          strncpy (sc, data + i + 1, p);
          sc[p] = '\0';

          res.type = RSL_STMT_STRING;
          res.v.stmt_string.val = sc;
          i = snext;

          goto end;
        }
      else if (isalpha (c))
        {
          char *identifier = NULL;
          int id_end = -1;
          bool is_label = false;

          for (size_t j = i + 1; j < dl; j++)
            {
              char d = data[j];

              if (isalnum (d) || d == '_')
                ;
              else
                {
                  if (d == ':')
                    is_label = true;
                  id_end = j;
                  break;
                }
            }

          if (id_end == -1)
            id_end = dl;

          int p = id_end - i;
          identifier = RSHELL_Malloc ((p + 1) * sizeof (char));
          strncpy (identifier, data + i, p);
          identifier[p] = '\0';

          if (is_label)
            {
              res.meta.is_labelled = is_label;
              res.meta.label_name = identifier;

              i = id_end;
              goto end;
            }
          else
            {
              if (is_rss_identifier (identifier))
                {
                  /* statement */

                  if (!strcmp (identifier, "if"))
                    {
                      int block_st_idx = rget_next_chr (data, '{', i, 1, 1);
                      int p = block_st_idx - i - 2;

                      char *cnd = RSHELL_Malloc ((p + 1) * sizeof (char));
                      strncpy (cnd, data + i + 2, p);
                      cnd[p] = '\0';

                      int block_end_idx
                          = rget_next_chr (data, '}', block_st_idx + 1, 1, 1);
                      p = block_end_idx - block_st_idx - 1;

                      char *bdy = RSHELL_Malloc ((p + 1) * sizeof (char));
                      strncpy (bdy, data + block_st_idx + 1, p);
                      bdy[p] = '\0';

                      res.type = RSL_STMT_BLOCK_IF;
                      res.v.block_if.cond = cnd;
                      res.v.block_if.body = bdy;
                      res.v.block_if.elif_count = 0;
                      res.v.block_if.ast_ed = false;
                      res.v.block_if.body_ast = NULL;
                      res.v.block_if.body_else = NULL;
                      res.v.block_if.elif_conds = NULL;
                      res.v.block_if.elif_bodies = NULL;
                      res.v.block_if.body_ast_count = 0;
                      res.v.block_if.body_else_ast = NULL;
                      res.v.block_if.elif_bodies_ast = NULL;
                      res.v.block_if.body_else_ast_count = 0;
                      res.v.block_if.elif_body_ast_size_counts = NULL;

                      RSHELL_Free (identifier);
                      i = block_end_idx;
                      goto end;
                    }
                  else if (!strcmp (identifier, "elif"))
                    {
                      assert (res.type == RSL_STMT_BLOCK_IF);

                      int block_st_idx = rget_next_chr (data, '{', i, 1, 1);
                      int p = block_st_idx - i - 4;

                      char *cnd = RSHELL_Malloc ((p + 1) * sizeof (char));
                      strncpy (cnd, data + i + 4, p);
                      cnd[p] = '\0';

                      int block_end_idx
                          = rget_next_chr (data, '}', block_st_idx + 1, 1, 1);
                      p = block_end_idx - block_st_idx - 1;

                      char *bdy = RSHELL_Malloc ((p + 1) * sizeof (char));
                      strncpy (bdy, data + block_st_idx + 1, p);
                      bdy[p] = '\0';

                      res.v.block_if.elif_conds = RSHELL_Realloc (
                          res.v.block_if.elif_conds,
                          (res.v.block_if.elif_count + 1) * sizeof (char *));

                      res.v.block_if.elif_bodies = RSHELL_Realloc (
                          res.v.block_if.elif_bodies,
                          (res.v.block_if.elif_count + 1) * sizeof (char *));

                      res.v.block_if.elif_conds[res.v.block_if.elif_count]
                          = cnd;
                      res.v.block_if.elif_bodies[res.v.block_if.elif_count]
                          = bdy;
                      res.v.block_if.elif_count++;

                      RSHELL_Free (identifier);
                      i = block_end_idx;
                      goto end;
                    }
                  else if (!strcmp (identifier, "else"))
                    {
                      assert (res.type == RSL_STMT_BLOCK_IF);
                      int op_brk_idx = i + 4;

                      while (data[op_brk_idx] != '{')
                        op_brk_idx++;

                      int cl_brk_idx
                          = rget_next_chr (data, '}', op_brk_idx + 1, 1, 1);
                      int p = cl_brk_idx - op_brk_idx - 2;
                      char *body_else
                          = RSHELL_Malloc ((p + 1) * sizeof (char));

                      strncpy (body_else, data + op_brk_idx + 1, p);
                      body_else[p] = '\0';

                      res.v.block_if.body_else = body_else;

                      cl_brk_idx++;
                      while (data[cl_brk_idx] == ' '
                             || data[cl_brk_idx] == '\t'
                             || data[cl_brk_idx] == '\n')
                        cl_brk_idx++;

                      RSHELL_Free (identifier);
                      // assert (data[cl_brk_idx] != ';' && "Expected `;'.");
                      goto lo;
                    }
                  else if (!strcmp (identifier, "goto"))
                    {
                      res.type = RSL_STMT_GOTO;
                      res.v.stmt_goto.name = RSHELL_Strdup (data + i + 5);

                      RSHELL_Free (identifier);
                      goto lo;
                    }
                  else if (!strcmp (identifier, "use"))
                    {
                      res.type = RSL_STMT_USE;
                      res.v.stmt_use.name = RSHELL_Strdup (data + i + 4);

                      RSHELL_Free (identifier);
                      goto lo;
                    }
                  else if (!strcmp (identifier, "class"))
                    {
                      int bs_idx = rget_next_chr (data, '{', i + 5, 1, 1);
                      int p = bs_idx - i - 5;
                      char *name = RSHELL_Malloc ((p + 1) * sizeof (char));
                      strncpy (name, data + i + 5, p);
                      name[p] = '\0';

                      char *_name = name;
                      name = rshell_trim (name);
                      RSHELL_Free (_name);

                      int be_idx = rget_next_chr (data, '}', bs_idx + 1, 1, 1);
                      p = be_idx - bs_idx - 1;
                      char *body = RSHELL_Malloc ((p + 1) * sizeof (char));
                      strncpy (body, data + bs_idx + 1, p);
                      body[p] = '\0';

                      res.type = RSL_STMT_CLASS_DECL;
                      res.v.stmt_class_decl.name = RSHELL_Strdup (name);
                      res.v.stmt_class_decl.body_count = 0;
                      res.v.stmt_class_decl.body = NULL;

                      // printf ("%s\n", body);
                      size_t bsps = 0;
                      char **bspl = rshell_str_split_delim (body, &bsps, ';');
                      res.v.stmt_class_decl.body = RSHELL_Malloc (
                          (res.v.stmt_class_decl.body_count = bsps)
                          * sizeof (stmt_t));

                      for (size_t j = 0; j < bsps; j++)
                        {
                          res.v.stmt_class_decl.body[j]
                              = rshell_ast_gen (bspl[j]);
                        }

                      rshell_ast_preprocess_tree (
                          &res.v.stmt_class_decl.body,
                          &res.v.stmt_class_decl.body_count);

                      for (size_t j = 0; j < bsps; j++)
                        RSHELL_Free (bspl[j]);
                      RSHELL_Free (bspl);

                      i = be_idx;

                      RSHELL_Free (name);
                      RSHELL_Free (body);
                      RSHELL_Free (identifier);
                      goto end;
                    }
                }
              else
                {
                  /* command */
                  size_t il = strlen (identifier);

                  // int cmd_end_idx
                  //     = rget_next_chr (data, ';', i + il + 1, 1, 1);

                  if (i + il == dl)
                    {
                      res.type = RSL_STMT_COMMAND;
                      res.v.stmt_cmd.name = RSHELL_Strdup (identifier);
                      res.v.stmt_cmd.args = "";
                    }
                  else
                    {
                      int cmd_end_idx = dl;
                      int p = cmd_end_idx - i - il - 1;

                      char *cm = RSHELL_Malloc ((p + 1) * sizeof (char));
                      strncpy (cm, data + i + il + 1, p);
                      cm[p] = '\0';

                      res.type = RSL_STMT_COMMAND;
                      res.v.stmt_cmd.name = RSHELL_Strdup (identifier);
                      res.v.stmt_cmd.args = cm;
                    }

                  RSHELL_Free (identifier);
                  goto lo;
                }
            }
        }
      else if (c == '+')
        {
          if (i && data[i - 1] == '+')
            {
              /* decrement operator */
              stmt_t vpres = res;
              assert (vpres.type == RSL_STMT_VAR);

              res.type = RSL_STMT_INCREMENT;
              res.v.stmt_dec.name = RSHELL_Strdup (vpres.v.stmt_var.name);
            }
        }
      else if (c == '-')
        {
          if (i && data[i - 1] == '-')
            {
              /* decrement operator */
              stmt_t vpres = res;
              assert (vpres.type == RSL_STMT_VAR);

              res.type = RSL_STMT_DECREMENT;
              res.v.stmt_dec.name = RSHELL_Strdup (vpres.v.stmt_var.name);
            }
          else if (i < dl - 1 && data[i + 1] == '>')
            {
              /* -> operator */
              assert (res.type != -1);
              stmt_t *vpres = rshell_ast_stmt_new (0);
              *vpres = res;

              res.type = RSL_STMT_MEM_ACCESS;

              int word_ei = i + 2;
              while (isalnum (data[word_ei]) || data[word_ei] == '_')
                word_ei++;

              int p = word_ei - i - 2;
              char *attr_name = RSHELL_Malloc ((p + 1) * sizeof (char));

              strncpy (attr_name, data + i + 2, p);
              attr_name[p] = '\0';

              res.v.mem_access.attr = attr_name;
              res.v.mem_access.parent = vpres;

              i = word_ei - 1;
              goto end;
            }
        }
      else if (c == '#')
        {
          int nl_idx = rget_next_chr (data, '\n', i, 1, 1);

          char *cmt = NULL;
          if (nl_idx == dl)
            cmt = RSHELL_Strdup (data);
          else
            {
              int p = nl_idx - i - 1;
              cmt = RSHELL_Malloc ((p + 1) * sizeof (char));
              strncpy (cmt, data + i + 1, p);
              cmt[p] = '\0';
            }

          res.type = RSL_STMT_COMMENT;
          res.v.stmt_cmt.val = cmt;

          i = nl_idx;
        }

    end:
      i++;
    }

lo:;
  return res;
}

bool
is_rss_identifier (char *id)
{
  char *IDS[] = { "if", "else", "elif", "goto", "use", "class", NULL };

  int i = 0;
  while (IDS[i])
    {
      if (!strcmp (IDS[i], id))
        return true;
      i++;
    }

  return false;
}

RSHELL_API int
rget_next_chr (char *data, char tar, int st, bool case_str, bool case_brack)
{
  int ni = -1;
  int in_str = 0;
  int gb = 0;
  size_t dl = strlen (data);
  char last_str_tok = 0;

  int i;
  for (i = st; i < dl; i++)
    {
      char c = data[i];

      if (case_str)
        {
          if (c == '\'' || c == '\"' || c == '`')
            {
              // printf ("%d %c\n", last_str_tok, last_str_tok);
              if (last_str_tok)
                {
                  if (c == last_str_tok)
                    goto L1;
                }
              else
                {
                L1:
                  if (i)
                    {
                      if (data[i - 1] != '\\')
                        {
                          in_str = !in_str;
                          if (in_str)
                            last_str_tok = c;
                        }
                      else
                        {
                          if (i > 1 && data[i - 2] == '\\')
                            {
                              in_str = !in_str;
                              if (in_str)
                                last_str_tok = c;
                            }
                        }
                    }
                  else
                    {
                      in_str = !in_str;
                      if (in_str)
                        last_str_tok = c;
                    }
                }
            }
        }

      if (c == tar)
        {
          if (case_brack)
            if (!gb)
              if (case_str)
                if (!in_str)
                  break;
                else
                  ;
              else
                break;
            else
              ;
          else
            {
              if (case_str)
                if (!in_str)
                  break;
                else
                  ;
              else
                break;
            }
        }

      if (case_brack)
        {
          if (c == '(' || c == '{' || c == '[')
            {
              if (case_str)
                if (!in_str)
                  gb++;
                else
                  ;
              else
                gb++;
            }

          if (c == ')' || c == '}' || c == ']')
            {
              if (case_str)
                if (!in_str)
                  gb--;
                else
                  ;
              else
                gb--;
            }
        }
    }

  return i;
}

RSHELL_API void
rshell_ast_print_stmt (stmt_t st)
{
  printf ("[stmt] is_labelled: %d", st.meta.is_labelled);

  if (st.meta.is_labelled)
    printf (", label_name: %s\n", st.meta.label_name);
  else
    printf ("\n");

  switch (st.type)
    {
    case RSL_STMT_COMMENT:
      {
        printf ("[cmt] %s\n", st.v.stmt_cmt.val);
      }
      break;
    case RSL_STMT_BLOCK_IF:
      {
        printf ("[if_block]\n[cond] \"%s\"\n[body] \"%s\"\n",
                st.v.block_if.cond, st.v.block_if.body);

        printf ("[elifs] %d\n", st.v.block_if.elif_count);
        for (size_t i = 0; i < st.v.block_if.elif_count; i++)
          {
            printf ("[%d] cond: \"%s\"\nbody: %s\n", i,
                    st.v.block_if.elif_conds[i], st.v.block_if.elif_bodies[i]);
          }

        if (st.v.block_if.body_else)
          printf ("[else] %s\n", st.v.block_if.body_else);
      }
      break;
    case RSL_STMT_COMMAND:
      {
        printf ("[command]\n[name] \"%s\"\n[args] \"%s\"\n",
                st.v.stmt_cmd.name, st.v.stmt_cmd.args);
      }
      break;
    case RSL_STMT_FUN_CALL:
      {
        printf ("[fun_call]\n[callee] \"%s\"\n[args] %d\n",
                st.v.fun_call.callee, st.v.fun_call.arg_count);

        for (size_t i = 0; i < st.v.fun_call.arg_count; i++)
          {
            printf ("[arg %d] \"%s\"\n", i, st.v.fun_call.args[i]);
          }
      }
      break;
    case RSL_STMT_FUN_DECL:
      {
        printf ("[fun_decl]\n[fname] \"%s\"\n[args] %d\n", st.v.fun_decl.fname,
                st.v.fun_decl.arg_count);

        for (size_t i = 0; i < st.v.fun_decl.arg_count; i++)
          {
            printf ("[arg %d] \"%s\"\n", i, st.v.fun_decl.args[i]);
          }

        printf ("[body] %d\n", st.v.fun_decl.body_count);

        for (size_t i = 0; i < st.v.fun_decl.body_count; i++)
          {
            rshell_ast_print_stmt (st.v.fun_decl.body[i]);
          }
      }
      break;
    case RSL_STMT_STRING:
      {
        printf ("[string] %s\n", st.v.stmt_string.val);
      }
      break;
    case RSL_STMT_VAR_DECL:
      {
        printf ("[var_decl]\n[name] \"%s\"\n[val] %s\n", st.v.var_decl.name,
                st.v.var_decl.val);
      }
      break;
    case RSL_STMT_VAR:
      {
        printf ("[var] \"%s\"\n", st.v.stmt_var.name);
      }
      break;
    case RSL_STMT_GOTO:
      {
        printf ("[goto] %s\n", st.v.stmt_goto.name);
      }
      break;
    case RSL_STMT_DECREMENT:
      {
        printf ("[dec] name: %s\n", st.v.stmt_dec.name);
      }
      break;
    case RSL_STMT_INCREMENT:
      {
        printf ("[inc] name: %s\n", st.v.stmt_inc.name);
      }
      break;
    case RSL_STMT_USE:
      {
        printf ("[use] name: %s\n", st.v.stmt_use.name);
      }
      break;
    case RSL_STMT_CLASS_DECL:
      {
        printf ("[class_decl] %s\n", st.v.stmt_class_decl.name);

        printf ("[class_body] %d\n", st.v.stmt_class_decl.body_count);

        for (size_t i = 0; i < st.v.stmt_class_decl.body_count; i++)
          {
            rshell_ast_print_stmt (st.v.stmt_class_decl.body[i]);
          }
      }
      break;
    case RSL_STMT_MEM_ACCESS:
      {
        printf ("[mem_access] attr: %s\n", st.v.mem_access.attr);
        printf ("[mem_stmt]\n");
        rshell_ast_print_stmt (*st.v.mem_access.parent);
      }
      break;

    default:
      assert (!printf ("%d\n", st.type) && "Invalid statement.");
      break;
    }
}

RSHELL_API void
rshell_ast_preprocess_tree (stmt_t **tree_org, size_t *sptr)
{
  stmt_t *tree = *tree_org;
  int shift = 0;

  for (size_t i = 0; i < *sptr; i++)
    {
      if (tree[i].type == -1)
        shift++;
      else
        {
          if (shift)
            tree[i - shift] = tree[i];
        }
    }

  *sptr -= shift;
  *tree_org = RSHELL_Realloc (*tree_org, (*sptr) * sizeof (stmt_t));
}