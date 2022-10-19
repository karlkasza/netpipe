//tcp.h
//by Scythe

#define MAXMSG 65535

#define MAXQUEUE 256

void tcptrf(int fsock, int ssock);

int tcpout(char *chost, unsigned short cport);

int tcpin(char *listenip, unsigned short sport);

void tcpfwd(char *listenip, unsigned short sport, char *chost,
	unsigned short cport, int argcp, char *argvp[]);
