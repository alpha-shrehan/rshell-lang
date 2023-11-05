#include "parser.h"
#include "classes.h"
#include "rsshell.h"

struct _fd_arg_s /* function decl arg struct */
{
  char *name;
  char *val;
};

pret_t _rshell_parse_tree (mod_t *);
char **_arg_sep_space (char *, size_t *);
struct _fd_arg_s _fd_arg_parse (char *);

expr_t *_rs_eval_arithmetic (expr_t *, mod_t *, pret_t *);
expr_t *_rs_call_function (fun_t *, expr_t **, int, mod_t *, pret_t *);

void _rs_cmd_parse_args (char **, size_t, mod_t *, pret_t *);

bool
contains_global (mod_t *mod, char *s)
{
  for (size_t i = 0; i < mod->ug_size; i++)
    if (!strcmp (mod->use_globals[i], s))
      return true;
  return false;
}

RSHELL_API mod_t *
rshell_parser_mod_new (stmt_t *st, size_t sc, mod_t *m)
{
  mod_t *t = RSHELL_Malloc (sizeof (mod_t));
  t->st_arr = st;
  t->st_count = sc;
  t->parent = m;
  t->vt = rshell_vtable_new ();
  t->ug_size = 0;
  t->use_globals = NULL;

  return t;
}

RSHELL_API pret_t
rshell_parser_parse_tree (mod_t *mod)
{
  return _rshell_parse_tree (mod);
}

