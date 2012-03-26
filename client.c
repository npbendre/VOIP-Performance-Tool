#include "defs.h"

#define To_nsec(x) ((x) * 1000000)
#define CALC_R (94.2 - 0.024*d - 0.11*(d-117.3)*H*(d-117.3) - 30*log(1.0+e*15.0))
int quit = 0;

struct data {
	double runtime;
	double avg_latency;
	struct timespec ts_recv;
        struct timespec ts_delay;
        struct timespec ts_curr;
        struct timespec ts_init;
	unsigned long num_sent;
	unsigned long num_recv;
	unsigned long loss;
};

void AlarmHandler() { quit = 1; }

void packetRecv(struct data *data, unsigned int size)
{
	static struct timespec ts;
	unsigned long tot;

	/* if different sized data recd */
	(size != PACKET_SIZE) ? ++data->loss : ++data->num_recv;
	tot = data->num_recv + data->loss;

	/* 2 packets recd */
	if (!(tot % 2)) {
		data->avg_latency *= (double) tot - 1;
		data->avg_latency += getTimeDiff(data->ts_recv, ts);
		data->avg_latency /= (double) tot;
	}
	else
		ts = data->ts_recv;
}

int main(int argc, char *argv[])
{
	int sock;
	int flags;
	char *server_ip;
	unsigned short server_port;
	struct sockaddr_in server;
	struct hostent *host;
	struct timeval tv = { 0, 0 };
	char *buf;
	unsigned int recv_size;
	int stop = 0;
	socklen_t size = sizeof(struct sockaddr_in);
	fd_set static_rd, static_wr, rd, wr;
	double d, e;
	int H = 0;
	struct data data = { .avg_latency = 0,
			     .num_sent = 0,
			     .num_recv = 0,
			     .loss = 0 };

	/* Invalid invocation of the program */
	if (argc != 4) {
		printf("Syntax: %s <server> <server port> <runtime> \n", argv[0]);
		exit(1);
	}

	server_ip = argv[1];
	server_port = atoi(argv[2]);
	data.runtime = atof(argv[3]);

	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	memset(&wr, 0, sizeof(fd_set));
	memset(&rd, 0, sizeof(fd_set));
	memset(&static_rd, 0, sizeof(fd_set));
	memset(&static_wr, 0, sizeof(fd_set));
	
	memset(&server, 0, size);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(server_ip);
	server.sin_port = htons(server_port);

	/* If user gave a hostname, we need to resolve it */
	if (server.sin_addr.s_addr == INADDR_NONE) {
		host = gethostbyname(server_ip);
		if (!host) {
			close(sock);
			DieWithError("Invalid hostname specified");
		}
		server.sin_addr.s_addr = *((unsigned long *) host->h_addr_list[0]);
	}

	/* set delay */
	data.ts_delay.tv_sec = 0;
	data.ts_delay.tv_nsec = To_nsec(SAMPLE_INTRVL);

	/* set flags for non blocking recvfrom() */
	flags = fcntl(sock, F_GETFL);
	flags |= O_NONBLOCK;
	fcntl(sock, F_SETFL, flags);
	FD_SET(sock, &static_rd);
	FD_SET(sock, &static_wr);

	/* Allocate data buffer */
	buf = (char *) calloc(PACKET_SIZE, sizeof(char));
	if (!buf) {
		printf("calloc() failed\n");
		close(sock);
		exit(1);
	}

	*buf = DATA_MSG+'0';
	signal(SIGALRM, AlarmHandler);

	/* get program start time */
	clock_gettime(CLOCK_REALTIME, &data.ts_init);
	printf("Start Sending packets...\n");

	for ( ; !quit; ) {

		/* Check if we have exceeded the run time */
		if (!stop) {
			clock_gettime(CLOCK_REALTIME, &data.ts_curr);
			if (getTimeDiff(data.ts_curr, data.ts_init) >= data.runtime) {
				stop = 1;
				*buf = DATA_END+'0';
				sendto(sock, buf, PACKET_SIZE, 0, (struct sockaddr*) &server, size);
				printf("Done sending\n");
				alarm(TIMEOUT/2);
			}
		}

		/* For non blocking recvfrom() */
		rd = static_rd;
		wr = static_wr;
		select(sock+1, &rd, &wr, NULL, &tv);

		/* data arrived */
		if (FD_ISSET(sock, &rd)) {
			recv_size = recvfrom(sock, buf, PACKET_SIZE, 0, (struct sockaddr*) &server, &size);
			clock_gettime(CLOCK_REALTIME, &data.ts_recv);
			packetRecv(&data, recv_size);
			continue;
		}

		if (stop && (data.num_sent <= data.num_recv)) {
			alarm(0);
			break;
		}

		if (!stop) {
			sendto(sock, buf, PACKET_SIZE, 0, (struct sockaddr*) &server, size);
			++data.num_sent;
		}

		nanosleep(&data.ts_delay, NULL);
	}

	data.loss += data.num_sent - data.num_recv;

	d = ((double)data.avg_latency * 1000 / 2) + 85;
	e = ((double)data.loss / data.num_sent) / 2;
	
	printf("R = %f\n", CALC_R);
	
	close(sock);
	free(buf);
	return 0;
}

