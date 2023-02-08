#include "systemcalls.h"
/* added for system() */
#include <stdlib.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <sys/stat.h>

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/

    /* check for null cmd */
    if(cmd == NULL)
    {
        printf("\r\ncmd arguement invalid.");
        return false;
    }
    
    /* attempt to create new process with given command */
    int result = system(cmd);

    /* error handling for system call failures */
    if(result == -1)
    {
        printf("\r\nsystem call returned -1, child process could not be created.");
        return false;
    }
    else if(result == 127)
    {
        printf("\r\nsystem call returned 127, child process could not execute shell.");
        return false;
    }
    else
        return true;

    return true;
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/

    /* var to store process status from wait() */
    int wstat   = 0;
    pid_t pid   = 0;
    pid_t wpid  = 0;

    /* flush stdout for readability */
    fflush(stdout);

    /* create new child process */
    pid = fork();
    /* error handling for fork, returns -1 if child can't be created*/
    if(pid == -1)
    {
        perror("fork failed to create child process");
        return false;
    }
    else if(pid == 0)
    {
        /* change newly created process with given commands */
        execv(command[0], command);
        /* error handling for execv */
        perror("execv failed to alter the child");
        exit(-1);
    }
    /* Parent process, fork returns pid of created child */
    else
    {
        /* wait for child to exit */
        wpid = waitpid(pid, &wstat, 0);
        if(wpid == -1)
        {
            perror("waitpid failure");
            return false;
        }
        /* check exit status of child, WIFEXITED indicates successful termination */
        if(WIFEXITED(wstat) == true)
        {
            if(wstat)
                return false;
            else
                return true;
        }
        else
        {
            perror("Child did not terminate normally");
            return false;   
        }
    }
    
    va_end(args);
    return true;
}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    /* taken directly from stackoverflow reference */
    int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0664);
    if(fd == -1)
    {
        perror("failed to open file");
        return false;
    }

    /* var to store process status from wait() */
    int wstat   = 0;
    /* store return from fork and wait */
    pid_t pid   = 0;
    pid_t wpid  = 0;

    /* flush stdout for readability */
    fflush(stdout);

    /* create new child process */
    pid = fork();
    /* error handling for fork, returns -1 if child can't be created */
    if(pid == -1)
    {
        perror("fork() failed to create child process");
        return false;
    }
    else if(pid == 0)
    {
        /* redirect stdout to opened file */
        if(dup2(fd, 1) < 0)
        {
            perror("dup2 failed to redirect output");
            return false;
        }
        close(fd);
        /* change newly created process with given commands */
        execv(command[0], command);
        perror("execv failed to alter child process");
        exit(-1);
    }
    /* Parent process, fork returns pid of created child */
    else
    {
        close(fd);
        /* wait for child to exit */
        wpid = waitpid(pid, &wstat, 0);
        if(wpid == -1)
        {
            perror("waitpid failure");
            return false;
        }
        /* check exit status of child, WIFEXITED indicates successful termination */
        if(WIFEXITED(wstat) == true)
        {
            if(wstat)
                return false;
            else
                return true;
        }
        else
        {
            perror("Child did not terminate normally");
            return false;   
        }
    }

    va_end(args);

    return true;
}