pret_t
_rshell_parse_tree (mod_t *mod)
{
  pret_t res = rshell_parser_pret_new ();
  rstrie_t *label_dict = rshell_trie_new (true);

  res.consider_globals = true;

  int i = 0;
  while (i < mod->st_count)
    {
      stmt_t s = mod->st_arr[i];

      if (s.meta.is_labelled
          && !rshell_trie_key_exists (label_dict, s.meta.label_name))
        {
          int *p = RSHELL_Malloc (sizeof (int));
          *p = i;
          rshell_trie_add (label_dict, s.meta.label_name, (void *)p);
        }

      switch (s.type)
        {
        case RSL_STMT_VAR_DECL:
          {
            expr_t *val
                = rshell_parser_expr_eval (s.v.var_decl.val, mod, &res);

            char *vn = rshell_trim (s.v.var_decl.name);
            size_t vl = strlen (vn);
            bool done = false;

            for (int j = vl - 1; j >= 0; j--)
              {
                char c = vn[j];

                if (c == '-')
                  {
                    if (j < vl - 1 && vn[j + 1] == '>')
                      {
                        char *lval = RSHELL_Strdup (vn);
                        lval[j] = '\0';
                        char *rval = rshell_trim (vn + j + 2);

                        expr_t *lev
                            = rshell_parser_expr_eval (lval, mod, &res);

                        switch (lev->type)
                          {
                          case RSHELL_EXPR_COBJ:
                            {
                              cobj_t *obj = rshell_cobj_get (lev->v.e_cobj.id);
                              mod_t *obj_m = obj->mod;

                              rshell_parser_mod_var_add (
                                  obj_m, rshell_strdup (rval), val);
                            }
                            break;
                          case RSHELL_EXPR_CLASS:
                            {
                              class_t *obj
                                  = rshell_class_get (lev->v.e_class.id);
                              mod_t *obj_m = obj->mod;

                              rshell_parser_mod_var_add (
                                  obj_m, rshell_strdup (rval), val);
                            }
                            break;

                          default:
                            break;
                          }

                        rshell_expr_free (lev);
                        RSHELL_Free (rval);
                        done = true;
                        break;
                      }
                  }
              }

            if (!done)
              rshell_parser_mod_var_add (mod, RSHELL_Strdup (vn), val);

            RSHELL_Free (vn);
          }
          break;
        case RSL_STMT_COMMAND:
          {
            // printf ("[%d]\n", GC_get_heap_size ());
            size_t arg_count = 0;
            char **args = _arg_sep_space (s.v.stmt_cmd.args, &arg_count);

            _rs_cmd_parse_args (args, arg_count, mod, &res);

            fs_t *fs = rshell_cmd_fs_new (true);
            cmd_t *cmd = rshell_cmd_get (s.v.stmt_cmd.name);
            assert (cmd);

            cmd_ret_t rt = cmd->routine (args, arg_count, fs);

            for (size_t j = 0; j < arg_count; j++)
              RSHELL_Free (args[j]);
            RSHELL_Free (args);

            rshell_cmd_fs_free (fs);
            // printf ("{%d}\n", GC_get_heap_size ());
          }
          break;
        case RSL_STMT_BLOCK_IF:
          {
            if (!s.v.block_if.ast_ed)
              {
                /* make ast */

                size_t ed_count = 0;
                char **ebdy_sts = rshell_str_split_delim (s.v.block_if.body,
                                                          &ed_count, ';');

                // for (size_t j = 0; j < ed_count; j++)
                //   {
                //     printf ("{%s}\n", ebdy_sts[j]);
                //   }

                s.v.block_if.body_ast
                    = RSHELL_Malloc ((s.v.block_if.body_ast_count = ed_count)
                                     * sizeof (stmt_t));

                for (size_t j = 0; j < ed_count; j++)
                  {
                    s.v.block_if.body_ast[j] = rshell_ast_gen (ebdy_sts[j]);

                    RSHELL_Free (ebdy_sts[j]);
                  }

                rshell_ast_preprocess_tree (&s.v.block_if.body_ast,
                                            &s.v.block_if.body_ast_count);

                RSHELL_Free (ebdy_sts);
                ebdy_sts = NULL;
                ed_count = 0;

                s.v.block_if.elif_body_ast_count = s.v.block_if.elif_count;

                s.v.block_if.elif_body_ast_size_counts = RSHELL_Malloc (
                    s.v.block_if.elif_body_ast_count * sizeof (size_t));

                s.v.block_if.elif_bodies_ast = RSHELL_Malloc (
                    s.v.block_if.elif_body_ast_count * sizeof (stmt_t *));

                for (size_t j = 0; j < s.v.block_if.elif_body_ast_count; j++)
                  {
                    ebdy_sts = rshell_str_split_delim (
                        s.v.block_if.elif_bodies[j], &ed_count, ';');

                    s.v.block_if.elif_bodies_ast[j] = RSHELL_Malloc (
                        (s.v.block_if.elif_body_ast_size_counts[j] = ed_count)
                        * sizeof (stmt_t));

                    for (size_t k = 0; k < ed_count; k++)
                      {
                        s.v.block_if.elif_bodies_ast[j][k]
                            = rshell_ast_gen (ebdy_sts[k]);

                        RSHELL_Free (ebdy_sts[k]);
                      }

                    rshell_ast_preprocess_tree (
                        &s.v.block_if.elif_bodies_ast[j],
                        &s.v.block_if.elif_body_ast_size_counts[j]);

                    RSHELL_Free (ebdy_sts);
                    ebdy_sts = NULL;
                    ed_count = 0;
                  }

                if (s.v.block_if.body_else != NULL)
                  {
                    ed_count = 0;
                    ebdy_sts = rshell_str_split_delim (s.v.block_if.body_else,
                                                       &ed_count, ';');

                    s.v.block_if.body_else_ast = RSHELL_Malloc (
                        (s.v.block_if.body_else_ast_count = ed_count)
                        * sizeof (stmt_t));

                    for (size_t j = 0; j < ed_count; j++)
                      {
                        s.v.block_if.body_else_ast[j]
                            = rshell_ast_gen (ebdy_sts[j]);

                        RSHELL_Free (ebdy_sts[j]);
                      }

                    rshell_ast_preprocess_tree (
                        &s.v.block_if.body_else_ast,
                        &s.v.block_if.body_else_ast_count);

                    RSHELL_Free (ebdy_sts);
                    ebdy_sts = NULL;
                  }

                s.v.block_if.ast_ed = true;
                mod->st_arr[i] = s;
              }

            expr_t *cond_eval
                = rshell_parser_expr_eval (s.v.block_if.cond, mod, &res);

            bool cd = rshell_expr_toBool (cond_eval);

            if (cd)
              {
                stmt_t *st_pres = mod->st_arr;
                size_t stc_pres = mod->st_count;

                mod->st_arr = s.v.block_if.body_ast;
                mod->st_count = s.v.block_if.body_ast_count;

                res = rshell_parser_parse_tree (mod);

                mod->st_arr = st_pres;
                mod->st_count = stc_pres;

                /* check if signals like jump */
                if (res.sig_jump)
                  {
                    if (rshell_trie_key_exists (label_dict,
                                                res.vjmp.label_name))
                      {
                        i = *(int *)rshell_trie_get (label_dict,
                                                     res.vjmp.label_name);
                        res.sig_jump = false;
                        rshell_expr_free (cond_eval);
                        continue;
                      }
                    else
                      {
                        rshell_expr_free (cond_eval);
                        goto end;
                      }
                  }
              }
            else
              {
                bool exec_else = true;

                for (size_t j = 0; j < s.v.block_if.elif_body_ast_count; j++)
                  {
                    expr_t *ec = rshell_parser_expr_eval (
                        s.v.block_if.elif_conds[j], mod, &res);

                    bool eb = rshell_expr_toBool (ec);

                    if (eb)
                      {
                        stmt_t *st_pres = mod->st_arr;
                        size_t stc_pres = mod->st_count;

                        mod->st_arr = s.v.block_if.elif_bodies_ast[j];
                        mod->st_count
                            = s.v.block_if.elif_body_ast_size_counts[j];

                        res = rshell_parser_parse_tree (mod);

                        mod->st_arr = st_pres;
                        mod->st_count = stc_pres;

                        /* check for signals */
                        if (res.sig_jump)
                          {
                            if (rshell_trie_key_exists (label_dict,
                                                        res.vjmp.label_name))
                              {
                                i = *(int *)rshell_trie_get (
                                    label_dict, res.vjmp.label_name);

                                res.sig_jump = false;
                                i--;

                                rshell_expr_free (cond_eval);
                                goto L0;
                              }
                            else
                              {
                                rshell_expr_free (cond_eval);
                                goto end;
                              }
                          }

                        exec_else = false;
                        break;
                      }

                    rshell_expr_free (ec);
                  }

                if (exec_else)
                  {
                    stmt_t *st_pres = mod->st_arr;
                    size_t stc_pres = mod->st_count;

                    mod->st_arr = s.v.block_if.body_else_ast;
                    mod->st_count = s.v.block_if.body_else_ast_count;

                    res = rshell_parser_parse_tree (mod);

                    mod->st_arr = st_pres;
                    mod->st_count = stc_pres;

                    /* check for signals (jmp, break etc) */
                    if (res.sig_jump)
                      {
                        if (rshell_trie_key_exists (label_dict,
                                                    res.vjmp.label_name))
                          {
                            i = *(int *)rshell_trie_get (label_dict,
                                                         res.vjmp.label_name);
                            res.sig_jump = false;
                            rshell_expr_free (cond_eval);
                            continue;
                          }
                        else
                          {
                            rshell_expr_free (cond_eval);
                            goto end;
                          }
                      }
                  }
              }

            rshell_expr_free (cond_eval);
          }
          break;
        case RSL_STMT_FUN_DECL:
          {
            fun_t *nf = rshell_fun_new (
                rshell_trim (s.v.fun_decl.fname),
                (char **)RSHELL_Malloc (s.v.fun_decl.arg_count
                                        * sizeof (char *)),
                s.v.fun_decl.arg_count, false, mod);

            bool saw_def = false;

            for (size_t j = 0; j < s.v.fun_decl.arg_count; j++)
              {
                /* TODO: store default values. Currently they are being
                 * ignored. */
                struct _fd_arg_s ca = _fd_arg_parse (s.v.fun_decl.args[j]);

                if (saw_def)
                  assert (!!ca.val);

                if (ca.val != NULL)
                  saw_def = true;

                nf->args[j] = ca.name;

                /* FOR NOW */
                RSHELL_Free (ca.val);
              }

            if (*nf->name == '$')
              nf->name++;
            nf->v.coded.body = s.v.fun_decl.body;
            nf->v.coded.body_count = s.v.fun_decl.body_count;
            rshell_fun_add (nf);

            expr_t *ev = rshell_expr_new (RSHELL_EXPR_FUNCTION);
            ev->v.e_func.id = nf->id;

            rshell_parser_mod_var_add (mod, nf->name, ev);
          }
          break;
        case RSL_STMT_FUN_CALL:
          {
            char *_fn;
            char *fname;
            _fn = fname = rshell_trim (s.v.fun_call.callee);

            // printf ("%s\n", fname);
            expr_t *ev = NULL;

            if (s.v.fun_call.eval_callee)
              {
                ev = rshell_parser_expr_eval (fname, mod, &res);
              }
            else
              {
                if (*fname == '$')
                  fname++;
                ev = rshell_parser_mod_var_get (mod, fname);
              }

            assert (!!ev);

            expr_t **arg_list
                = RSHELL_Malloc (s.v.fun_call.arg_count * sizeof (expr_t *));

            for (size_t j = 0; j < s.v.fun_call.arg_count; j++)
              {
                arg_list[j] = rshell_parser_expr_eval (s.v.fun_call.args[j],
                                                       mod, &res);
              }

            if (ev->meta.ap_count)
              {
                int p1 = s.v.fun_call.arg_count;
                int p2 = ev->meta.ap_count;
                int p = p1 + p2;

                arg_list = RSHELL_Realloc (arg_list, p * sizeof (expr_t *));

                for (int j = p1 - 1; j >= 0; j--)
                  arg_list[j + p2] = arg_list[j];

                for (size_t j = 0; j < p2; j++)
                  arg_list[j] = ev->meta.arg_pass[j];

                // for (size_t j = 0; j < p; j++)
                //   printf ("[%d]\n", arg_list[j]->type);
              }

            switch (ev->type)
              {
              case RSHELL_EXPR_FUNCTION:
                {
                  fun_t *nf = rshell_fun_get_byID (ev->v.e_func.id);
                  assert (!!nf);

                  rshell_expr_free (_rs_call_function (
                      nf, arg_list, s.v.fun_call.arg_count, mod, &res));
                }
                break;

              default:
                break;
              }

            RSHELL_Free (_fn);
          }
          break;
        case RSL_STMT_DECREMENT:
          {
            expr_t *ev = rshell_parser_mod_var_get (mod, s.v.stmt_dec.name);
            assert (!!ev && ev->type == RSHELL_EXPR_INT);
            ev->v.e_int.val--;

            // expr_t *nv = rshell_expr_new (RSHELL_EXPR_INT);
            // nv->v.e_int.val = ev->v.e_int.val - 1;

            // rshell_parser_mod_var_add (mod, s.v.stmt_dec.name, nv);
          }
          break;
        case RSL_STMT_INCREMENT:
          {
            expr_t *ev = rshell_parser_mod_var_get (mod, s.v.stmt_dec.name);
            assert (!!ev && ev->type == RSHELL_EXPR_INT);
            ev->v.e_int.val++;

            // expr_t *nv = rshell_expr_new (RSHELL_EXPR_INT);
            // nv->v.e_int.val = ev->v.e_int.val + 1;

            // rshell_parser_mod_var_add (mod, s.v.stmt_dec.name, nv);
          }
          break;
        case RSL_STMT_GOTO:
          {
            if (!rshell_trie_key_exists (label_dict, s.v.stmt_goto.name))
              {
                res.sig_jump = true;
                res.vjmp.label_name = s.v.stmt_goto.name;
                goto end;
              }

            i = *(int *)rshell_trie_get (label_dict, s.v.stmt_goto.name);
            continue;
          }
          break;
        case RSL_STMT_USE:
          {
            char *name_f = rshell_trim (s.v.stmt_use.name);
            assert (*name_f == '@');

            mod->use_globals = RSHELL_Realloc (
                mod->use_globals, (mod->ug_size + 1) * sizeof (char *));
            mod->use_globals[mod->ug_size++] = name_f + 1;
          }
          break;
        case RSL_STMT_CLASS_DECL:
          {
            class_t *nc = rshell_class_new (
                RSHELL_Strdup (s.v.stmt_class_decl.name), NULL);

            mod_t *nc_mod = rshell_parser_mod_new (
                s.v.stmt_class_decl.body, s.v.stmt_class_decl.body_count, mod);

            rshell_parser_parse_tree (nc_mod);
            nc->mod = nc_mod;

            rshell_class_append (nc);
            rshell_class_addToObjCount (nc, 1);

            expr_t *cv = rshell_expr_new (RSHELL_EXPR_CLASS);
            cv->v.e_class.id = nc->meta.id;

            rshell_parser_mod_var_add (
                mod, RSHELL_Strdup (s.v.stmt_class_decl.name), cv);
          }
          break;

        default:
          break;
        }

    L0:
      i++;
    }

  rshell_trie_free (label_dict);
end:
  return res;
}

