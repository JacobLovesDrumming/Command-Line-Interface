/* Jacob Almeda
 * CSC 345
 * Project 1 - User Interface
 */
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <wait.h>
#include <fcntl.h>

#define MAX_LINE 128    // the maximum length of command
#define MAX_ARGS 64     // the maximum number of arguments
#define READ_END 0      // for pipe file descriptor
#define WRITE_END 1     // for pipe file descriptor

/* structure for command history linked list
   it is a doubly linked list */
typedef struct HistoryItem
{
    char command[MAX_LINE];
    struct HistoryItem* next;
    struct HistoryItem* prev;
} commandHistory;

/* initialize a linked list item with a command from the user */
struct HistoryItem* createItem(char* command)
{
    struct HistoryItem* newItem = (struct HistoryItem*)malloc(sizeof(struct HistoryItem));
    strcpy(newItem->command, command);
    newItem->next = NULL;
    newItem->prev = NULL;
    return newItem;
}

/* when there are 5 history items, shift them up by one
   (history command 2 is placed in item 1, for instance).
   The newest command is placed in spot 5 */
void rearrange(struct HistoryItem** head, char* new_command)
{
    struct HistoryItem* temp = *head;
    while (temp->next != NULL) {
        strcpy(temp->command, temp->next->command);
        temp = temp->next;
    }
    strcpy(temp->command, new_command);
}

/* insert a new command into history at the end of the linked list */
void insertItem(struct HistoryItem** head, char* command)
{
    int MAX_ITEMS = 5;
    int item_count = 1;
    struct HistoryItem* newItem = createItem(command);
    
    /* if the linked list is empty, the new item becomes the head */
    if (*head == NULL) {
        *head = newItem;
        return;
    }
    
    /* traverse through linked list and 
       place new item in first open spot, if it exists */
    struct HistoryItem* temp = *head;
    while (temp->next != NULL) {
        /* if there are five commands in history, rearrange the linked list */
        if (++item_count == 5) {
            /* shift items up by one, place new command in last spot */
            rearrange(head, command);
            free(newItem);
            return;
        }
        temp = temp->next;
    }
    newItem->prev = temp;
    temp->next = newItem;
}

/* get the command at the end of the linked list
   (the previous command) */
char* getLastItem(struct HistoryItem** head)
{
    /* is list is empty, return NULL */
    if (*head == NULL) {
        printf("no commands in history\n");
        return NULL;
    }
    /* traverse list to find last item */
    struct HistoryItem* temp = *head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    return temp->command;
}

/* traverse through linked list and print the command of each item */
void printHistory(struct HistoryItem** head) 
{
    struct HistoryItem* temp = *head;
    int i = 1;
    printf("\nHISTORY:\n");
    while (temp != NULL) {
        printf("%d: %s\n", i, temp->command);
        temp = temp->next;
        ++i;
    }
    printf("\n");
}

/* get the tail of the linked list */
struct HistoryItem* getTail(struct HistoryItem** head)
{
    struct HistoryItem* temp = *head;
    while (temp->next != NULL) {
        temp = temp->next;
    }
    return temp;
}

