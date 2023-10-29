#include "rstack.h"

expr_t **expr_mem_stack;
size_t ems_count;

int *rst_breakpoints; /* indices of expressions deleted */
size_t rst_bp_count;

void _rst_bp_realloc (void);
void _rst_bp_append (int);
int _rst_bp_pop (int);

void _rst_ems_realloc (void);

RSHELL_API void
rshell_rst_exprStackInit (void)
{
  expr_mem_stack = NULL;
  ems_count = 0;
}

RSHELL_API expr_t ***
rshell_rst_getExprStack (void)
{
  return &expr_mem_stack;
}

RSHELL_API size_t *
rshell_rst_getExprStackSize (void)
{
  return &ems_count;
}

RSHELL_API expr_t *
rshell_rst_esAppend (expr_t *e)
{
  // printf ("emscount: %d, bpcount: %d\n", ems_count, rst_bp_count);
  if (rst_bp_count)
    {
      int p = _rst_bp_pop (-1);
      /* memory is already cleared, only a void is present */

      expr_mem_stack[p] = e;
      e->meta.isallocated = true;
      e->meta.id = p;
    }
  else
    {
      _rst_ems_realloc ();

      expr_mem_stack[ems_count] = e;
      e->meta.isallocated = true;
      e->meta.id = ems_count;

      ems_count++;
    }

  return e;
}

void
_rst_bp_realloc (void)
{
  rst_breakpoints
      = RSHELL_Realloc (rst_breakpoints, (rst_bp_count + 1) * sizeof (int));
}

void
_rst_bp_append (int i)
{
  _rst_bp_realloc ();
  rst_breakpoints[rst_bp_count++] = i;
}

int
_rst_bp_pop (int i)
{
  int j;
  if (i == -1)
    {
      j = rst_breakpoints[rst_bp_count - 1];
      rst_bp_count--;
    }
  else
    {
      assert (i < rst_bp_count);
      j = rst_breakpoints[i];

      for (size_t i = j + 1; i < rst_bp_count; i++)
        {
          rst_breakpoints[i - 1] = rst_breakpoints[i];
        }
      rst_bp_count--;
    }

  return j;
}

void
_rst_ems_realloc (void)
{
  expr_mem_stack
      = RSHELL_Realloc (expr_mem_stack, (ems_count + 1) * sizeof (expr_t *));
}

RSHELL_API void
rshell_rst_esModifyObjCount (expr_t *e, int m)
{
  assert (e->meta.isallocated);
  e->meta.objcount += m;
  // printf ("{%d}\n", e->meta.objcount);

  if (e->meta.objcount < 1)
    {
      // here;
      /* add breakpoint */
      _rst_bp_append (e->meta.id);

      /* free the memory */
      rshell_expr_free (e);
    }
}