RSHELL_API pret_t
rshell_parser_pret_new (void)
{
  return (pret_t){ .sig_break = false,
                   .sig_continue = false,
                   .sig_return = false };
}

RSHELL_API expr_t *
rshell_parser_expr_eval (char *s, mod_t *mod, pret_t *mgr)
{
  s = rshell_trim (s);

  expr_t *e = NULL;
  size_t dl = strlen (s);
  size_t i = 0;

  while (i < dl)
    {
      char d = s[i];

      switch (d)
        {
        case '@': /* type specifier */
          {
            int istart = rget_next_chr (s, '(', i + 1, 1, 1);
            assert (istart != dl && "'(' not found.");

            int p = istart - i - 1;
            char *id = RSHELL_Malloc ((istart - i) * sizeof (char));

            strncpy (id, s + i + 1, p);
            id[p] = '\0';

            // printf ("Identifier: %s\n", id);

            if (!strcmp (id, "int"))
              {
                if (s[istart] == '(') /* Always true?? */
                  {
                    int arg_ei = rget_next_chr (s, ')', istart + 1, 1, 1);
                    int p = arg_ei - istart - 1;

                    char *vl = RSHELL_Malloc ((p + 1) * sizeof (char));
                    strncpy (vl, s + istart + 1, p);
                    vl[p] = '\0';

                    char *vltr = rshell_trim (vl);
                    assert (strlen (vltr));
                    RSHELL_Free (vl);

                    // bool cg_p = mgr->consider_globals;
                    // mgr->consider_globals = false;

                    expr_t *vlev = rshell_parser_expr_eval (vltr, mod, mgr);

                    assert (vlev->type == RSHELL_EXPR_NOTYPE
                            || vlev->type == RSHELL_EXPR_FLOAT
                            || vlev->type == RSHELL_EXPR_INT
                            || vlev->type == RSHELL_EXPR_STRING);

                    // mgr->consider_globals = cg_p;

                    if (!!e)
                      rshell_expr_free (e);
                    e = rshell_expr_new (RSHELL_EXPR_INT);

                    switch (vlev->type)
                      {
                      case RSHELL_EXPR_NOTYPE:
                        e->v.e_int.val = atoi (vlev->v.e_notype.val);
                        break;
                      case RSHELL_EXPR_INT:
                        e->v.e_int.val = vlev->v.e_int.val;
                        break;
                      case RSHELL_EXPR_FLOAT:
                        e->v.e_int.val = (int32_t)vlev->v.e_float.val;
                        break;
                      case RSHELL_EXPR_STRING:
                        e->v.e_int.val = atoi (vlev->v.e_string.val);
                        break;

                      default:
                        break;
                      }

                    rshell_expr_free (vlev);
                    RSHELL_Free (vltr);

                    i = arg_ei;
                  }
              }
            else if (!strcmp (id, "float"))
              {
                if (s[istart] == '(')
                  {
                    int arg_ei = rget_next_chr (s, ')', istart + 1, 1, 1);
                    int p = arg_ei - istart - 1;

                    char *vl = RSHELL_Malloc ((p + 1) * sizeof (char));
                    strncpy (vl, s + istart + 1, p);
                    vl[p] = '\0';

                    char *vltr = rshell_trim (vl);
                    assert (strlen (vltr));
                    RSHELL_Free (vl);

                    // bool cg_p = mgr->consider_globals;
                    // mgr->consider_globals = false;

                    expr_t *vlev = rshell_parser_expr_eval (vltr, mod, mgr);
                    assert (vlev->type == RSHELL_EXPR_NOTYPE);

                    // mgr->consider_globals = cg_p;

                    if (!!e)
                      rshell_expr_free (e);
                    e = rshell_expr_new (RSHELL_EXPR_FLOAT);
                    e->v.e_float.val = atof (vlev->v.e_notype.val);

                    rshell_expr_free (vlev);
                    RSHELL_Free (vltr);

                    i = arg_ei;
                  }
              }
            else if (!strcmp (id, "seq"))
              {
                int end_idx = rget_next_chr (s, ')', istart + 1, 1, 1);
                int comma_idx = rget_next_chr (s, ',', istart + 1, 1, 1);
                int last_comma_idx = istart;

                array_t *narr = rshell_seq_new ();

                do
                  {
                    int p = comma_idx - last_comma_idx - 1;
                    char *expr_data = RSHELL_Malloc ((p + 1) * sizeof (char));

                    strncpy (expr_data, s + last_comma_idx + 1, p);
                    expr_data[p] = '\0';

                    /* internally allocates to stack */
                    rshell_seq_add (
                        narr, rshell_rst_esAppend (rshell_parser_expr_eval (
                                  expr_data, mod, mgr)));

                    RSHELL_Free (expr_data);

                    last_comma_idx = comma_idx;
                    comma_idx = rget_next_chr (s, ',', comma_idx + 1, 1, 1);
                  }
                while (comma_idx < end_idx);

                comma_idx = end_idx;
                int p = comma_idx - last_comma_idx - 1;
                char *expr_data = RSHELL_Malloc ((p + 1) * sizeof (char));

                strncpy (expr_data, s + last_comma_idx + 1, p);
                expr_data[p] = '\0';

                /* internally allocates to stack */
                rshell_seq_add (
                    narr, rshell_rst_esAppend (
                              rshell_parser_expr_eval (expr_data, mod, mgr)));

                RSHELL_Free (expr_data);

                rshell_seq_append (narr);

                if (!!e)
                  rshell_expr_free (e);
                e = rshell_expr_new (RSHELL_EXPR_SEQUENCE);
                e->v.e_seq.id = narr->id;
                rshell_seq_addToObjCount (narr, 1);

                i = end_idx;
              }
            /* removed because of bugs */
            /* else if (!strcmp (id, "str") || !strcmp (id, "string"))
              {
                if (s[istart] == '(')
                  {
                    int arg_ei = rget_next_chr (s, ')', istart + 1, 1, 0);
                    int p = arg_ei - istart - 1;
                    char *vl = RSHELL_Malloc ((p + 1) * sizeof (char));

                    strncpy (vl, s + istart + 1, p);
                    vl[p] = '\0';

                    expr_t *vlev = rshell_parser_expr_eval (vl, mod, mgr);
                    assert (vlev->type == RSHELL_EXPR_NOTYPE
                            || vlev->type == RSHELL_EXPR_STRING);

                    e->type = RSHELL_EXPR_STRING;
                    if (vlev->type == RSHELL_EXPR_NOTYPE)
                      e->v.e_string.val = vlev->v.e_notype.val;
                    else
                      e->v.e_string.val = vlev->v.e_string.val;

                    // printf ("%s\n", e->v.e_string.val);

                    i = arg_ei;
                    goto L0;
                  }
              } */
            else
              {
                assert (0 && "No @ rule exists.\n");
              }

            RSHELL_Free (id);
            goto L0;
          }
          break;
        case '\'':
        case '\"':
          {
            int send = rget_next_chr (s, d, i, 1,
                                      0); /* i NOT i+1 because string rules are
                                             before matching chars */
            int p = send - i - 1;

            char *sr = RSHELL_Malloc ((p + 1) * sizeof (char));
            strncpy (sr, s + i + 1, p);
            sr[p] = '\0';

            // printf ("%s\n", sr);

            if (!!e)
              rshell_expr_free (e);
            e = rshell_expr_new (RSHELL_EXPR_STRING);
            e->v.e_string.val = rshell_unescape_str (sr);

            RSHELL_Free (sr);

            i = send;
          }
          break;
        case 'l':
        case 'g':
          {
            if (i)
              goto L0;

            bool eq_flag = false;

            if (s[i + 1] == 'e' && s[i + 2] == 'q')
              eq_flag = true;

            int as_idx = i + 1 + (2 * eq_flag);
            while (s[as_idx] == ' ' || s[as_idx] == '\t' || s[as_idx] == '\n'
                   || s[as_idx] == '\r')
              as_idx++;

            int idx_space = rget_next_chr (s, ' ', as_idx, 1, 1);
            int p = idx_space - as_idx;

            char *arg1 = RSHELL_Malloc ((p + 1) * sizeof (char));
            strncpy (arg1, s + as_idx, p);
            arg1[p] = '\0';

            p = dl - idx_space - 1;
            char *arg2 = RSHELL_Malloc ((p + 1) * sizeof (char));
            strncpy (arg2, s + idx_space + 1, p);
            arg2[p] = '\0';

            // printf ("[%s],[%s]\n", arg1, arg2);

            expr_t *arg1_eval = rshell_parser_expr_eval (arg1, mod, mgr);
            expr_t *arg2_eval = rshell_parser_expr_eval (arg2, mod, mgr);

            bool cmp_r = false;

            if (RSEXPR_IS_NUM (*arg1_eval) && RSEXPR_IS_NUM (*arg2_eval))
              {
                float a1, a2;

                if (arg1_eval->type == RSHELL_EXPR_INT)
                  a1 = arg1_eval->v.e_int.val;
                else
                  a1 = arg1_eval->v.e_float.val;

                if (arg2_eval->type == RSHELL_EXPR_INT)
                  a2 = arg2_eval->v.e_int.val;
                else
                  a2 = arg2_eval->v.e_float.val;

                // printf ("%f %f\n", a1, a2);
                if (eq_flag)
                  cmp_r = d == 'l' ? a1 <= a2 : a1 >= a2;
                else
                  cmp_r = d == 'l' ? a1 < a2 : a1 > a2;
              }

            if (!!e)
              rshell_expr_free (e);
            e = rshell_expr_new (RSHELL_EXPR_INT);
            e->v.e_int.val = cmp_r;

            // printf ("%d\n", cmp_r);

            RSHELL_Free (arg1);
            RSHELL_Free (arg2);
            rshell_expr_free (arg1_eval);
            rshell_expr_free (arg2_eval);

            goto end;
          }
          break;
        case 'e':
          {
            if (!(i + 1 < dl && s[i + 1] == 'q' && !i))
              goto L0;

            /* eq */

            int as_idx = i + 2;
            while (s[as_idx] == ' ' || s[as_idx] == '\t' || s[as_idx] == '\n'
                   || s[as_idx] == '\r')
              as_idx++;

            int idx_space = rget_next_chr (s, ' ', as_idx, 1, 1);
            int p = idx_space - as_idx;

            char *arg1 = RSHELL_Malloc ((p + 1) * sizeof (char));
            strncpy (arg1, s + as_idx, p);
            arg1[p] = '\0';

            p = dl - idx_space - 1;
            char *arg2 = RSHELL_Malloc ((p + 1) * sizeof (char));
            strncpy (arg2, s + idx_space + 1, p);
            arg2[p] = '\0';

            // printf ("[%s],[%s]\n", arg1, arg2);

            expr_t *arg1_eval = rshell_parser_expr_eval (arg1, mod, mgr);
            expr_t *arg2_eval = rshell_parser_expr_eval (arg2, mod, mgr);
            bool cmp_r = false;

            if (RSEXPR_IS_NUM (*arg1_eval) && RSEXPR_IS_NUM (*arg2_eval))
              {
                float a1, a2;

                if (arg1_eval->type == RSHELL_EXPR_INT)
                  a1 = arg1_eval->v.e_int.val;
                else
                  a1 = arg1_eval->v.e_float.val;

                if (arg2_eval->type == RSHELL_EXPR_INT)
                  a2 = arg2_eval->v.e_int.val;
                else
                  a2 = arg2_eval->v.e_float.val;

                // printf ("%f %f\n", a1, a2);
                cmp_r = a1 == a2;
              }

            if (!!e)
              rshell_expr_free (e);
            e = rshell_expr_new (RSHELL_EXPR_INT);
            e->v.e_int.val = cmp_r;

            // printf ("%d\n", cmp_r);
            rshell_expr_free (arg1_eval);
            rshell_expr_free (arg2_eval);

            goto end;
          }
          break;
        case 'n':
          {
            if (!(i + 2 < dl && s[i + 1] == 'e' && s[i + 2] == 'q' && !i))
              goto L0;
            /* neq */

            int as_idx = i + 3;
            while (s[as_idx] == ' ' || s[as_idx] == '\t' || s[as_idx] == '\n'
                   || s[as_idx] == '\r')
              as_idx++;

            int idx_space = rget_next_chr (s, ' ', as_idx, 1, 1);
            int p = idx_space - as_idx;

            char *arg1 = RSHELL_Malloc ((p + 1) * sizeof (char));
            strncpy (arg1, s + as_idx, p);
            arg1[p] = '\0';

            p = dl - idx_space - 1;
            char *arg2 = RSHELL_Malloc ((p + 1) * sizeof (char));
            strncpy (arg2, s + idx_space + 1, p);
            arg2[p] = '\0';

            // printf ("[%s],[%s]\n", arg1, arg2);

            expr_t *arg1_eval = rshell_parser_expr_eval (arg1, mod, mgr);
            expr_t *arg2_eval = rshell_parser_expr_eval (arg2, mod, mgr);
            bool cmp_r = false;

            if (RSEXPR_IS_NUM (*arg1_eval) && RSEXPR_IS_NUM (*arg2_eval))
              {
                float a1, a2;

                if (arg1_eval->type == RSHELL_EXPR_INT)
                  a1 = arg1_eval->v.e_int.val;
                else
                  a1 = arg1_eval->v.e_float.val;

                if (arg2_eval->type == RSHELL_EXPR_INT)
                  a2 = arg2_eval->v.e_int.val;
                else
                  a2 = arg2_eval->v.e_float.val;

                // printf ("%f %f\n", a1, a2);
                cmp_r = a1 != a2;
              }
            else
              {
                char *arg1_repr = rshell_expr_toStr (arg1_eval);
                char *arg2_repr = rshell_expr_toStr (arg2_eval);

                cmp_r = !!strcmp (arg1_repr, arg2_repr);

                RSHELL_Free (arg1_repr);
                RSHELL_Free (arg2_repr);
              }

            if (!!e)
              rshell_expr_free (e);
            e = rshell_expr_new (RSHELL_EXPR_INT);
            e->v.e_int.val = cmp_r;

            // printf ("%d\n", cmp_r);
            rshell_expr_free (arg1_eval);
            rshell_expr_free (arg2_eval);

            goto end;
          }
          break;
        case '$':
          {
            char *vname = RSHELL_Malloc (dl * sizeof (char)); /* max size */
            size_t vc = 0;

            size_t j;
            for (j = i + 1; j < dl; j++)
              {
                char c = s[j];

                if (!(isalnum (c) || c == '_'))
                  break;

                vname[vc++] = c;
              }

            vname[vc++] = '\0';

            expr_t *vg = rshell_parser_mod_var_get (mod, vname);
            // printf ("%s\n", rshell_expr_toStr (vg));

            if (!vg)
              {
                here;
                printf ("%s\n", vname);
                assert (0 && "Var not found");
              }

            // *e = *vg;
            if (!!e)
              rshell_expr_free (e);
            e = rshell_expr_copy (vg);
            // here;

            RSHELL_Free (vname);
            // here;

            i = j - 1;
            // printf ("%c\n", s[i]);
            goto L0;
          }
          break;
        case '[':
          {
            assert (e);

            int end_idx = rget_next_chr (s, ']', i + 1, 1, 1);
            int p = end_idx - i - 1;
            char *expr_data = RSHELL_Malloc ((i + 1) * sizeof (char));

            strncpy (expr_data, s + i + 1, p);
            expr_data[p] = '\0';
            expr_t *evexp = rshell_parser_expr_eval (expr_data, mod, mgr);

            switch (e->type)
              {
              case RSHELL_EXPR_SEQUENCE:
                {
                  array_t *t = rshell_seq_get (e->v.e_seq.id);
                  assert (evexp->type == RSHELL_EXPR_INT
                          || evexp->type == RSHELL_EXPR_NOTYPE);

                  int idx = -1;
                  if (evexp->type == RSHELL_EXPR_INT)
                    {
                      idx = evexp->v.e_int.val % t->size;
                    }
                  else
                    {
                      size_t l = strlen (evexp->v.e_notype.val);
                      for (size_t i = 0; i < l; i++)
                        assert (isdigit (evexp->v.e_notype.val[i]));

                      idx = atoi (evexp->v.e_notype.val) % t->size;
                    }

                  rshell_expr_free (e);
                  e = rshell_expr_copy (t->vals[idx]);
                }
                break;

              default:
                break;
              }

            RSHELL_Free (expr_data);
            rshell_expr_free (evexp);

            i = end_idx;
          }
          break;
        case '(':
          {
            int bre_idx = rget_next_chr (s, ')', i + 1, 1, 1);
            int p = bre_idx - i - 1;
            char *dat_enc = RSHELL_Malloc ((p + 1) * sizeof (char));
            strncpy (dat_enc, s + i + 1, p);
            dat_enc[p] = '\0';

            char *_de = dat_enc;
            dat_enc = rshell_trim (dat_enc);
            RSHELL_Free (_de);
            int datlen = strlen (dat_enc);

            if (e == NULL)
              {
                e = rshell_parser_expr_eval (dat_enc, mod, mgr);
                i = bre_idx;
              }
            else
              {
                expr_t **arglist = NULL;
                int al = 0;
                int ci = rget_next_chr (dat_enc, ',', 0, 1, 1);

                if (ci == datlen) /* either 1 or no arguments */
                  {
                    if (datlen) /* 1 argument */
                      {
                        arglist = RSHELL_Malloc (sizeof (expr_t *));
                        *arglist = rshell_parser_expr_eval (dat_enc, mod, mgr);
                        al = 1;
                      }
                  }
                else
                  {
                    int lci = 0;
                    while (ci != datlen)
                      {
                        arglist = RSHELL_Realloc (
                            arglist, (al + 1) * sizeof (expr_t *));

                        int p = ci - lci;
                        char *dt = RSHELL_Malloc ((p + 1) * sizeof (char));
                        strncpy (dt, dat_enc + lci, p);
                        dt[p] = '\0';

                        arglist[al++] = rshell_parser_expr_eval (dt, mod, mgr);

                        RSHELL_Free (dt);
                        lci = ci + 1;
                        ci = rget_next_chr (dat_enc, ',', lci, 1, 1);
                      }

                    arglist = RSHELL_Realloc (arglist,
                                              (al + 1) * sizeof (expr_t *));

                    int p = ci - lci;
                    char *dt = RSHELL_Malloc ((p + 1) * sizeof (char));
                    strncpy (dt, dat_enc + lci, p);
                    dt[p] = '\0';

                    arglist[al++] = rshell_parser_expr_eval (dt, mod, mgr);

                    RSHELL_Free (dt);
                  }

                switch (e->type)
                  {
                  case RSHELL_EXPR_FUNCTION:
                    {
                      fun_t *f = rshell_fun_get_byID (e->v.e_func.id);
                      assert (!!f);

                      rshell_expr_free (e);
                      e = _rs_call_function (f, arglist, al, mod, mgr);
                    }
                    break;
                  case RSHELL_EXPR_CLASS:
                    {
                      class_t *ct = rshell_class_get (e->v.e_class.id);
                      assert (!!ct);
                      mod_t *m = rshell_parser_mod_copy_deep (ct->mod);

                      expr_t *_f_init
                          = rshell_vtable_get (m->vt, CLASS_CONSTRUCTOR_NAME);

                      cobj_t *obj = rshell_cobj_new (ct, m);
                      rshell_cobj_append (obj);

                      rshell_expr_free (e);
                      e = rshell_expr_new (RSHELL_EXPR_COBJ);
                      e->v.e_cobj.id = obj->meta.id;

                      arglist = RSHELL_Realloc (arglist,
                                                (al + 1) * sizeof (expr_t *));

                      for (int j = al; j > -1; j--)
                        arglist[j + 1] = arglist[j];
                      arglist[0] = rshell_expr_copy (e);
                      al++;

                      if (_f_init != NULL)
                        {
                          fun_t *f
                              = rshell_fun_get_byID (_f_init->v.e_func.id);

                          expr_t *this_ptr
                              = rshell_rst_esAppend (rshell_expr_copy (e));

                          rshell_expr_free (
                              _rs_call_function (f, arglist, al, m, mgr));
                        }
                    }
                    break;

                  default:
                    break;
                  }

                for (size_t j = 0; j < al; j++)
                  rshell_expr_free (arglist[j]);
                RSHELL_Free (arglist);
              }

            RSHELL_Free (dat_enc);
            i = bre_idx;
          }
          break;
        case '-':
          {
            if (i < dl - 1)
              {
                if (s[i + 1] != '>')
                  goto arith;
              }
            else
              goto arith;

            /* -> operator */
            assert (!!e);

            /* get attribute name */
            int word_ei = i + 2;
            while (isalnum (s[word_ei]) || s[word_ei] == '_')
              word_ei++;

            int p = word_ei - i - 2;
            char *attr_name = RSHELL_Malloc ((p + 1) * sizeof (char));

            strncpy (attr_name, s + i + 2, p);
            attr_name[p] = '\0';

            switch (e->type)
              {
              case RSHELL_EXPR_CLASS:
                {
                  class_t *ct = rshell_class_get (e->v.e_class.id);
                  assert (!!ct);
                  mod_t *m = ct->mod;

                  mod_t *par = m->parent;
                  m->parent = NULL;

                  expr_t *v = rshell_parser_mod_var_get (m, attr_name);
                  assert (!!v && "Attribute not found.");

                  rshell_expr_free (e);
                  e = rshell_expr_copy (v);

                  m->parent = par;
                }
                break;
              case RSHELL_EXPR_COBJ:
                {
                  cobj_t *ct = rshell_cobj_get (e->v.e_cobj.id);
                  assert (!!ct);
                  mod_t *m = ct->mod;

                  mod_t *par = m->parent;
                  m->parent = NULL;

                  expr_t *v = rshell_parser_mod_var_get (m, attr_name);
                  assert (!!v && "Attribute not found.");

                  rshell_expr_free (e);
                  e = rshell_expr_copy (v);

                  rshell_expr_addToArgPass (
                      e, rshell_expr_fromExpr (
                             (expr_t){ .type = RSHELL_EXPR_COBJ,
                                       .v.e_cobj.id = ct->meta.id }));

                  m->parent = par;
                }
                break;

              default:
                assert (0 && "Data type does not have public attributes.");
                break;
              }

            RSHELL_Free (attr_name);
            i = word_ei - 1;
          }
          break;
        case '+':
        case '*':
        case '/':
        case '%':
          {
          arith:
            assert (!e);

            char *raw = RSHELL_Strdup (s + 1);
            char *p = raw;
            raw = rshell_trim (raw);
            RSHELL_Free (p);

            int space_idx = rget_next_chr (raw, ' ', 0, 1, 1);
            char *arg1 = RSHELL_Strdup (raw);
            arg1[space_idx] = '\0';
            char *arg2 = RSHELL_Strdup (raw + space_idx);

            char *a1 = arg1;
            arg1 = rshell_trim (arg1);
            RSHELL_Free (a1);

            char *a2 = arg2;
            arg2 = rshell_trim (arg2);
            RSHELL_Free (a2);

            expr_t *ev1 = rshell_parser_expr_eval (arg1, mod, mgr);
            expr_t *ev2 = rshell_parser_expr_eval (arg2, mod, mgr);

            if (!!e)
              rshell_expr_free (e);
            e = rshell_expr_new (RSHELL_EXPR_ARITH);
            e->v.e_arith.lval = ev1;
            e->v.e_arith.rval = ev2;
            e->v.e_arith.op = d;

            expr_t *r = _rs_eval_arithmetic (e, mod, mgr);

            rshell_expr_free (ev1);
            rshell_expr_free (ev2);
            rshell_expr_free (e);
            e = r;
            // printf ("%d\n", r->type);

            RSHELL_Free (arg1);
            RSHELL_Free (arg2);
            RSHELL_Free (raw);

            goto end;
          }
          break;
        case '`':
          {
            int tick_end
                = rget_next_chr (s, '`', i, 1, 1); /* IT WILL BE i NOT i+1 */
            int p = tick_end - i - 1;
            // printf ("%c\n", s[tick_end]);
            char *_cmd = RSHELL_Malloc ((p + 1) * sizeof (char));
            strncpy (_cmd, s + i + 1, p);
            _cmd[p] = '\0';

            // printf ("%s\n", _cmd);

            char *cmd_name = NULL, *cmd_args = NULL;
            int first_space_idx = rget_next_chr (_cmd, ' ', 0, 1, 1);

            cmd_name = RSHELL_Strdup (_cmd);
            cmd_name[first_space_idx] = '\0';

            cmd_args = first_space_idx != strlen (_cmd)
                           ? RSHELL_Strdup (_cmd + first_space_idx + 1)
                           : NULL;

            size_t arg_count = 0;
            char **args = _arg_sep_space (cmd_args, &arg_count);
            _rs_cmd_parse_args (args, arg_count, mod, mgr);

            fs_t *fs = rshell_cmd_fs_new (false);
            cmd_t *cmd = rshell_cmd_get (cmd_name);
            assert (cmd);

            cmd_ret_t rt = cmd->routine (args, arg_count, fs);

            array_t *narr = rshell_seq_new ();

            expr_t *idx_0 = rshell_expr_new (RSHELL_EXPR_INT);
            idx_0->v.e_int.val = rt.code;

            expr_t *idx_1 = rshell_expr_new (RSHELL_EXPR_STRING);
            idx_1->v.e_string.val = RSHELL_Strdup (fs->buf);

            expr_t *idx_2 = rshell_expr_new (RSHELL_EXPR_STRING);
            idx_2->v.e_string.val = RSHELL_Strdup (fs->buf_err);

            rshell_seq_add (narr, idx_0);
            rshell_seq_add (narr, idx_1);
            rshell_seq_add (narr, idx_2);

            rshell_seq_append (narr);

            if (!!e)
              rshell_expr_free (e);
            e = rshell_expr_new (RSHELL_EXPR_SEQUENCE);
            e->v.e_seq.id = narr->id;
            rshell_seq_addToObjCount (narr, 1);

            for (size_t j = 0; j < arg_count; j++)
              RSHELL_Free (args[j]);
            RSHELL_Free (args);

            RSHELL_Free (cmd_name);
            RSHELL_Free (_cmd);

            rshell_cmd_fs_free (fs);
            i = tick_end;
          }
          break;
        default:
          break;
        }

    L0:
      i++;
    }

end:
  if (e == NULL || e->type == -1)
    {
      e = rshell_expr_new (RSHELL_EXPR_NOTYPE);
      e->v.e_notype.val = RSHELL_Strdup (s);

      if (mgr->consider_globals)
        {
          if (contains_global (mod, "int"))
            {
              char *pres_s = rshell_trim (s);

              size_t j = 0, dl = strlen (pres_s);
              while (j < dl)
                {
                  if (pres_s[j] >= '0' && pres_s[j] <= '9')
                    j++;
                  else
                    break;
                }

              if (j == dl) /* all are numeric */
                {
                  RSHELL_Free (e->v.e_notype.val);
                  e->type = RSHELL_EXPR_INT;
                  e->v.e_int.val = atoi (pres_s);
                }

              RSHELL_Free (pres_s);
            }

          if (contains_global (mod, "float"))
            {
              char *pres_s = rshell_trim (s);

              size_t j = 0, dl = strlen (pres_s);
              int saw_dot = 0;
              while (j < dl)
                {
                  if (pres_s[j] == '.')
                    saw_dot++;
                  if ((pres_s[j] >= '0' && pres_s[j] <= '9')
                      || pres_s[j] == '.')
                    j++;
                  else
                    break;
                }

              if (j == dl && saw_dot == 1) /* all are numeric */
                {
                  RSHELL_Free (e->v.e_notype.val);
                  e->type = RSHELL_EXPR_FLOAT;
                  e->v.e_float.val = atof (pres_s);
                }

              RSHELL_Free (pres_s);
            }
        }
    }

  RSHELL_Free (s);
  return e;
}

