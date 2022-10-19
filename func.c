//func.c
//by Scythe

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "func.h"

//Function for logging...
void do_log(char *templ_str, ...) {
	va_list msgs;
	char *finder = templ_str;
	int count = 0;

	//Count the number of variables in template
	do {
	finder = strchr(finder, '%');
	if (finder!=NULL)
		if (strlen(finder)!=strlen(templ_str)) {
			finder++;
			if ((finder[0])!='%') count++;
			else
			if (strlen(finder)!=strlen(templ_str)) finder++;
		}
	} while (finder!=NULL);

	va_start(msgs, count);

	if (uselog) {
		time_t curtime = time(0);
		struct tm *loctime;
		char timer[64];
		FILE *logf;

		loctime = localtime(&curtime);
		strftime(timer, 64, "%Y-%m-%d, %H-%M-%S", loctime);

		logf = fopen(logfn, "a");
		if (logf == NULL) {
		    printf("[%i] could not open log file %s: %s!\n", getpid(), logfn, strerror(errno));
		    exit(EXIT_FAILURE);
		}
		else {
			fprintf(logf, "%s [%i] ", timer, getpid());
			vfprintf(logf, templ_str, msgs);
			fprintf(logf, "\n");
			fclose(logf);
		}
	}

	if (opt_fork) {
		printf("[%i] ", getpid());
		vprintf(templ_str, msgs);
		printf("\n");
	}

	va_end(msgs);
}

//Will fork the process into background
//Of course only if -f opt was not set
void do_fork(void) {
	if (!opt_fork) {
		pid_t pid;

		//Fork
		if ((pid = fork()) < 0) {
			do_log("could not fork: %s\n", strerror(errno));
			exit(EXIT_FAILURE);
		}
		//Parent dies
		if (pid > 0) exit(EXIT_SUCCESS);
		//Child goes to hollywood
		setsid();

		do_log("forked to background");
	}
}
