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
		fd = open(p->in_file, O_RDONLY);
		// OS gives you a new file descriptor, points to the in_file
		if (fd < 0) {
			perror("open input file failed!");
			exit(EXIT_FAILURE);
		}
		if (dup2(fd, STDIN_FILENO) < 0) {
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
	if (p->in != 0 && p->in != -1) {
		if (dup2(p->in, STDIN_FILENO) < 0) {
			perror("dup2 pipe in");
			exit(EXIT_FAILURE);
		}
		close(p->in);
	}
	if (p->out != 1 && p->out != -1) {
		if (dup2(p->out, STDOUT_FILENO) < 0) {
			perror("dup2 pipe out");
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
	// the external commands will go into this section
	pid_t pid = fork();
	if (pid < 0) {
		// pid < 0 -> error
		perror("fork failed!");
		return -1;
	} else if (pid == 0) {
		// child process
		execvp(p->args[0], p->args);
		perror("execvp() failed!");
		exit(EXIT_FAILURE);
	} else {
		// parent process
		int status;
		wait(&status);
		if (wait(&status) < 0) {
			perror("wait error!");
			return -1;
		}
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
				if( in == -1 | out == -1)
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