RSHELL_API void
rshell_parser_mod_var_add (mod_t *mod, char *k, expr_t *v)
{
  expr_t *gv = rshell_vtable_get (mod->vt, k);

  if (gv != NULL)
    {
      rshell_rst_esModifyObjCount (gv, -1);

      rshell_vtable_add_withMeta (mod->vt, k, rshell_rst_esAppend (v),
                                  (struct _rshell_vtable_add_meta_stru){
                                      .var_exists = true, .ve_idx = -1 });
    }
  else
    {
      rshell_vtable_add (mod->vt, k, rshell_rst_esAppend (v));
    }

  rshell_rst_esModifyObjCount (v, 1);
}

RSHELL_API expr_t *
rshell_parser_mod_var_get (mod_t *mod, char *k)
{
  expr_t *g = rshell_vtable_get (mod->vt, k);

  if (!g)
    {
      if (mod->parent)
        return rshell_parser_mod_var_get (mod->parent, k);
    }

  return g;
}

char **
_arg_sep_space (char *arg, size_t *s_ptr)
{
  if (arg == NULL)
    return NULL;

  char **res = NULL;
  size_t rc = 0;

  int lim_x = 0, lim_y = 0;
  size_t dl = strlen (arg);
  int gb = 0;
  int in_str = 0;

  size_t i;
  for (i = 0; i < dl; i++)
    {
      char c = arg[i];

      if (c == ' ' && !in_str && !gb)
        {
          lim_y = i;

          if (lim_y == lim_x)
            continue; /* additional space */

          res = RSHELL_Realloc (res, (rc + 1) * sizeof (char *));

          int p = lim_y - lim_x;
          res[rc] = RSHELL_Malloc ((p + 1) * sizeof (char));

          strncpy (res[rc], arg + lim_x, p);
          res[rc][p] = '\0';

          char *res_pres = res[rc];
          res[rc] = rshell_trim (res[rc]);

          RSHELL_Free (res_pres);

          rc++;
          lim_x = i + 1;
          continue;
        }

      if (c == '\"' || c == '\'' || c == '`')
        {
          if (i)
            {
              if (arg[i - 1] == '\\')
                {
                  if (i > 1 && arg[i - 2] == '\\')
                    in_str = !in_str;
                }
              else
                in_str = !in_str;
            }
          else
            in_str = !in_str;
        }

      if ((c == '(' || c == '[' || c == '{') && !in_str)
        gb++;

      if ((c == ')' || c == ']' || c == '}') && !in_str)
        gb--;
    }

  if (lim_x != lim_y)
    {
      lim_y = i;
      res = RSHELL_Realloc (res, (rc + 1) * sizeof (char *));

      int p = lim_y - lim_x;
      res[rc] = RSHELL_Malloc ((p + 1) * sizeof (char));

      strncpy (res[rc], arg + lim_x, p);
      res[rc][p] = '\0';

      char *res_pres = res[rc];
      res[rc] = rshell_trim (res[rc]);

      rc++;
      RSHELL_Free (res_pres);
    }

  if (!lim_x && !lim_y)
    {
      if (dl)
        {
          res = RSHELL_Malloc (sizeof (char *));
          *res = RSHELL_Strdup (arg);
          rc++;
        }
    }

  if (s_ptr != NULL)
    *s_ptr = rc;

  return res;
}

