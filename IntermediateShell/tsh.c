/* 
 * tsh - A tiny shell program with job control
 * 
 * Rajarshi Das 34220662
 * Anaika Pathak 34432977
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>

/* Misc manifest constants */
#define MAXLINE    1024   /* max line size */
#define MAXARGS     128   /* max args on a command line */
#define MAXJOBS      16   /* max jobs at any point in time */
#define MAXJID    1<<16   /* max job ID */

/* Job states */
#define UNDEF 0 /* undefined */
#define FG 1    /* running in foreground */
#define BG 2    /* running in background */
#define ST 3    /* stopped */

/* 
 * Jobs states: FG (foreground), BG (background), ST (stopped)
 * Job state transitions and enabling actions:
 *     FG -> ST  : ctrl-z
 *     ST -> FG  : fg command
 *     ST -> BG  : bg command
 *     BG -> FG  : fg command
 * At most 1 job can be in the FG state.
 */

/* Global variables */
extern char **environ;      /* defined in libc */
char prompt[] = "tsh> ";    /* command line prompt (DO NOT CHANGE) */
int verbose = 0;            /* if true, print additional output */
int nextjid = 1;            /* next job ID to allocate */
char sbuf[MAXLINE];         /* for composing sprintf messages */

struct job_t {              /* The job struct */
    pid_t pid;              /* job PID */
    int jid;                /* job ID [1, 2, ...] */
    int state;              /* UNDEF, BG, FG, or ST */
    char cmdline[MAXLINE];  /* command line */
};
struct job_t jobs[MAXJOBS]; /* The job list */
/* End global variables */


/* Function prototypes */

/* Here are the functions that you will implement */
void eval(char *cmdline);
int builtin_cmd(char **argv);
void do_bgfg(char **argv);
void waitfg(pid_t pid);

void sigchld_handler(int sig);
void sigtstp_handler(int sig);
void sigint_handler(int sig);

/* Here are helper routines that we've provided for you */
int parseline(const char *cmdline, char **argv); 
void sigquit_handler(int sig);

void clearjob(struct job_t *job);
void initjobs(struct job_t *jobs);
int maxjid(struct job_t *jobs); 
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline);
int deletejob(struct job_t *jobs, pid_t pid); 
pid_t fgpid(struct job_t *jobs);
struct job_t *getjobpid(struct job_t *jobs, pid_t pid);
struct job_t *getjobjid(struct job_t *jobs, int jid); 
int pid2jid(pid_t pid); 
void listjobs(struct job_t *jobs);

void usage(void);
void unix_error(char *msg);
void app_error(char *msg);
typedef void handler_t(int);
handler_t *Signal(int signum, handler_t *handler);

/*
 * main - The shell's main routine 
 */
int main(int argc, char **argv) 
{
    char c;
    char cmdline[MAXLINE];
    int emit_prompt = 1; /* emit prompt (default) */

    /* Redirect stderr to stdout (so that driver will get all output
     * on the pipe connected to stdout) */
    dup2(1, 2);

    /* Parse the command line */
    while ((c = getopt(argc, argv, "hvp")) != EOF) {
        switch (c) {
        case 'h':             /* print help message */
            usage();
	    break;
        case 'v':             /* emit additional diagnostic info */
            verbose = 1;
	    break;
        case 'p':             /* don't print a prompt */
            emit_prompt = 0;  /* handy for automatic testing */
	    break;
	default:
            usage();
	}
    }

    /* Install the signal handlers */

    /* These are the ones you will need to implement */
    Signal(SIGINT,  sigint_handler);   /* ctrl-c */
    Signal(SIGTSTP, sigtstp_handler);  /* ctrl-z */
    Signal(SIGCHLD, sigchld_handler);  /* Terminated or stopped child */

    /* This one provides a clean way to kill the shell */
    Signal(SIGQUIT, sigquit_handler); 

    /* Initialize the job list */
    initjobs(jobs);

    /* Execute the shell's read/eval loop */
    while (1) {

	/* Read command line */
	if (emit_prompt) {
	    printf("%s", prompt);
	    fflush(stdout);
	}
	if ((fgets(cmdline, MAXLINE, stdin) == NULL) && ferror(stdin))
	    app_error("fgets error");
	if (feof(stdin)) { /* End of file (ctrl-d) */
	    fflush(stdout);
	    exit(0);
	}

	/* Evaluate the command line */
	eval(cmdline);
	fflush(stdout);
	fflush(stdout);
    } 

    exit(0); /* control never reaches here */
}
  
