#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "stddef.h"

int main(int argc, char *argv[])
{
  int fd_p_s[2];
  int fd_c_s[2];
  int primes[33] = {0};
  for (int i = 0; i < 33; i++)
    primes[i] = i + 2;
  for (int n = 2; n <= 35; n++)
  {
    pipe(fd_p_s);
    pipe(fd_c_s);
    if (fork() == 0)
    {
      // child
      close(fd_p_s[1]);
      read(fd_p_s[0], primes, 4 * 33);
      for (int i = 0; i < 33; i++)
        if (primes[i] % n == 0 && primes[i] != n)
          primes[i] = 0;
      close(fd_c_s[0]);
      write(fd_c_s[1], primes, 4 * 33);
      exit(0);
    }
    else
    {
      // parent
      close(fd_p_s[0]);
      write(fd_p_s[1], primes, 4 * 33);
      wait(NULL);
      close(fd_c_s[1]);
      read(fd_c_s[0], primes, 4 * 33);
    }
  }
  for (int i = 0; i < 33; i++)
    if (primes[i] != 0)
      printf("prime %d\n", primes[i]);
  exit(0);
}
