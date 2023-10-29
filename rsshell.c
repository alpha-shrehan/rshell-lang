#include "rsshell.h"

RSHELL_API char *
rshell_file_read (char *_FName)
{
  FILE *f = fopen (_FName, "r");

  char c;
  char *s = NULL;
  size_t sc = 0;

  while ((c = fgetc (f)) != EOF)
    {
      s = RSHELL_Realloc (s, (sc + 1) * sizeof (char));
      s[sc++] = c;
    }

  s = RSHELL_Realloc (s, (sc + 1) * sizeof (char));
  s[sc++] = '\0';

  return s;
}

RSHELL_API char **
rshell_str_split_delim (char *_Data, size_t *_Rp, char delim)
{
  char **res = RSHELL_Malloc (sizeof (char *));
  size_t rc = 0, i = 0, sl = strlen (_Data), buf = 0;

  *res = NULL;
  bool in_str = false;
  int gb = 0;

  while (i < sl)
    {
      char c = _Data[i];

      /*
        Instead of considering reallocations everytime,
        allocate maximum memory size (time over space)
      */
      res[rc] = RSHELL_Realloc (res[rc], sl * sizeof (char));
      res[rc][buf++] = c;

      if (c == '\"' || c == '\'' || c == '`')
        {
          if (i)
            {
              if (_Data[i - 1] != '\\')
                in_str = !in_str;
              else
                {
                  if (i > 1 && _Data[i - 2] == '\\')
                    in_str = !in_str;
                }
            }
          else
            in_str = !in_str;
        }

      if (!in_str)
        {
          if (c == '(' || c == '{' || c == '[')
            gb++;

          if (c == ')' || c == '}' || c == ']')
            gb--;

          if (c == delim && !gb)
            {
              res[rc][buf - 1] = '\0';
              buf = 0;

              if (i != sl - 1)
                {
                  res = RSHELL_Realloc (res, (rc + 2) * sizeof (char *));
                  res[++rc] = NULL;
                }
            }
        }

    end:
      i++;
    }

  if (buf)
    res[rc][buf++] = '\0';

  if (_Rp != NULL)
    *_Rp = rc + 1;

  return res;
}

RSHELL_API char *
rshell_trim (char *s)
{
  while (*s != '\0' && strstr ("\n\t \r", (char *)(char[]){ *s, '\0' }))
    {
      s++;
    }

  size_t dl = strlen (s);
  while (dl && strstr ("\n\t \r", (char *)(char[]){ s[dl - 1], '\0' }))
    {
      dl--;
    }

  s[dl] = '\0';
  return RSHELL_Strdup (s);
}

RSHELL_API char *
rshell_unescape_str (const char *input)
{
  if (input == NULL)
    {
      return NULL;
    }

  int il = strlen (input);
  char *res = (char *)RSHELL_Malloc (il + 1);

  int ri = 0;
  for (int i = 0; i < il; i++)
    {
      if (input[i] == '\\')
        {
          i++; // Move past the backslash

          // Handle escape sequences
          switch (input[i])
            {
            case 'n':
              res[ri++] = '\n';
              break;
            case 't':
              res[ri++] = '\t';
              break;
            case '\\':
              res[ri++] = '\\';
              break;
            // Add more escape sequences as needed
            default:
              // If it's not a recognized escape sequence, copy the backslash
              // and the character as is
              res[ri++] = '\\';
              res[ri++] = input[i];
            }
        }
      else
        {
          // Copy non-escape characters as is
          res[ri++] = input[i];
        }
    }

  // Null-terminate the result string
  res[ri] = '\0';

  return res;
}