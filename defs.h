#include <stdio.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <math.h>

#define SAMPLE_INTRVL 20
#define PACKET_SIZE 160
#define DATA_MSG 1
#define DATA_END 2

#define BUF_LEN 10
#define TIMEOUT 5		/* 5 seconds */

/* error printing and exit function */
void DieWithError(char *);

/* returns time difference in seconds */
double getTimeDiff(struct timespec ts1, struct timespec ts2);

