//netpipe.c
//by Scythe

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "netpipe.h"
#include "func.h"
#include "tcp.h"

char *logfn;
short uselog, opt_fork;

int main(int argc, char *argv[]) {
	int sock1, sock2;
	char *host1, *host2;
	unsigned short port1, port2;
	short mode = 0;

	//When incorrect parameters used
	void usage(char *msg) {
		printf("%s\n", msg);
		printf("Usage: %s mode parameters [options] ...\n", argv[0]);
		printf("Use %s --help for more information\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	//Help text
	void showhelp(void) {
		printf("%s %s - %s\n\n", MYNAME, MYVER, MYDESC);
		printf("Usage: %s mode parameters [options] ...\n", argv[0]);
		printf("Modes/(parameters):\n");
		printf("\ti2i\t\tIncoming-to-Incoming connection/\n");
		printf("\t\t\t(ListenIP1:SPort1:ListenIP2:SPort2)\n");
		printf("\t\t\t- Use 0 (0.0.0.0) for unspecified ListenIP!\n");
		printf("\to2o\t\tOutgoing-to-Outgoing connection/\n");
		printf("\t\t\t(DHost1:DPort1:DHost2:DPort2)\n");
		printf("\ti2o\t\tIncoming-to-Outgoing connection/ (a.k.a. PortForward)\n");
		printf("\t\t\t(ListenIP:SPort:DHost:DPort)\n");
		printf("Possible options:\n");
		printf("\t-l/--log file\tLog into file\n");
		printf("\t-f/--fork\tDo not fork into background\n");
		printf("\t-h/--help\tShow this help\n");
		printf("\t-v/--version\tDisplay version information\n");
		printf("For example:\t%s i2i 0:2300:192.168.1.1:2301 -l whatever\n", argv[0]);
		printf("The other side:\t%s o2o 10.0.0.1:2300:127.0.0.1:23 -f\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	//Version information
	void showver(void) {
		printf("%s %s - %s\n", MYNAME, MYVER, MYDESC);
		exit(EXIT_SUCCESS);
	}

	//Signal handling for graceful exit
	void exitsig(int signum) {
		do_log("signal caught, exit");
		exit(EXIT_SUCCESS);
	}

//Main() really starts here	
	if (argc == 1 ) usage("Too few arguments!");

	if (argc == 2) {
		if ((!strcasecmp(argv[1], "-h")) ||
			(!strcasecmp(argv[1], "--help"))) showhelp();

		if ((!strcasecmp(argv[1], "-v")) ||
			(!strcasecmp(argv[1], "--version"))) showver();

		usage("Too few arguments!");
	}

	//Mode
	if (!strcasecmp(argv[1],"i2i")) mode = 1;
	if (!strcasecmp(argv[1],"o2o")) mode = 2;
	if (!strcasecmp(argv[1],"i2o")) mode = 3;
	if (!mode) usage("Unknown mode!");

	//Parameters for modes
	{
	char *tmpcpy;
	char *token;

	//Copy the argument for strtok()
	asprintf(&tmpcpy, argv[2]);

	token = strtok(tmpcpy, ":");
	if (token==NULL) usage("Malformed parameters - see first field!");
	asprintf(&host1, token);

	token = strtok(NULL, ":");
	if (token==NULL) usage("Malformed parameters - see second field!");
	port1 = (unsigned short) strtoul(token, 0, 0);
	if ((port1 <= 0) || (port1 > 65535)) usage("Bad port - second field!");

	token = strtok(NULL, ":");
	if (token==NULL) usage("Malformed parameters - see third field!");
	asprintf(&host2, token);

	token = strtok(NULL, ":");
	if (token==NULL) usage("Malformed parameters - see fourth field!");
	port2 = (unsigned short) strtoul(token, 0, 0);
	if ((port2 <= 0) || (port2 > 65535)) usage("Bad port - fourth field!");

	free(tmpcpy);
	}

	//Parsing additional options
	{
	int i, done;

	//These two are defined in func.h header, so they don't have defaults
	uselog = 0;
	opt_fork = 0;

	for (i=3; i < argc; i++) {
		done = 0;

		if ((!strcasecmp(argv[i], "-f")) ||
			(!strcasecmp(argv[i], "--fork"))) {
		opt_fork++; done++;
		}

		if ((!strcasecmp(argv[i], "-l")) ||
			(!strcasecmp(argv[i], "--log"))) {
			if (i<argc-1) {
				//I know this isn't nice, but i AM bad ;-)
				i++; //Don't try this home, kids!

				//Copy the pointer, because argv[] may change
				asprintf(&logfn, "%s", argv[i]);

				uselog++;
				done++;
			}
			else usage("No log file!");
		}

		if ((!strcasecmp(argv[i], "-h")) ||
			(!strcasecmp(argv[i], "--help"))) showhelp();

		if ((!strcasecmp(argv[i], "-v")) ||
			(!strcasecmp(argv[i], "--version"))) showver();

		if (!done) usage("Unknown option(s)!");
	}
	}

	//Use our own signal handler
	signal(SIGINT, exitsig);
	signal(SIGTERM, exitsig);

	//Start doing the thing we came here for...
	do_log("Netpipe start");

//Choose the mode
	//I2I
	if (mode == 1) {
	do_log("I2I: %s:%i>-<%s:%i", host1, port1, host2, port2);

	printf("Waiting for connection on %s:%i...\n", host1, port1);
	sock1 = tcpin(host1, port1);
	printf("Incoming connection, now waiting on %s:%i...\n", host2, port2);
	sock2 = tcpin(host2, port2);

	//Go BG
	do_fork();

	tcptrf(sock1, sock2);

	do_log("I2I done");
	}
	//O2O
	if (mode == 2) {
	do_log("O2O: %s:%i<->%s:%i", host1, port1, host2, port2);

	//Backgrounding...
	do_fork();

	sock1 = tcpout(host1, port1);
	if (sock1 > 0) {
		sock2 = tcpout(host2, port2);
		if (sock2 > 0) {
			tcptrf(sock1, sock2);
		}
	}

	do_log("O2O done");
	}

	//I2O
	if (mode == 3) {

	do_log("I2O: %s:%i>->%s:%i", host1, port1, host2, port2);

	//Backgrounding...
	do_fork();
	tcpfwd(host1, port1, host2, port2, argc, argv);

	do_log("I2O done");
	}

	return(EXIT_SUCCESS);
}
