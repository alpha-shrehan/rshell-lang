#pragma once

#include "header.h"

struct _rshell_cmd_fstream_stru
{
  char *buf;
  char *buf_err;

  bool use_standard;
  FILE *_stdout, *_stdin, *_stderr;
};

typedef struct _rshell_cmd_fstream_stru fs_t;

struct _rshell_cmd_ret_stru
{
  int code;
};

typedef struct _rshell_cmd_ret_stru cmd_ret_t;

struct _rshell_cmd_stru
{
  char *name;
  cmd_ret_t (*routine) (char **, int, fs_t *);

  int id;
};

typedef struct _rshell_cmd_stru cmd_t;

#ifdef __cplusplus
extern "C"
{
#endif

  RSHELL_API fs_t *rshell_cmd_fs_new (bool _UseStandardStreams);
  RSHELL_API void rshell_cmd_fs_printf (fs_t *, char *, ...);
  RSHELL_API void rshell_cmd_fs_errprintf (fs_t *, char *, ...);
  RSHELL_API void rshell_cmd_fs_free (fs_t *);

  RSHELL_API cmd_t *rshell_cmd_new (char *n,
                                    cmd_ret_t (*f) (char **, int, fs_t *));
  RSHELL_API cmd_t *rshell_cmd_add (cmd_t *);
  RSHELL_API cmd_t *rshell_cmd_get (char *);

  RSHELL_API void rshell_cmd_init (void);
