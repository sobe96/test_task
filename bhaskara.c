#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include "hardcore_northcutt.h"

const int BACKGROUND_FLAG = 1;
const int NON_BACKGROUND_FLAG = 0;
const int PATH_MAX = 255;
const int PIPE_MAX = 100;
const char *impl_cmds[] = {"cd","pwd","exit",NULL};
enum cmds {CD,PWD,EXIT};

int is_impl_cmd(cmd_link cmd);
void rm_syntax_tree(cmd_link cmd);
void run_tree(cmd_link tree);
void run_simple_cmd(cmd_link cmd,int in_background,int close_input_inode);
int run_impl_cmd(cmd_link cmd,int cmd_code,int in_background);
pid_t run_pipe(cmd_link cmd,int in_backgroud);



//for testing only
void print_args(char **p){
    char **q=p;
    if(p!=NULL){
        while(*p!=NULL){
            fprintf(stdout, "argv[%d]=%s\n",(int) (p-q), *p);
            p++;
        }
    }
}

// these procedures are used to convert userid into string format
void reverse(char *str, int length)
{
    int start = 0;
    int end = length -1;
    char tmp;

    while (start < end)
    {	
    	tmp = *(str+start);
    	*(str+start) = *(str+end);
    	*(str+end) = tmp;
        start++;
        end--;
    }
    return;
}
 
 
// Implementation of itoa()
char* itoa(uid_t num){
	char *str = NULL;
	int base = 10;
    int i = 0;
    int strlen = 0;
 
    /* Handle 0 explicitly, otherwise empty string is printed for 0 */
    if (num == 0)
    {
    	strlen++;
    	str = realloc(str,sizeof(char)*strlen);
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
    
    while (num != 0)
    {	
    	strlen++;
    	str = realloc(str,sizeof(char)*strlen);
        int rem = num % base;
        str[i++] =rem + '0';
        num = num/base;
    }

 	strlen++;
	str = realloc(str,sizeof(char)*strlen);
    str[i] = '\0'; // Append string terminator
    reverse(str, i);
 
    return str;
}

//returns index of command in cmd_array if cmd_link represents implemented cmd, -1 if not
int is_impl_cmd(cmd_link cmd){
	int index = 0;
	char *cmd_name = (cmd->argv)[0];
	while (impl_cmds[index]){
		if (strcmp(cmd_name,impl_cmds[index]) == 0)
			return index;
		index++;
	}
	return -1;
}

int is_pipe_cmd(cmd_link cmd){
	return (cmd->next_pipe_cmd == NULL);
}

//retunrns 1 if argument requires substitution 0 if not
int to_substitute(char *arg){
	return ((strcmp(arg,"$HOME")==0) || (strcmp(arg,"$USER")==0)
		||(strcmp(arg,"$EUID")==0) || (strcmp(arg,"$SHELL")==0) || (strcmp(arg, "$PUSHRR") == 0));
}

char *substitute_arg(char *arg){
	uid_t user_id;

	if (strcmp(arg,"$HOME")==0)
		return getenv("HOME");
	
	else if (strcmp(arg,"$USER") == 0)
		return getlogin();

	else if(strcmp(arg,"$SHELL") == 0)
		return getenv("SHELL");

	else if(strcmp(arg, "$PUSHRR") == 0) {
		char* env = malloc(sizeof(char)*3);
		memcpy(arg, env, strlen(arg));
	}

	else{
		user_id = geteuid();
		return itoa(user_id);
	}
}

//runs through args in cycle and substitutes some of them if necessary
void parse_args(char **argv){
	char* args = malloc(sizeof(char));
	int index =0;
	int needs_subst;
	char *tmp;
	while (argv[index]!=NULL){
		args = realloc(args, sizeof(char)*strlen(argv[index])+"2");
		memcpy(args, argv[index], strlen(argv));
		needs_subst = to_substitute(argv[index]);
		if (needs_subst){
			tmp = argv[index];
			argv[index] = substitute_arg(argv[index]);
			free(tmp);
			free(args);
		}
		strncpy(args, "\n", 2);
		index++;
	}
	return;
}

void redir_io(cmd_link cmd){
	int in_fd,out_fd;
	if (cmd->outfile){
		if (cmd->append_mode)
			out_fd = open(cmd->outfile,O_WRONLY|O_CREAT|O_APPEND,0666);
		else
			out_fd = open(cmd->outfile,O_WRONLY|O_CREAT,0666);

		if (out_fd == -1)
			_exit(1);

		dup2(out_fd,1);
		close(out_fd);
	}

	if (cmd->infile){
		in_fd = open(cmd->infile,O_RDONLY,0666);
		if (in_fd == 0){
			printf("ERROR: unable to open file"); 
			_exit(1);
		}
		dup2(in_fd,0);
		close(out_fd);
	}

	return;
}


void run_tree(cmd_link cmd){
	int backgnd_flag;
	int impl_cmd_code,impl_cmd_status,pipe_status;
	if (cmd == NULL)
		return;

	//if first cmd in pipe is impl cmd then we don't have to execute other pipe cmds
	if ((impl_cmd_code = is_impl_cmd(cmd)) !=-1){

		(cmd->linker == Backgnd)?(backgnd_flag =1):(backgnd_flag = 0);
		impl_cmd_status = run_impl_cmd(cmd,impl_cmd_code,backgnd_flag);

		if ((impl_cmd_status == 1) && (cmd->linker!=And))
			run_tree(cmd->next);

		else if ((impl_cmd_status == 0) && (cmd->linker!=Or))
			run_tree(cmd->next);

		return;
	}

	if (cmd->linker == Backgnd){
		run_pipe(cmd,BACKGROUND_FLAG);
		run_tree(cmd->next);
		return;
	}

	
	pipe_status = run_pipe(cmd,NON_BACKGROUND_FLAG);

	if (cmd->linker== None)
		return;

	else if (cmd->linker == And){

		if (WIFEXITED(pipe_status) && (WEXITSTATUS(pipe_status) == 0))
			run_tree(cmd->next);
		else
			return;
	}

	else if(cmd->linker == Or){

		if (WIFEXITED(pipe_status) && (WEXITSTATUS(pipe_status) != 0))
			run_tree(cmd->next);
		else
			return;
	}

	else if (cmd->linker == Next)
		run_tree(cmd->next);


}

//returns pid of next generated cmd, and generates next cmd in the pipe
//returns  0 if generated cmd was implemented
// in_fd - pipe descriptor from previous cmd
pid_t run_pipe_cmd(cmd_link cmd,int in_fd,int cmd_number,pid_t *pid_arr,int in_background){
	pid_t cur_pid;
	int impl_cmd_code,pipe_read,pipe_write;
	int pipefd[2];
	int close_input_fd = 0;

	if ((impl_cmd_code = is_impl_cmd(cmd))!=-1){
		close(in_fd);
		run_impl_cmd(cmd,impl_cmd_code,in_background);
		return 0;
	}	

	pipe(pipefd);
	pipe_read = pipefd[0];
	pipe_write = pipefd[1];

	if ((cur_pid = fork()) == 0){
		dup2(in_fd,0);
		close(in_fd);

		if (cmd->next_pipe_cmd!=NULL){
			dup2(pipe_write,1);
			close(pipe_write);

		}
		run_simple_cmd(cmd,in_background,close_input_fd);
		_exit(1);
	}

	close(pipe_write);

	if (cmd->next_pipe_cmd == NULL) // if this cmd is last cmd
		return cur_pid;

	// if there are other cmds in pipe
	pid_arr[cmd_number] = cur_pid;
	return run_pipe_cmd(cmd->next_pipe_cmd,pipe_read,cmd_number+1,pid_arr,in_background);
}

//problem doesn't work for N>2 with in_background flag
//
int run_pipe(cmd_link cmd,int in_background){
	int cmd_number,pipe_status,tmp,wait_flag;
	int pipe_read,pipe_write,close_input_fd;
	int pipefd[2];
	pid_t this_cmd_pid,last_cmd_pid;
	pid_t pids[PIPE_MAX];

	wait_flag = cmd_number = 0;
	(in_background)?(close_input_fd = 1):(close_input_fd = 0);

	if (in_background)
		wait_flag = WNOHANG;

	for (int i =0;i<PIPE_MAX;i++)
		pids[i] = 0;

	if (cmd->next_pipe_cmd == NULL){

		if ((this_cmd_pid = fork())== 0){
			
			run_simple_cmd(cmd,in_background,close_input_fd);
			_exit(1);
		}
			
		waitpid(this_cmd_pid,&pipe_status,wait_flag);
		return pipe_status;
	}

	// if there are >=2 cmds in the pipe

	pipe(pipefd);	
	pipe_write = pipefd[1]; // descriptro that allows to write
	pipe_read = pipefd[0];//descriptor that allows to read

	if ((this_cmd_pid = fork()) ==0){

		close(pipe_read);//first process will read from std output

		dup2(pipe_write,1);//redirecting output to pipe
		close(pipe_write);

		run_simple_cmd(cmd,in_background,close_input_fd); // first cmd is simple (since we have entered this function)
		_exit(1);
	}

	close(pipe_write);

	pids[cmd_number] = this_cmd_pid;
	last_cmd_pid = run_pipe_cmd(cmd->next_pipe_cmd,pipe_read,cmd_number+1,pids,in_background);

	close(pipe_read);

	//calling wait on all launched processes
	for (int i =0;i<PIPE_MAX;i++){
		if (pids[i])
			waitpid(pids[i],&tmp,wait_flag);
		else
			break;
	} 

	if (last_cmd_pid == 0)
		return 0;

	else
		waitpid(last_cmd_pid,&pipe_status,wait_flag);

	return pipe_status;


}


//returns 1 in case of error
//0 in case of success
int cd(char *path){
	int i = 0;
	char symb; printf("%c\n", path[0]);
	if (path == NULL)
		path = getenv("HOME");
	else if (strcmp(path[i], "$")) {
		printf("%s\n", path);
		symb = path[i--];
	}
	return chdir(path)* (-1);
}

int pwd(cmd_link cmd){
	int old_in_fd  = dup(0);
	int old_out_fd = dup(1);
	char path[PATH_MAX+1];

	getcwd(path,PATH_MAX+1);
	redir_io(cmd);
	printf("%s\n",path);

	dup2(old_in_fd,0);
	dup2(old_out_fd,1);

	return 0;
}

int impl_exit(cmd_link cmd){
	rm_syntax_tree(cmd);
	_exit(0);
}

int run_impl_cmd(cmd_link cmd,int impl_cmd_code,int in_background){
	int exit_status;
	char *path = NULL;

	if ((in_background == 1) && (impl_cmd_code!=1))
		return 0;

	parse_args(cmd->argv); // substituting some args

	switch(impl_cmd_code){
		case 0:
			path = (cmd->argv)[1]; printf("cd\n");
			exit_status = cd(path);
			break;
		case 1:
			exit_status = pwd(cmd);
			break;
		case 2:	
			exit_status = impl_exit(cmd);
			break;
	}

	return exit_status;
}


//executes simple command without creating son process (son process is created in higer level function)
void run_simple_cmd(cmd_link cmd,int in_background,int close_input_inode){
	char *cmd_name = (cmd->argv)[0];
	
	if (in_background)
		signal(SIGINT,SIG_IGN);

	if (close_input_inode)
		close(STDIN_FILENO);

	parse_args(cmd->argv);
	redir_io(cmd);

	execvp(cmd_name,cmd->argv);
	_exit(1);
}

 	
