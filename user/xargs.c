#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDDER_FILENO 2
#define MAXLINE 1024

int main(int argc, char *argv[])
{
  char buf[MAXLINE];
  char *argv_child[MAXARG];
  int argc_child = 0;

  char *cmd = argv[1];
  for (int i = 1; i < argc; i++)
    argv_child[argc_child++] = argv[i];
  int len = 0;
  while ((len = read(STDIN_FILENO, buf, MAXLINE)) > 0)
  {
    if (fork() == 0)
    {
      char *arg = (char *)malloc(sizeof(buf));
      int index_char = 0;
      for (int i = 0; i < len; i++)
      {
        if (buf[i] == ' ' || buf[i] == '\n' || i == len - 1)
        {

          arg[index_char] = '\0';
          argv_child[argc_child++] = arg;
          index_char = 0;
          arg = (char *)malloc(sizeof(buf));
        }
        else
        {
          arg[index_char++] = buf[i];
        }
      }
      argv_child[argc_child] = 0;
      exec(cmd, argv_child);
    }
    else
    {
      wait(0);
    }
  }
  return 0;
}
