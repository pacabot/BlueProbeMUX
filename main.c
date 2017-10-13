/*
 * main.c
 *
 *  Created on: Oct 13, 2017
 *      Author: colin
 */

#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <pty.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char const *argv[])
{
  int master, slave;
  char name[256];

  int e = openpty(&master, &slave, &name[0], NULL, NULL);
  if(0 > e) {
    printf("Error: %s\n", strerror(errno));
    return -1;
  }

  printf("Slave PTY: %s\n", name);

  int r;

  while((r = read(master, &name[0], sizeof(name)-1)) > 0) {
    name[r] = '\0';
    printf("%s", &name[0]);
  }

  close(slave);
  close(master);

  return 0;
}