// return index of first '|' or -1 if none 
static int find_pipe(char **argv) {
    for (int i = 0; argv[i]; ++i) if (!strcmp(argv[i], "|")) return i;
    return -1;
}

// detect if any redirection token exists anywhere in argv 
static int has_any_redir(char **argv) {
    for (int i = 0; argv[i]; ++i) {
        if (!strcmp(argv[i], "<") || !strcmp(argv[i], ">") ||
            !strcmp(argv[i], ">>") || !strcmp(argv[i], "2>")) return 1;
    }
    return 0;
}

// strip ONE redirection occurrence in-place; return 1 if stripped, 0 if none, -1 on syntax error 
static int strip_one_redir(char **argv, char **pin, char **pout, char **perr, int *pappend) {
    for (int i = 0; argv[i]; ++i) {
        if (!strcmp(argv[i], "<")) {
            if (!argv[i+1]) { printf("syntax error: < needs a file\n"); return -1; }
            *pin = argv[i+1];
            for (int j = i; argv[j+2]; ++j) argv[j] = argv[j+2];
            argv[i] = NULL;
            return 1;
        }
        if (!strcmp(argv[i], ">") || !strcmp(argv[i], ">>")) {
            if (!argv[i+1]) { printf("syntax error: > / >> need a file\n"); return -1; }
            *pout = argv[i+1];
            *pappend = (argv[i][1] == '>');
            for (int j = i; argv[j+2]; ++j) argv[j] = argv[j+2];
            argv[i] = NULL;
            return 1;
        }
        if (!strcmp(argv[i], "2>")) {
            if (!argv[i+1]) { printf("syntax error: 2> needs a file\n"); return -1; }
            *perr = argv[i+1];
            for (int j = i; argv[j+2]; ++j) argv[j] = argv[j+2];
            argv[i] = NULL;
            return 1;
        }
    }
    return 0;
}

// strip ALL redirs; returns 0 on success, -1 on syntax error 
static int strip_all_redirs(char **argv, char **pin, char **pout, char **perr, int *pappend) {
    *pin = *pout = *perr = NULL; *pappend = 0;
    for (;;) {
        int r = strip_one_redir(argv, pin, pout, perr, pappend);
        if (r == 0) return 0;      
        if (r < 0)  return -1;    
        
    }
}

// apply redirections in the child; return -1 on failure 
static int apply_inline_redirs(char *in, char *out, char *err, int append) {
    int fd;
    if (in) {
        fd = open(in, O_RDONLY);
        if (fd < 0) { printf("%s: cannot open for input\n", in); return -1; }
        dup2(fd, STDIN_FILENO); close(fd);
    }
    if (out) {
        int flags = O_WRONLY | O_CREAT | (append ? O_APPEND : O_TRUNC);
        fd = open(out, flags, 0666);
        if (fd < 0) { printf("%s: cannot open for output\n", out); return -1; }
        dup2(fd, STDOUT_FILENO); close(fd);
    }
    if (err) {
        fd = open(err, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (fd < 0) { printf("%s: cannot open for error\n", err); return -1; }
        dup2(fd, STDERR_FILENO); close(fd);
    }
    return 0;
}

/* 
 * eval - Evaluate the command line that the user has just typed in
 * 
 * If the user has requested a built-in command (quit, jobs, bg or fg)
 * then execute it immediately. Otherwise, fork a child process and
 * run the job in the context of the child. If the job is running in
 * the foreground, wait for it to terminate and then return.  Note:
 * each child process must have a unique process group ID so that our
 * background children don't receive SIGINT (SIGTSTP) from the kernel
 * when we type ctrl-c (ctrl-z) at the keyboard.  
*/
void eval(char *cmdline) 
{
    char *argv[MAXARGS];
    int bg = parseline(cmdline, argv);
    if (argv[0] == NULL) return;           // blank line 

    // Handle built-ins immediately 
    if (builtin_cmd(argv)) return;

    int pidx = find_pipe(argv);            // index of '|' or -1 
    int anyRedir = has_any_redir(argv);    // any of <, >, >>, 2>? 

    // Assignment restriction: don't mix pipes with redirection 
    if (pidx >= 0 && anyRedir) {
        printf("error: mixing pipes with redirection is not supported in this assignment\n");
        return;
    }

    // Pipes/redirections are foreground-only per spec 
    if ((pidx >= 0 || anyRedir) && bg) {
        printf("note: background not supported with pipes/redirection; running in foreground\n");
        bg = 0;
    }

    if (pidx < 0) {
        
        char *in=NULL, *out=NULL, *err=NULL; int append=0;
        if (strip_all_redirs(argv, &in, &out, &err, &append) < 0) return;

        // Block SIGCHLD around fork/addjob  
        sigset_t mask, prev;
        sigemptyset(&mask); sigaddset(&mask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &mask, &prev);

        pid_t pid = fork();
        if (pid < 0) {
            unix_error("fork error");
            sigprocmask(SIG_SETMASK, &prev, NULL);
            return;
        }

        if (pid == 0) {
            // child 
            setpgid(0,0);                                  
            sigprocmask(SIG_SETMASK, &prev, NULL);         

            // Apply <, >, >>, 2> if any 
            if (apply_inline_redirs(in, out, err, append) < 0) exit(0);

            execve(argv[0], argv, environ);
            printf("%s: Command not found.\n", argv[0]);
            exit(0);
        }

        // parent 
        int state = bg ? BG : FG;
        addjob(jobs, pid, state, cmdline);
        sigprocmask(SIG_SETMASK, &prev, NULL);

        if (bg) {
            printf("[%d] (%d) %s", pid2jid(pid), pid, cmdline);
        } else {
            waitfg(pid);
        }
        return;
    }

    // SINGLE PIPE: argv[0..pidx-1] | argv[pidx+1..]
    argv[pidx] = NULL;              
    char **left  = argv;
    char **right = &argv[pidx+1];

    int fd[2];
    if (pipe(fd) < 0) { unix_error("pipe error"); return; }

    pid_t c1 = fork();
    if (c1 < 0) { unix_error("fork error"); close(fd[0]); close(fd[1]); return; }
    if (c1 == 0) {
        // left child: stdout -> pipe write 
        setpgid(0,0);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[0]); close(fd[1]);
        // (restriction) redirections disabled in pipe mode 
        execve(left[0], left, environ);
        printf("%s: Command not found.\n", left[0]);
        exit(0);
    }

    pid_t c2 = fork();
    if (c2 < 0) { unix_error("fork error"); close(fd[0]); close(fd[1]); return; }
    if (c2 == 0) {
        // right child: stdin <- pipe read 
        setpgid(0,0);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]); close(fd[1]);
        execve(right[0], right, environ);
        printf("%s: Command not found.\n", right[0]);
        exit(0);
    }

    // parent: close both ends and wait both (foreground only) 
    close(fd[0]); close(fd[1]);
    int status;
    waitpid(c1, &status, 0);
    waitpid(c2, &status, 0);
}