struct _fd_arg_s
_fd_arg_parse (char *cmd)
{
  struct _fd_arg_s s;
  s.name = NULL;
  s.val = NULL;

  size_t i = 0;
  size_t dl = strlen (cmd);

  int in_str = 0;
  int gb = 0;

  while (i < dl)
    {
      char c = cmd[i];

      if (c == '\'' || c == '\"' || c == '`')
        {
          if (i)
            {
              if (cmd[i - 1] != '\\')
                in_str = !in_str;
              else
                {
                  if (i > 1 && cmd[i - 2] == '\\')
                    in_str = !in_str;
                }
            }
          else
            in_str = !in_str;
        }

      if (!in_str)
        {
          if (c == '=')
            {
              char *n = RSHELL_Malloc (i * sizeof (char));
              strncpy (n, cmd, i);
              n[i] = '\0';

              s.name = rshell_trim (n);
              RSHELL_Free (n);

              if (*s.name == '$')
                s.name++;

              s.val = RSHELL_Strdup (cmd + i + 1);
              break;
            }

          if (c == '(' || c == '{' || c == '[')
            gb++;
          if (c == ')' || c == '}' || c == ']')
            gb--;
        }

    end:
      i++;
    }

  if (s.name == NULL)
    {
      char *_p;
      char *p;
      _p = p = rshell_trim (cmd);

      if (*p == '$')
        p++;
      s.name = RSHELL_Strdup (p);
      RSHELL_Free (_p);
    }

  return s;
}

