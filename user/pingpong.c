#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "stddef.h"

int main(int argc, char *argv[])
{
  int fd_p_s[2];
  int fd_c_s[2];
  pipe(fd_p_s);
  pipe(fd_c_s);
  char str[5];
  if (fork() == 0)
  {
    // child
    close(fd_p_s[1]);
    read(fd_p_s[0], str, 4);
    printf("%d: received %s\n", getpid(), str);
    close(fd_c_s[0]);
    write(fd_c_s[1], "pong", strlen("pong"));
  }
  else
  {
    // parent
    close(fd_p_s[0]);
    write(fd_p_s[1], "ping", strlen("ping"));
    wait(NULL);
    close(fd_c_s[1]);
    read(fd_c_s[0], str, 4);
    printf("%d: received %s\n", getpid(), str);
  }
  exit(0);
}
