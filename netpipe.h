//netpipe.h
//by Scythe

#define MYNAME "Netpipe"
#define MYVER "0.1"
#define MYDESC "Universal TCP forwarder"

//-----------------------------------------------------------------
//Because the compiler is crying for an
//inmplicit declaration of asprintf()
//This code is taken from stdio.h
extern int asprintf (char **__restrict __ptr,
		     __const char *__restrict __fmt, ...)
     __THROW __attribute__ ((__format__ (__printf__, 2, 3)));
//-----------------------------------------------------------------
