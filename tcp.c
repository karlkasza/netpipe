//tcp.c
//by Scythe

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>

//netpipe.h only included for asprintf()
#include "netpipe.h"
#include "tcp.h"
#include "func.h"

//Simple TCP data forwarding function
void tcptrf(int fsock, int ssock) {
	int recvd, sent;
	char msg[MAXMSG];
	fd_set readfds;
	int numfds;
	int s_read, s_write;

	do_log("starting transfer");
	do {
		if (fsock > ssock) numfds = fsock;
		else numfds = ssock;
		numfds++;

		FD_ZERO(&readfds);
		FD_SET(ssock, &readfds);
		FD_SET(fsock, &readfds);
		select(numfds, &readfds, NULL, NULL, NULL);

		if (FD_ISSET(ssock, &readfds)) {
			s_read = ssock;
			s_write = fsock;
		}
		else {
			if (FD_ISSET(fsock, &readfds)) {
				s_read = fsock;
				s_write = ssock;
			}
			else {
				shutdown(ssock, 2);
				shutdown(fsock, 2);
				do_log("unknown transfer error");
				return;
			}
		}
		recvd = recv(s_read, msg, MAXMSG, 0);
		if (recvd > 0) {
			sent = send(s_write, msg, recvd, 0);
		}
	} while ((recvd > 0) && (sent > 0));

	shutdown(ssock, 2);
	shutdown(fsock, 2);
	do_log("stopping transfer");
}

//Function, that will open outgoing connection
int tcpout(char *chost, unsigned short cport) {
	struct sockaddr_in serverinfo;
	struct hostent *hostinfo;
	int csock;

	csock = socket(PF_INET, SOCK_STREAM, 0);
	if (csock < 0) {
		do_log("socket error: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	serverinfo.sin_family = AF_INET;
	serverinfo.sin_port = htons(cport);
	hostinfo = gethostbyname(chost);

	if (hostinfo == NULL) {
		do_log("gethostbyname error: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	serverinfo.sin_addr = *(struct in_addr *) hostinfo->h_addr;

	if (connect(csock, (struct sockaddr *) &serverinfo, sizeof (serverinfo)) != 0) {
		return(errno*-1);
	}

	do_log("connected to %s:%i", chost, cport);
	return(csock);
}

//Function, that waits for 1 incoming connection
int tcpin(char *listenip, unsigned short sport) {
	int lsock, csock;
	struct sockaddr_in laddr, caddr;
	socklen_t len;

	if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		do_log("socket error: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	len = sizeof(laddr);

	if (strcmp(listenip, "*") == 0) laddr.sin_addr.s_addr = htonl(INADDR_ANY);
	else {
		laddr.sin_addr.s_addr = inet_addr(listenip);
		if (laddr.sin_addr.s_addr == INADDR_NONE) {
			do_log("illegal listenIP");
			exit(EXIT_FAILURE);
		}
	}

	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(sport);

	if ((bind(lsock, (const struct sockaddr *) &laddr, len))) {
		do_log("cannot bind socket to port %s:%i: %s",
				listenip, sport, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (listen(lsock, 1)) {
		do_log("cannot listen on port %s:%i: %s",
				listenip, sport, strerror(errno));
		exit(EXIT_FAILURE);
	}

	len = sizeof(caddr);
	if ((csock = accept(lsock, (struct sockaddr *) &caddr, &len)) < 0) {
		do_log("accept error: %s", strerror(errno));
		shutdown(lsock, 2);
		shutdown(csock, 2);
		exit(EXIT_FAILURE);
	}
	shutdown(lsock, 2);
	
	if (uselog) {
		char ip[16] = "unknown";
		size_t size = sizeof(caddr);

		if (getpeername(csock, (struct sockaddr *) &caddr, &size))
			do_log("getpeername error: %s", strerror(errno));
		else strcpy(ip, inet_ntoa(caddr.sin_addr));

		do_log("connection from %s to %s:%i", ip, listenip, sport);
	}

	return(csock);
}

//Function for port-forwarding, spawning cild processes
//- mostly taken from tcpin()
void tcpfwd(char *listenip, unsigned short sport, char *chost,
	unsigned short cport, int argcp, char *argvp[]) {
	int lsock, csock;
	struct sockaddr_in laddr, caddr;
	socklen_t len;

	//Signal handler for SIGCHLD
	void childdone(int signum) {
		int status;
		pid_t child;

		child = waitpid(WAIT_ANY, &status, WNOHANG);
		do_log("connection %i closed", child);
	}

	if ((lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		do_log("socket error: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}

	len = sizeof(laddr);

	if (strcmp(listenip, "*") == 0) laddr.sin_addr.s_addr = htonl(INADDR_ANY);
	else {
		laddr.sin_addr.s_addr = inet_addr(listenip);
		if (laddr.sin_addr.s_addr == INADDR_NONE) {
			do_log("illegal listenIP");
			exit(EXIT_FAILURE);
		}
	}

	laddr.sin_family = AF_INET;
	laddr.sin_port = htons(sport);

	if ((bind(lsock, (const struct sockaddr *) &laddr, len))) {
		do_log("cannot bind socket to port %s:%i: %s",
				listenip, sport, strerror(errno));
		exit(EXIT_FAILURE);
	}

	//Note that listen()-s 2nd parameter changed since tcpin()
	if (listen(lsock, MAXQUEUE)) {
		do_log("cannot listen on port %s:%i: %s",
				listenip, sport, strerror(errno));
		exit(EXIT_FAILURE);
	}

	len = sizeof(caddr);

	//Create a signal handler for finished children
	signal(SIGCHLD, childdone);

	//And the fun begins
	do {

	if ((csock = accept(lsock, (struct sockaddr *) &caddr, &len)) < 0) {
		do_log("accept error: %s", strerror(errno));
		shutdown(lsock, 2);
		shutdown(csock, 2);
		exit(EXIT_FAILURE);
	}
	else {
		pid_t pid;

		//Fork a child
		if ((pid = fork()) < 0) {
			printf("Could not fork: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		//The child does this, the parent just passes by,
		//then returns to accept loop
		if (pid == 0) {
			int fsock = 0;
			char ip[16] = "unknown";
			size_t size = sizeof(caddr);

			//Do the usual
			if (getpeername(csock, (struct sockaddr *) &caddr, &size))
				do_log("getpeername error: %s", strerror(errno));
			else strcpy(ip, inet_ntoa(caddr.sin_addr));

			do_log("connection from %s, forwarding...", ip);

			//Change process name by modifying argv[0] - *argp here
			//There must be a cleaner way to modify process name
			{
			unsigned int i, maxlen = 0;
			char *tmpmsg;

			//Maximum procname length
			// +1 for spaces after parameters
			for (i = 0; i < argcp; i++) maxlen+=strlen(argvp[i])+1;
			// -1 for last parameter didn't have a space after it
			maxlen--;

			// 0 out parameters
			for (i = 0; i < argcp; i++) strncpy(argvp[i], "\0", strlen(argvp[i]));

			//Write whatever we want into argv[0]
			asprintf(&tmpmsg, "netpipe: %s>%s:%i", ip, chost, cport);
			//strncpy() will only copy maxlen chars
			strncpy(argvp[0], tmpmsg, maxlen);

			free(tmpmsg);
			}

			//Start the actual forward
			fsock = tcpout(chost, cport);
			if (fsock > 0) {
				tcptrf(fsock, csock);
			}
			shutdown(csock, 2);
			shutdown(fsock, 2);

			//The child dies here...
			exit(EXIT_SUCCESS);
		}
	}
	} while(1);
}