RSHELL_API void
rshell_parser_mod_free (mod_t *mod)
{
  for (size_t i = 0; i < mod->ug_size; i++)
    RSHELL_Free (mod->use_globals[i]);
  RSHELL_Free (mod->use_globals);

  for (size_t i = 0; i < RSHELL_VTABLE_SIZE; i++)
    {
      for (size_t j = 0; j < mod->vt->lens[i]; j++)
        {
          expr_t *ev = mod->vt->elems[i][j].val;
          RSHELL_Free (mod->vt->elems[i][j].name);

          if (ev)
            rshell_rst_esModifyObjCount (ev, -1);
        }
    }

  rshell_vtable_free (mod->vt);
  RSHELL_Free (mod);
}

expr_t *
_rs_eval_arithmetic (expr_t *e, mod_t *mod, pret_t *mgr)
{
  expr_t *res = NULL;
  expr_t *lhs = e->v.e_arith.lval;
  expr_t *rhs = e->v.e_arith.rval;

  // printf ("%d %d\n", lhs->type, rhs->type);

  if (RSEXPR_IS_NUM (*lhs) && RSEXPR_IS_NUM (*rhs))
    {
      res = rshell_expr_new (RSHELL_EXPR_FLOAT);
      res->v.e_float.val = 0;

      switch (e->v.e_arith.op)
        {
        case '+':
          {
            if (lhs->type == RSHELL_EXPR_INT)
              res->v.e_float.val += (float)lhs->v.e_int.val;
            else
              res->v.e_float.val += lhs->v.e_float.val;

            if (rhs->type == RSHELL_EXPR_INT)
              res->v.e_float.val += (float)rhs->v.e_int.val;
            else
              res->v.e_float.val += rhs->v.e_float.val;
          }
          break;

        case '-':
          {
            if (lhs->type == RSHELL_EXPR_INT)
              res->v.e_float.val += lhs->v.e_int.val;
            else
              res->v.e_float.val += lhs->v.e_float.val;

            if (rhs->type == RSHELL_EXPR_INT)
              res->v.e_float.val -= rhs->v.e_int.val;
            else
              res->v.e_float.val -= rhs->v.e_float.val;
          }
          break;

        case '*':
          {
            if (lhs->type == RSHELL_EXPR_INT)
              res->v.e_float.val += lhs->v.e_int.val;
            else
              res->v.e_float.val += lhs->v.e_float.val;

            if (rhs->type == RSHELL_EXPR_INT)
              res->v.e_float.val *= rhs->v.e_int.val;
            else
              res->v.e_float.val *= rhs->v.e_float.val;
          }
          break;

        case '/':
          {
            if (lhs->type == RSHELL_EXPR_INT)
              res->v.e_float.val += lhs->v.e_int.val;
            else
              res->v.e_float.val += lhs->v.e_float.val;

            if (rhs->type == RSHELL_EXPR_INT)
              res->v.e_float.val /= rhs->v.e_int.val;
            else
              res->v.e_float.val /= rhs->v.e_float.val;
          }
          break;

        case '%':
          {
            assert (lhs->type == rhs->type && lhs->type == RSHELL_EXPR_INT);
            res->type = RSHELL_EXPR_INT;
            res->v.e_int.val = lhs->v.e_int.val % rhs->v.e_int.val;
          }
          break;

        default:
          break;
        }
    }
  else if (lhs->type == rhs->type && lhs->type == RSHELL_EXPR_STRING)
    {
      res = rshell_expr_new (RSHELL_EXPR_STRING);
      res->v.e_string.val = NULL;

      switch (e->v.e_arith.op)
        {
        case '+':
          {
            res->v.e_string.val
                = RSHELL_Malloc ((strlen (lhs->v.e_string.val)
                                  + strlen (rhs->v.e_string.val) + 1)
                                 * sizeof (char));

            *res->v.e_string.val = '\0';
            strcat (res->v.e_string.val, lhs->v.e_string.val);
            strcat (res->v.e_string.val, rhs->v.e_string.val);
          }
          break;

        default:
          assert (0 && "Operator not supported.");
          break;
        }
    }

  assert (!!res);
  return res;
}

