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
#include <termios.h>
#include <unistd.h>

int set_interface_attribs (int fd, int speed, int parity)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		fprintf (stderr, "error %d from tcgetattr", errno);
		return -1;
	}

	cfsetospeed (&tty, speed);
	cfsetispeed (&tty, speed);

	tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
	// disable IGNBRK for mismatched speed tests; otherwise receive break
	// as \000 chars
	tty.c_iflag &= ~IGNBRK;         // disable break processing
	tty.c_lflag = 0;                // no signaling chars, no echo,
									// no canonical processing
	tty.c_oflag = 0;                // no remapping, no delays
	tty.c_cc[VMIN]  = 0;            // read doesn't block
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

	tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
									// enable reading
	tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
	tty.c_cflag |= parity;
	tty.c_cflag &= ~CSTOPB;
	tty.c_cflag &= ~CRTSCTS;

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
	{
		fprintf (stderr, "error %d from tcsetattr", errno);
		return -1;
	}
	return 0;
}

void set_blocking (int fd, int should_block)
{
	struct termios tty;
	memset (&tty, 0, sizeof tty);
	if (tcgetattr (fd, &tty) != 0)
	{
		fprintf (stderr, "error %d from tggetattr", errno);
		return;
	}

	tty.c_cc[VMIN]  = should_block ? 1 : 0;
	tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

	if (tcsetattr (fd, TCSANOW, &tty) != 0)
		fprintf (stderr, "error %d setting term attributes", errno);
}

int main(int argc, char const *argv[])
{
	/* PTY init*/
	int master1, slave1;
	int master2, slave2;
	char name1[256];
	char name2[256];

	int e1 = openpty(&master1, &slave1, &name1[0], NULL, NULL);
	if(0 > e1)
	{
		printf("Error: %s\n", strerror(errno));
		return -1;
	}

	int e2 = openpty(&master2, &slave2, &name2[0], NULL, NULL);
	if(0 > e2)
	{
		printf("Error: %s\n", strerror(errno));
		return -1;
	}

	printf("Slave PTY1: %s\n", name1);
	printf("Slave PTY2: %s\n", name2);

	/*TTY*/
	int n;
	char *portname = "/dev/ttyUSB0";
	int fd = open (portname, O_RDWR | O_NOCTTY | O_SYNC);
	if (fd < 0)
	{
		fprintf (stderr, "error %d opening %s: %s", errno, portname, strerror (errno));
		return -1;
	}

	set_interface_attribs (fd, B115200, 0);  // set speed to 115,200 bps, 8n1 (no parity)
	set_blocking (fd, 0);                // set no blocking
	sleep (3);
	write (fd, "hello!\n", 7);           // send 7 character greeting

	usleep ((7 + 25) * 100);             // sleep enough to transmit the 7 plus
	                                     // receive 25:  approx 100 uS per char transmit
	char buf [100];
	while(1)
	{
		while ((n = read (fd, buf, (sizeof(buf) - 1)))<=0) // read up to 100 characters if ready to read
		{
			usleep(1000);
		}
		buf[n] = '\0';
		write(master1, buf, strlen(buf));
		write(master2, buf, strlen(buf));
		printf("recived : %s", buf);
	}
	close(slave1);
	close(master1);
	close(slave2);
	close(master2);

	return 0;

	/* read a PTY*/

//	strcpy(name,"coucou tout le monde\n");
//	while(1)
//	{
//		write(master, name, strlen(name));
//		printf("%s", &name[0]);
//		sleep(1);
//	}


//  return 0;
}
