#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "../include/command.h"
#include "../include/builtin.h"

// ======================= requirement 2.3 =======================
/**
 * @brief 
 * Redirect command's stdin and stdout to the specified file descriptor
 * If you want to implement ( < , > ), use "in_file" and "out_file" included the cmd_node structure
 * If you want to implement ( | ), use "in" and "out" included the cmd_node structure.
 *
 * @param p cmd_node structure
 * 
 */
void redirection(struct cmd_node *p){
	int fd;
	// <
	// stdin(fd[0]) point to target file
	if (p->in_file != NULL) {
		fd = open(p->in_file, O_RDONLY); // open() returns -1 if failed
		// OS gives you a new file descriptor, points to the in_file
		if (fd < 0) {
			perror("open input file failed!");
			exit(EXIT_FAILURE);
		}
		if (dup2(fd, STDIN_FILENO) < 0) {
			// Make file descriptor STDIN_FILENO (0) point to the same open file as fd
			perror("dup2 stdin failed!");
			close(fd);
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	// >
	// stdout(fd[1]) point to target file
	if (p->out_file != NULL) {
		fd = open(p->out_file, O_WRONLY|O_CREAT|O_TRUNC, 0644);
		if (fd < 0) {
			perror("open output file failed!");
			exit(EXIT_FAILURE);
		}
		if (dup2(fd, STDOUT_FILENO) < 0) {
			perror("dup2 output file failed!");
			close(fd);
			exit(EXIT_FAILURE);
		}
		close(fd);
	}
	// for 2.4, we need redirection to support pipe()
	// if there exists a pipe, the in|out of the node would not be 0|1
	if (p->in != 0 && p->in != -1) {
		if (dup2(p->in, STDIN_FILENO) < 0) {
			perror("dup2 pipe in error!");
			exit(EXIT_FAILURE);
		}
		close(p->in);
	}
	if (p->out != 1 && p->out != -1) {
		if (dup2(p->out, STDOUT_FILENO) < 0) {
			perror("dup2 pipe out error!");
			exit(EXIT_FAILURE);
		}
		close(p->out);
	}
}
// ===============================================================

// ======================= requirement 2.2 =======================
/**
 * @brief 
 * Execute external command
 * The external command is mainly divided into the following two steps:
 * 1. Call "fork()" to create child process
 * 2. Call "execvp()" to execute the corresponding executable file
 * @param p cmd_node structure
 * @return int 
 * Return execution status
 */
int spawn_proc(struct cmd_node *p)
{
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed!");
        return -1;
    } else if (pid == 0) {
		// the external command should have modified stdin/stdout/stderr
		// for example: ls > out.txt
        redirection(p);
        execvp(p->args[0], p->args);
        perror("execvp() failed!");
        exit(EXIT_FAILURE);
    } else {
        // Only wait if this command does not send output into a pipe
		// If this command is sending output to a pipe, 
		// p->out will be set to the write end of that pipe (some fd > 1).
		// example: ls | grep txt
		// Let both run at the same time or else it'll be Serializing the pipeline
        if (p->out == 1) {
			// go straight to the terminal
            int status;
            if (wait(&status) < 0) {
                perror("wait error!");
                return -1;
            }
        }
		// If this is a normal command (no pipe) ¡÷ wait() now.
		// If this command is part of a pipe ¡÷ don¡¦t wait() here; 
		// it will be handled as part of the whole pipeline.
    }
    return 1;
}

// ===============================================================


// ======================= requirement 2.4 =======================
/**
 * @brief 
 * Use "pipe()" to create a communication bridge between processes
 * Call "spawn_proc()" in order according to the number of cmd_node
 * @param cmd Command structure  
 * @return int
 * Return execution status 
 */

// Each command is independent, but the pipe connects them like a data stream.
int fork_cmd_node(struct cmd *cmd)
{
    struct cmd_node *cur = cmd->head;

    if (cur == NULL) {
        return -1;
    }

    int prev_read_fd = 0;   // stdin for first command
    int pipefd[2];

    while (cur != NULL) {
        int write_fd = 1;       // default: stdout
        int next_read_fd = -1;  // read end for next command

        // If there is a next command, create a pipe
        if (cur->next != NULL) {
            if (pipe(pipefd) < 0) {
                perror("pipe error!");
                return -1;
            }
            next_read_fd = pipefd[0];  // read side for next command
            write_fd     = pipefd[1];  // write side for current command
        }

        // Tell this node which fds to use
        cur->in  = prev_read_fd;
        cur->out = write_fd;

        // Run this command (child will call redirection() + execvp())
        if (spawn_proc(cur) < 0) {
            return -1;
        }

        // Parent: close write end we just used
        if (write_fd != 1) {
            close(write_fd);
        }

        // Parent: close previous read end (no longer needed)
        if (prev_read_fd != 0 && prev_read_fd != -1) {
            close(prev_read_fd);
        }

        // Prepare for next command
        prev_read_fd = next_read_fd;
        cur = cur->next;
    }

    // Safety: close last read end if still open
    if (prev_read_fd != 0 && prev_read_fd != -1) {
        close(prev_read_fd);
    }

    return 1;
}

// ===============================================================


void shell()
{
	while (1) {
		printf(">>> $ ");
		char *buffer = read_line();
		if (buffer == NULL)
			continue;

		struct cmd *cmd = split_line(buffer);
		
		int status = -1;
		// only a single command
		struct cmd_node *temp = cmd->head;
		
		if(temp->next == NULL){
			status = searchBuiltInCommand(temp);
			if (status != -1){
				int in = dup(STDIN_FILENO), out = dup(STDOUT_FILENO);
				if( in == -1 || out == -1)
					perror("dup");
				redirection(temp);
				status = execBuiltInCommand(status,temp);

				// recover shell stdin and stdout
				if (temp->in_file)  dup2(in, 0);
				if (temp->out_file){
					dup2(out, 1);
				}
				close(in);
				close(out);
			}
			else{
				//external command
				status = spawn_proc(cmd->head);
			}
		}
		// There are multiple commands ( | )
		else{
			
			status = fork_cmd_node(cmd);
		}
		// free space
		while (cmd->head) {
			
			struct cmd_node *temp = cmd->head;
      		cmd->head = cmd->head->next;
			free(temp->args);
   	    	free(temp);
   		}
		free(cmd);
		free(buffer);
		
		if (status == 0)
			break;
	}
}