/* 
 * parseline - Parse the command line and build the argv array.
 * 
 * Characters enclosed in single quotes are treated as a single
 * argument.  Return true if the user has requested a BG job, false if
 * the user has requested a FG job.  
 */
int parseline(const char *cmdline, char **argv) 
{
    static char array[MAXLINE]; /* holds local copy of command line */
    char *buf = array;          /* ptr that traverses command line */
    char *delim;                /* points to first space delimiter */
    int argc;                   /* number of args */
    int bg;                     /* background job? */

    strcpy(buf, cmdline);
    buf[strlen(buf)-1] = ' ';  /* replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    if (*buf == '\'') {
	buf++;
	delim = strchr(buf, '\'');
    }
    else {
	delim = strchr(buf, ' ');
    }

    while (delim) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* ignore spaces */
	       buf++;

	if (*buf == '\'') {
	    buf++;
	    delim = strchr(buf, '\'');
	}
	else {
	    delim = strchr(buf, ' ');
	}
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* ignore blank line */
	return 1;

    /* should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0) {
	argv[--argc] = NULL;
    }
    return bg;
}

/* 
 * builtin_cmd - If the user has typed a built-in command then execute
 *    it immediately.  
 */
int builtin_cmd(char **argv) 
{
    if (argv[0] == NULL) {
        return 1; /* nothing to do */
    }

    if (!strcmp(argv[0], "quit")) {
        exit(0);
    }

    if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs);
        return 1;
    }

    if (!strcmp(argv[0], "bg") || !strcmp(argv[0], "fg")) {
        do_bgfg(argv);
        return 1;
    }

    if (!strcmp(argv[0], "&")) {
        /* parseline normally strips &, but ignore if present */
        return 1;
    }
    return 0;     /* not a builtin command */
}

/* 
 * do_bgfg - Execute the builtin bg and fg commands
 */