RSHELL_API mod_t *
rshell_parser_mod_copy_deep (mod_t *mod)
{
  mod_t *m = rshell_parser_mod_new (mod->st_arr, mod->st_count, mod->parent);

  rshell_vtable_free (m->vt);
  m->vt = rshell_vtable_copy_deep (mod->vt);

  m->ug_size = mod->ug_size;
  m->use_globals = RSHELL_Malloc (m->ug_size * sizeof (char *));

  for (size_t i = 0; i < m->ug_size; i++)
    m->use_globals[i] = rshell_strdup (mod->use_globals[i]);

  return m;
}

expr_t *
_rs_call_function (fun_t *f, expr_t **arglist, int arg_count, mod_t *mod,
                   pret_t *mgr)
{
  expr_t *e = NULL;
  mod_t *m = rshell_parser_mod_new (NULL, 0, f->parent);

  assert (f->arg_count == arg_count && "Invalid number of arguments passed.");

  for (size_t j = 0; j < f->arg_count; j++)
    {
      struct _fd_arg_s ca = _fd_arg_parse (f->args[j]);

      expr_t *_ev = rshell_rst_esAppend (rshell_expr_copy (arglist[j]));

      rshell_vtable_add (m->vt, ca.name, (void *)_ev);

      RSHELL_Free (ca.val);
      rshell_rst_esModifyObjCount (_ev, 1);
    }

  if (f->isnative)
    {
      e = f->v.f_native (m, mgr);
    }
  else
    {
      m->st_arr = f->v.coded.body;
      m->st_count = f->v.coded.body_count;

      rshell_parser_parse_tree (m);

      /* TODO: add return statement */
      e = rshell_expr_new (RSHELL_EXPR_INT);
      e->v.e_int.val = 0;
    }

  m->parent = NULL;
  rshell_parser_mod_free (m);
  return e;
}