int main(void)
{
    int i, j;
    pid_t pid;
    char commandBuf[MAX_LINE];  // command line arguments
    char* args[MAX_ARGS];       // pointers to command line arguments
    int should_run = 1;         // flag to determine when to exit program 
    char full_path[1024];       // full path name
    char* curr_directory[512];  // current directory name
    int special_char;           // flag if command contains ">", "<", or "|"
    int i_after_char;           // index of arg after ">", "<", or "|"
    char* args_before_char[MAX_ARGS / 2];   // args before ">", "<", or "|"
    struct HistoryItem* historyHead = NULL; // head of command history linked list

    while (should_run) {
        /* get current directory name */
        getcwd(full_path, sizeof(full_path));
        curr_directory[0] = strtok(full_path, "/");
        i = 1;
        while (curr_directory[i - 1] != NULL && i < 512) {
            curr_directory[i] = strtok(NULL, "/");
            ++i;
        }

        /* print command line prompt
           flush any buffered output from previous commands, just in case */
        fflush(stdout);
        printf("osh:%s>", curr_directory[i - 2]);
        fflush(stdout);

        /* read command line input from user, remove the newline character */
        fgets(commandBuf, MAX_LINE, stdin);
        commandBuf[strcspn(commandBuf, "\n")] = '\0';

        /* check if command contains ">", "<", or "|" */
        special_char = 0;
        if (strstr(commandBuf, ">") != NULL) {
            special_char = 1;
        }
        else if (strstr(commandBuf, "<") != NULL) {
            special_char = 2;
        }
        else if (strstr(commandBuf, "|") != NULL) {
            special_char = 3;
        }

        /* if enter is pressed, but there is no user input */
        if (strcmp(commandBuf, "") == 0) {
            continue;
        }
        /* if "exit" is entered */
        else if (strcmp(commandBuf, "exit") == 0) {
            should_run = 0;
            continue;
        }
        /* print command history */
        else if (strcmp(commandBuf, "history") == 0)
        {
            printHistory(&historyHead);
            continue;
        }
        /* if command is "!!" */
        else if (strcmp(commandBuf, "!!") == 0) {
            char* prev_command = getLastItem(&historyHead);
            /* if there is no previous command, skip to next loop iteration */
            if (prev_command == NULL) {
                continue;
            }
            /* the previous command is placed in current command buffer 
               the command is echoed */
            strcpy(commandBuf, prev_command);
            printf("%s\n", commandBuf);

            /* check if command contains ">", "<", or "|" */
            if (strstr(commandBuf, ">") != NULL) {
                special_char = 1;
            }
            else if (strstr(commandBuf, "<") != NULL) {
                special_char = 2;
            }
            else if (strstr(commandBuf, "|") != NULL) {
            special_char = 3;
            }
        }
        else {
            /* save command into history */
            insertItem(&historyHead, commandBuf);
        }
        
        /* split command line input into tokens */
        args[0] = strtok(commandBuf, " ");
        i = 1;
        while (args[i - 1] != NULL && i < MAX_LINE/2 + 1) {
            args[i] = strtok(NULL, " ");
            ++i;
        }
    
        /* if command is "cd", change working directory */
        if (strcmp(args[0], "cd") == 0) {
            if (chdir(args[1]) != 0) {
                perror("cd failed");
            }
            continue;
        }

        /* if the command contains ">", "<", or "|", then
           separate the args before and after the character */
        if (special_char) {
            /* reset args_before_char array */
            for (i = 0; i < MAX_ARGS / 2; ++i) {
                args_before_char[i] = NULL;
            }
            /* copy args into args_before_char array until ">", "<", or "|" is found */
            i = 0;
            while (strcmp(args[i], ">") != 0 && strcmp(args[i], "<") != 0 && strcmp(args[i], "|") != 0) {
                args_before_char[i] = args[i];
                if (args[++i] == NULL) {
                    break;
                }
            }
            i_after_char = i + 1;
        }

        /* After reading user input, the steps are:
        * (1) fork a child process using fork()
        * (2) the child process will invoke execvp()
        * (3) parent will invoke wait() unless command included & 
        */

       /* (1) fork a child process */
       pid = fork();

        if (pid < 0) { /* error occurred */
            fprintf(stderr, "Fork Failed");
            return 1;
        }

        /* *** CHILD process ***
           (2) the child process will invoke execvp() */
        else if (pid == 0) 
        { 
            int fd = -1;        // file descriptor for redirection
            int pipe_fd[2];     // file descriptor for communication using pipe
            pid_t grand_pid;    // pid to create a grandchild process for piping

            /* output redirection (>) */
            if (special_char == 1) {
                /* set fd to file descriptor of user-given file */
                fd = open(args[i_after_char], O_WRONLY);
                /* duplicate fd to standard output */
                dup2(fd, STDOUT_FILENO);
                close(fd);
                /* the output of the command is redirected into the file */
                if (execvp(args_before_char[0], args_before_char) < 0) {
                    perror("execvp failed");
                    exit(1);
                }
                return 0;
            }
            /* input redirection (<) */
            else if (special_char == 2) {
                /* set fd to file descriptor of user-given file */
                fd = open(args[i_after_char], O_RDONLY);
                /* duplicate fd to standard input */
                dup2(fd, STDIN_FILENO);
                close(fd);
                /* the data from the file becomes the input for the command */
                if (execvp(args_before_char[0], args_before_char) < 0) {
                    perror("execvp failed");
                    exit(1);
                }
                return 0;
            }
            /* communication via a pipe */
            else if (special_char == 3) {
                /* create pipe */
                if (pipe(pipe_fd) == -1) {
                    fprintf(stderr, "Pipe failed");
                    return 1;
                }
                /* create grandchild process */
                grand_pid = fork();

                if (grand_pid < 0) { /* error occurred */
                    fprintf(stderr, "Fork failed");
                    return 1;
                }
                else if (grand_pid > 0) { /* parent (FIRST CHILD) process */
                    /* close unused end of pipe */
                    close(pipe_fd[READ_END]);

                    /* duplicate the write end of pipe to standard output */
                    dup2(pipe_fd[WRITE_END], STDOUT_FILENO);
                    /* close write end of pipe */
                    close(pipe_fd[WRITE_END]);
                    /* the output of the command is put into the write end of the pipe */
                    if (execvp(args_before_char[0], args_before_char) < 0) {
                        perror("execvp failed");
                        exit(1);
                    }
                    return 0;
                }
                else { /* child (GRANDCHILD) process */
                    /* args after ">", "<", or "|" */
                    char* args_after_char[MAX_ARGS / 2];
                    i = 0;
                    j = i_after_char;
                    /* save args after ">", "<", or "|" */
                    while (args[j] != NULL) {
                        args_after_char[i] = args[j];
                        ++i;
                        ++j;
                    }
                    /* close unused end of pipe */
                    close(pipe_fd[WRITE_END]);

                    /* duplicate the read end of the pipe to standard input */
                    dup2(pipe_fd[READ_END], STDIN_FILENO);
                    /* close the read end of the pipe */
                    close(pipe_fd[READ_END]);
                    /* use data from read end of pipe as input to the command */
                    if (execvp(args_after_char[0], args_after_char) < 0) {
                        perror("execvp failed");
                        exit(1);
                    }
                }
            }
            /* execute the command */
            else if (execvp(args[0], args) < 0) {
                perror("execvp failed");
                exit(1);
            }
            return 0;
        }

        /* *** PARENT process ***
           (3) parent will invoke wait() unless command included & */
        else 
        {  
            /* find length of args to check final index for "&" */
            i = 0;
            while (args[i] != NULL) {
                ++i;
            }
            /* if the command does not includes "&", wait for child */
            if (strcmp(args[i - 1], "&") != 0) {
                wait(NULL);
            }
        }
    }
    return 0;
}