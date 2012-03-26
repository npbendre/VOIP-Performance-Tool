#include "defs.h"

int quit = 0;

/* SIGINT handler */
void CNT_C_Code()
{ 
	printf("CNT-C Interrupt, exiting...\n");
	quit = 1; 
}

/* SIGALRM handler */
void AlarmHandler() { }

int main(int argc, char *argv[])
{
	struct sockaddr_in server;
	struct sockaddr_in client;
	socklen_t size = sizeof(struct sockaddr_in);
	struct sigaction sa_int;
	struct sigaction sa_alrm;
	int first_flag = 0;
	unsigned int recv_size;
	char *buf;
	int sock;
	int done_flg = 0;

	/* Invalid invocation of the program */
	if (argc != 2) {
		printf("Syntax: %s <server Port>\n", argv[0]);
		exit(1);
	}

	if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
		DieWithError("socket() failed");

	memset(&server, 0, size);
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	server.sin_port = htons(atoi(argv[1]));

	if (bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
		close(sock);
		DieWithError("bind() failed");
	}

	/* assign handler and initialize set to all signals */
	sa_int.sa_handler = CNT_C_Code;
	sa_alrm.sa_handler = AlarmHandler;
	if (sigfillset(&sa_int.sa_mask) < 0 || sigfillset(&sa_alrm.sa_mask) < 0) {
        	close(sock);
		DieWithError("sigfillset() failed");
	}

	/* set the handler */
	sa_int.sa_flags = sa_alrm.sa_flags = 0;	
        if (sigaction(SIGINT, &sa_int, 0) < 0 || sigaction(SIGALRM, &sa_alrm, 0) < 0) {
        	close(sock);
		DieWithError("sigaction() failed");
	}

	printf("Waiting for client...\n\n");

	/* Allocate buffer */
	buf = (char *) calloc(PACKET_SIZE, sizeof(char));
	if (!buf) {
		close(sock);
		DieWithError("calloc() failed");
	}

	printf("Echoing client's packets...\n");
	for ( ; !quit; ) {
		if (first_flag) alarm(TIMEOUT);

		/* get messages from client */
		recv_size = recvfrom(sock, buf, PACKET_SIZE, 0, (struct sockaddr *) &client, &size);
		if (errno == EINTR) {
			if (!quit) done_flg = 1;
			break;
		}
		if (first_flag) alarm(0);
		
		if (!first_flag) first_flag = 1;
		if (*buf == DATA_END+'0') break;

		/* echo back the message */
		sendto(sock, buf, recv_size, 0, (struct sockaddr*) &client, size);		
	}

	printf("Done...\n");

	free(buf);
	close(sock);
	return 0;
}