void do_bgfg(char **argv) 
{
    /* argv[0] is "bg" or "fg"; argv[1] should be PID or %JID */
    if (argv[1] == NULL) {
        if (!strcmp(argv[0], "bg"))
            printf("bg command requires PID or %%jobid\n");
        else
            printf("fg command requires PID or %%jobid\n");
        return;
    }

    struct job_t *job = NULL;
    pid_t pid = 0;
    int jid = 0;

    /* Is it a %JID? */
    if (argv[1][0] == '%') {
        /* Expect digits after % */
        if (!isdigit((unsigned char)argv[1][1])) {
            /* Not a number after % */
            if (!strcmp(argv[0], "bg"))
                printf("bg: argument must be a PID or %%jobid\n");
            else
                printf("fg: argument must be a PID or %%jobid\n");
            return;
        }
        jid = atoi(&argv[1][1]);
        job = getjobjid(jobs, jid);
        if (job == NULL) {
            /* No such job */
            printf("%s: No such job\n", argv[1]);
            return;
        }
        pid = job->pid;
    } 
    else if (isdigit((unsigned char)argv[1][0])) {
        /* Treat as PID */
        pid = (pid_t)atoi(argv[1]);
        job = getjobpid(jobs, pid);
        if (job == NULL) {
            /* No such process */
            printf("%d: No such process\n", pid);
            return;
        }
        jid = job->jid;
    } 
    else {
        /* Not PID nor %JID */
        if (!strcmp(argv[0], "bg"))
            printf("bg: argument must be a PID or %%jobid\n");
        else
            printf("fg: argument must be a PID or %%jobid\n");
        return;
    }

    /* Send SIGCONT to the job's *process group*/
    if (kill(-pid, SIGCONT) < 0) {
        /* If something odd happened, just return to prompt */
        return;
    }

    if (!strcmp(argv[0], "bg")) {
        job->state = BG;
        /* Print in the exact format the traces expect */
        printf("[%d] (%d) %s", jid, pid, job->cmdline);
    } else { /* "fg" */
        job->state = FG;
        /* Bring to foreground and wait */
        waitfg(pid);
    }
    return;
}

/* 
 * waitfg - Block until process pid is no longer the foreground process
 */
void waitfg(pid_t pid)
{
    /* Busy-wait loop with short sleeps */
    while (fgpid(jobs) == pid) {
        sleep(1);
    }
    return;
}

/*****************
 * Signal handlers
 *****************/

/* 
 * sigchld_handler - The kernel sends a SIGCHLD to the shell whenever
 *     a child job terminates (becomes a zombie), or stops because it
 *     received a SIGSTOP or SIGTSTP signal. The handler reaps all
 *     available zombie children, but doesn't wait for any other
 *     currently running children to terminate.  
 */
void sigchld_handler(int sig) 
{
    int olderrno = errno;      /* save, then restore errno */
    int status;
    pid_t pid;

    /* Reap all children that have changed state */
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {

        /* If child terminated normally: remove job */
        if (WIFEXITED(status)) {
            deletejob(jobs, pid);
        }
        /* If child was terminated by a signal: print and remove */
        else if (WIFSIGNALED(status)) {
            int sig_num = WTERMSIG(status);
            printf("Job (%d) terminated by signal %d\n", pid, sig_num);
            deletejob(jobs, pid);
        }
        /* If child was stopped by a signal: print and mark ST */
        else if (WIFSTOPPED(status)) {
            int sig_num = WSTOPSIG(status);
            struct job_t *job = getjobpid(jobs, pid);
            if (job) job->state = ST;
            printf("Job (%d) stopped by signal %d\n", pid, sig_num);
        }
    }

    /* If waitpid was interrupted by a signal, that's fine. 
       For any other error (pid == -1) we just return. */
    errno = olderrno;
    return;
}

/* 
 * sigint_handler - The kernel sends a SIGINT to the shell whenver the
 *    user types ctrl-c at the keyboard.  Catch it and send it along
 *    to the foreground job.  
 */
void sigint_handler(int sig) 
{
    int olderrno = errno;
    pid_t pid = fgpid(jobs);
    if (pid != 0) {
        /* Send to the *process group* (note the minus sign) */
        kill(-pid, SIGINT);
    }
    errno = olderrno;
    return;
}

/*
 * sigtstp_handler - The kernel sends a SIGTSTP to the shell whenever
 *     the user types ctrl-z at the keyboard. Catch it and suspend the
 *     foreground job by sending it a SIGTSTP.  
 */
void sigtstp_handler(int sig) 
{
    int olderrno = errno;
    pid_t pid = fgpid(jobs);
    if (pid != 0) {
        /* Send to the *process group* (note the minus sign) */
        kill(-pid, SIGTSTP);
    }
    errno = olderrno;
    return;
}

/*********************
 * End signal handlers
 *********************/

/***********************************************
 * Helper routines that manipulate the job list
 **********************************************/

