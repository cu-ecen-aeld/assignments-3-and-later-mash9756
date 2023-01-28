/**
 *  @file       writer.c
 *  @author     Mark Sherman
 *  @version    1.0
 *  @date       01/27/2023
 * 
 *  @brief      writer.sh implemented in C
*/

#include <stdio.h>
#include <syslog.h>
#include <string.h>

/**
 * @name    usage_info
 * @param   argv    arguments passed to program from shell
 * 
 * @brief   prints usage information for the program
 * 
 * @return  VOID
*/
void usage_info(char *argv[])
{
    printf("\r\n|-------------------------------USAGE INFO------------------------------|");
    printf("\r\n|Program Name: writer.exe\t\t\t\t\t\t|");
    printf("\r\n|\tWrites given text string to given file in RELATIVE folder path\t|");
    printf("\r\n|\tExample: ./writer.exe conf/test.txt test\t\t\t|");
    printf("\r\n|\t\twrite \"test\" to file test.txt in folder \"conf\"\t\t|\n");
    printf("|-----------------------------------------------------------------------|\n\n");
}

int main(int argc, char *argv[])
{
    /* Setup syslog */
    openlog(NULL, 0, LOG_USER);

    if(argc < 3)
    {
        printf("\r\nNo arguements passed.\r\n");
        usage_info(argv);
        syslog(LOG_ERR, "No arguements passed.");
        return 1;
    }
    else if(argc >= 4)
    {
        printf("\r\nToo many arguements passed.\r\n");
        usage_info(argv);
        syslog(LOG_ERR, "Too many arguements passed.");
        return 1;
    }

    /* setup file IO */
    FILE *file;
    char *write_file    = argv[1];
    char *write_string  = argv[2];

    printf("\r\n\nWrite File: %s", write_file);
    printf("\r\nWrite String: %s\r\n", write_string);
    
    /* open given file path with read + write capability */
    file = fopen(write_file, "w");
    if(file == NULL)
    {
        printf("\r\nUnable to open file.");
        syslog(LOG_ERR, "Unable to open file.");
        printf("\r\nExiting...\r\n");
        return 1;
    }
    else
    {
        printf("\r\nSuccessfully opened file %s!", write_file);
        syslog(LOG_DEBUG, "Successfully opened user file.");
    }
    
    syslog(LOG_DEBUG, "Writing %s to %s...", write_file, write_string);
    fwrite(write_string, strlen(write_string) + 1, 1, file);
    syslog(LOG_DEBUG, "Write complete!");
    printf("\r\nWrite Successful!\n\n");

    /* close file and syslog */
    fclose(file);
    closelog();

    return 0;
}