void
_rs_cmd_parse_args (char **args, size_t arg_count, mod_t *mod, pret_t *mgr)
{
  for (size_t j = 0; j < arg_count; j++)
    {
      // if (*args[j] == '$' || *args[j] == '\'' || *args[j] == '\"'
      //     || *args[j] == '@')
      //   {
      //     char *pres_arg = args[j];
      //     expr_t *r = rshell_parser_expr_eval (args[j], mod,
      //     mgr);

      //     args[j] = rshell_expr_toStr (r);
      //     // args[j] = RSHELL_Strdup ("<test>");

      //     rshell_expr_free (r);
      //     RSHELL_Free (pres_arg);
      //   }
      if (*args[j] == '{' && args[j][strlen (args[j]) - 1] == '}')
        {
          char *ac = RSHELL_Strdup (args[j] + 1);
          ac[strlen (ac) - 1] = '\0';
          RSHELL_Free (args[j]);

          expr_t *ev = rshell_parser_expr_eval (ac, mod, mgr);
          args[j] = rshell_expr_toStr (ev);

          rshell_expr_free (ev);
          RSHELL_Free (ac);
        }
      if (*args[j] == '\\' && args[j][1] == '{')
        {
          char *ac = RSHELL_Strdup (args[j] + 1);
          RSHELL_Free (args[j]);
          args[j] = ac;
        }
      if (*args[j] == '\"' || *args[j] == '\'')
        {
          char *ac = RSHELL_Strdup (args[j] + 1);
          ac[strlen (ac) - 1] = '\0';

          char *p = ac;
          ac = rshell_unescape_str (ac);
          RSHELL_Free (p);

          RSHELL_Free (args[j]);
          args[j] = ac;
        }

      // printf ("%s\n", args[j]);
    }
}