/* clearjob - Clear the entries in a job struct */
void clearjob(struct job_t *job) {
    job->pid = 0;
    job->jid = 0;
    job->state = UNDEF;
    job->cmdline[0] = '\0';
}

/* initjobs - Initialize the job list */
void initjobs(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	clearjob(&jobs[i]);
}

/* maxjid - Returns largest allocated job ID */
int maxjid(struct job_t *jobs) 
{
    int i, max=0;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid > max)
	    max = jobs[i].jid;
    return max;
}

/* addjob - Add a job to the job list */
int addjob(struct job_t *jobs, pid_t pid, int state, char *cmdline) 
{
    int i;
    
    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == 0) {
	    jobs[i].pid = pid;
	    jobs[i].state = state;
	    jobs[i].jid = nextjid++;
	    if (nextjid > MAXJOBS)
		nextjid = 1;
	    strcpy(jobs[i].cmdline, cmdline);
  	    if(verbose){
	        printf("Added job [%d] %d %s\n", jobs[i].jid, jobs[i].pid, jobs[i].cmdline);
            }
            return 1;
	}
    }
    printf("Tried to create too many jobs\n");
    return 0;
}

/* deletejob - Delete a job whose PID=pid from the job list */
int deletejob(struct job_t *jobs, pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;

    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid == pid) {
	    clearjob(&jobs[i]);
	    nextjid = maxjid(jobs)+1;
	    return 1;
	}
    }
    return 0;
}

/* fgpid - Return PID of current foreground job, 0 if no such job */
pid_t fgpid(struct job_t *jobs) {
    int i;

    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].state == FG)
	    return jobs[i].pid;
    return 0;
}

/* getjobpid  - Find a job (by PID) on the job list */
struct job_t *getjobpid(struct job_t *jobs, pid_t pid) {
    int i;

    if (pid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid)
	    return &jobs[i];
    return NULL;
}

/* getjobjid  - Find a job (by JID) on the job list */
struct job_t *getjobjid(struct job_t *jobs, int jid) 
{
    int i;

    if (jid < 1)
	return NULL;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].jid == jid)
	    return &jobs[i];
    return NULL;
}

/* pid2jid - Map process ID to job ID */
int pid2jid(pid_t pid) 
{
    int i;

    if (pid < 1)
	return 0;
    for (i = 0; i < MAXJOBS; i++)
	if (jobs[i].pid == pid) {
            return jobs[i].jid;
        }
    return 0;
}

/* listjobs - Print the job list */
void listjobs(struct job_t *jobs) 
{
    int i;
    
    for (i = 0; i < MAXJOBS; i++) {
	if (jobs[i].pid != 0) {
	    printf("[%d] (%d) ", jobs[i].jid, jobs[i].pid);
	    switch (jobs[i].state) {
		case BG: 
		    printf("Running ");
		    break;
		case FG: 
		    printf("Foreground ");
		    break;
		case ST: 
		    printf("Stopped ");
		    break;
	    default:
		    printf("listjobs: Internal error: job[%d].state=%d ", 
			   i, jobs[i].state);
	    }
	    printf("%s", jobs[i].cmdline);
	}
    }
}
/******************************
 * end job list helper routines
 ******************************/


/***********************
 * Other helper routines
 ***********************/

/*
 * usage - print a help message
 */
void usage(void) 
{
    printf("Usage: shell [-hvp]\n");
    printf("   -h   print this message\n");
    printf("   -v   print additional diagnostic information\n");
    printf("   -p   do not emit a command prompt\n");
    exit(1);
}

/*
 * unix_error - unix-style error routine
 */
void unix_error(char *msg)
{
    fprintf(stdout, "%s: %s\n", msg, strerror(errno));
    exit(1);
}

/*
 * app_error - application-style error routine
 */
void app_error(char *msg)
{
    fprintf(stdout, "%s\n", msg);
    exit(1);
}

/*
 * Signal - wrapper for the sigaction function
 */
handler_t *Signal(int signum, handler_t *handler) 
{
    struct sigaction action, old_action;

    action.sa_handler = handler;  
    sigemptyset(&action.sa_mask); /* block sigs of type being handled */
    action.sa_flags = SA_RESTART; /* restart syscalls if possible */

    if (sigaction(signum, &action, &old_action) < 0)
	unix_error("Signal error");
    return (old_action.sa_handler);
}

/*
 * sigquit_handler - The driver program can gracefully terminate the
 *    child shell by sending it a SIGQUIT signal.
 */
void sigquit_handler(int sig) 
{
    printf("Terminating after receipt of SIGQUIT signal\n");
    exit(1);
}



