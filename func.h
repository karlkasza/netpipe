//func.h
//by Scythe

extern char *logfn;
extern short uselog, opt_fork;

void do_log(char *templ_str, ...);

void do_fork(void);
