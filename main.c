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

#include <stdlib.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <err.h>

/**
 * whether or not to print debug messages to stderr
 *   0 : debug off
 *   1 : debug on
 */
#define DEBUG   1

/**
 * whether or not to create virtual TTYs for the multiplex
 *   0 : do not create
 *   1 : create
 */
#define CREATE_NODES    1

/* number of virtual TTYs to create (most modems can handle up to 4) */
#define NUM_NODES   2

/* name of the virtual TTYs to create */
#define BASENAME_NODES  "/dev/ttyGSM"

/* name of the driver, used to get the major number */
#define DRIVER_NAME "gsmtty"

/**
 *   Prints debug messages to stderr if debug is wanted
 */
static void dbg(char *fmt, ...) {

    va_list args;

    if (DEBUG) {
        fflush(NULL);
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(NULL);
    }
    return;
}

/**
 *   Creates nodes for the virtual TTYs
 *   Returns the number of nodes created
 */
int make_nodes(int major, char *basename, int number_nodes) {

    int minor, created = 0;
    dev_t device;
    char node_name[15];
    mode_t oldmask;

    /* set a new mask to get 666 mode and stores the old one */
    oldmask = umask(0);

    for (minor=1; minor<number_nodes+1; minor++) {

        /* append the minor number to the base name */
        sprintf(node_name, "%s%d", basename, minor);

        /* store a device info with major and minor */
        device = makedev(major, minor);

        /* create the actual character node */
        if (mknod(node_name, S_IFCHR | 0666, device) != 0) {
            warn("Cannot create %s", node_name);
        } else {
            created++;
            dbg("Created %s", node_name);
        }

    }

    /* revert the mask to the old one */
    umask(oldmask);

    return created;
}

/**
 *   Removes previously created TTY nodes
 *   Returns nothing, it doesn't really matter if it fails
 */
void remove_nodes(char *basename, int number_nodes) {

    int node;
    char node_name[15];

    for (node=1; node<number_nodes+1; node++) {

        /* append the minor number to the base name */
        sprintf(node_name, "%s%d", basename, node);

        /* unlink the actual character node */
        dbg("Removing %s", node_name);
        if (unlink(node_name) == -1)
            warn("Cannot remove %s", node_name);

    }

    return;
}

/**
*   Gets the major number of the driver device
*   Returns  the major number on success
*           -1 on failure
*/
int get_major(char *driver) {

    FILE *fp;
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    char device[20];
    int major = -1;

    /* open /proc/devices file */
    if ((fp = fopen("/proc/devices", "r")) == NULL)
        err(EXIT_FAILURE, "Cannot open /proc/devices");

    /* read the file line by line */
    while ((major == -1) && (read = getline(&line, &len, fp)) != -1) {

        /* if the driver name string is found in the line, try to get the major */
        if (strstr(line, driver) != NULL) {
            if (sscanf(line,"%d %s\n", &major, device) != 2)
                major = -1;
        }

        /* free the line before getting a new one */
        if (line) {
            free(line);
            line = NULL;
        }

    }

    /* close /proc/devices file */
    fclose(fp);

    return major;
}

int main(int argc, char const *argv[])
{
    int master, slave;
    char name[256];
    int major;

    /* create the virtual TTYs */
    if (CREATE_NODES) {
        int created;
        if ((major = get_major(DRIVER_NAME)) < 0)
            errx(EXIT_FAILURE, "Cannot get major number");
        if ((created = make_nodes(major, BASENAME_NODES, NUM_NODES)) < NUM_NODES)
            warnx("Cannot create all nodes, only %d/%d have been created.", created, NUM_NODES);
    }

    int e = openpty(&master, &slave, &name[0], NULL, NULL);
    if(0 > e)
    {
        printf("Error: %s\n", strerror(errno));
        return -1;
    }

    printf("Slave PTY: %s\n", name);

    int r;

    while((r = read(master, &name[0], sizeof(name)-1)) > 0)
    {
        name[r] = '\0';
        printf("%s", &name[0]);
    }

    close(slave);
    close(master);

    return 0;
}
