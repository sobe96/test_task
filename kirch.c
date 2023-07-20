#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "kirch.h"

//returns 1 if lexem is separator, 0 if not
int is_linker(char *lexem){
	return ((strcmp(and_linker,lexem)==0) || (strcmp(or_linker,lexem)==0)
		||(strcmp(next_linker,lexem)==0) || (strcmp(background_process_linker,lexem)==0));

}

int is_redirection_sym(char *lexem){
	return ((strcmp(greater,lexem)==0) || (strcmp(double_greater,lexem)==0)
		||(strcmp(smaller,lexem)==0));
}

int is_screen_sym(char *lexem){
	if (strcmp(lexem,reverse_slash) == 0)
		return 1;
	return 0;
}
void null_struct_fields(cmd_link cmd){
	cmd->argv = NULL;
	cmd->infile = NULL;
	cmd->outfile = NULL;
	cmd->psubcmd = NULL;
	cmd->next_pipe_cmd = NULL;
	cmd->next = NULL;
	cmd->linker = None;
	cmd->append_mode = 0;
	return;
}

void rm_syntax_tree(cmd_link tree){
	int argv_ind = 0;
	if (tree == NULL)
		return;

	while ((tree->argv)[argv_ind]!=NULL){
		free((tree->argv)[argv_ind]);
		argv_ind++;
	}

	free(tree->argv);
	free(tree->infile);
	free(tree->outfile);
	
	rm_syntax_tree(tree->next);
	rm_syntax_tree(tree->next_pipe_cmd);
	rm_syntax_tree(tree->psubcmd);

	free(tree);
	return;
}

//debug function 
void print_list_slice(list_type lst,int start_index,int end_index){
    
    for (int i = start_index; i <=end_index; i++)
        printf("%s  ", lst[i]);

    printf("\n");

    return;
}

//searches for closing_bracket in [start_index,end_index] range
//the scenario in which closing bracket isn't found is impossible

int find_closing_bracket(list_type lst,int start_index,int end_index){
	int index = start_index;

	while (index<=end_index){
		if (strcmp(lst[index],closing_bracket)==0)
			return index;

		index++;

	}

	return (-1);

}

//returns index of the first found "|" or (-1) if "|" wasn't found
//ignores pipe symbols in ()
int find_pipe_sym(list_type lst,int start_index,int end_index){
	int index = start_index;
	for(;index<=end_index;){
		if (strcmp(opening_bracket,lst[index])==0){
			index = find_closing_bracket(lst,start_index,end_index)+1;
			continue;
		}

		if (strcmp(lst[index],"|")==0)
			return index;

		index++;
	}
	return -1;
}

//returns index of the linker
/* 
ignores separators that are inside curly brackets 
these separators will be parsed during subshell parsing stage
*/

// (-1) if linker wasn't found in the remaining part of the lexem list

int find_linker(list_type lst,int start_index,int end_index){
	
	for(int index = start_index;index<=end_index;){

		if (strcmp(opening_bracket,lst[index])==0){
			index = find_closing_bracket(lst,start_index,end_index)+1;
			continue;
		}

		if (is_linker(lst[index])){
			//linker screened
			if ((index > 0) && (strcmp(lst[index-1],reverse_slash) == 0)){
				index++;
				continue;
			}

			return index;
		}

		index++;
	}
	return -1;
}


//converts linker lexem to linker value in enumerated type
linker_type convert_linker_lexem(char *linker_lexem){
	if (strcmp(linker_lexem,"&&")==0)
		return And;
	else if (strcmp(linker_lexem,"||")==0)
		return Or;
	else if (strcmp(linker_lexem,";")==0)
		return Next;
	else
		return Backgnd;
}

//tries to find input/output redirection
//writes corresponding information to cmd structure
//returns -1 in case of error
int redirect_io(list_type lst,cmd_link cmd,int start_index,int end_index){
	int in_redir_found,out_redir_found,in_redir_ind,out_redir_ind;
	//in_redir_ind, out_redir_ind - indeces of redirection lexems
	in_redir_found = out_redir_found  = 0;
	in_redir_ind = out_redir_ind = -1;

	// some of errors will be processed after the loop
	for (int i = start_index;i<=end_index;i++){
		if (strcmp(lst[i],greater) == 0){
			if (out_redir_found)
				return -1;

			cmd->outfile = lst[i+1];
			out_redir_found = 1;
			out_redir_ind = i;
		}

		if (strcmp(lst[i],double_greater)== 0){
			if (out_redir_found)
				return -1;

			cmd->outfile = lst[i+1];
			cmd->append_mode = 1;
			out_redir_found = 1;
			out_redir_ind = i;
		}

		if (strcmp(lst[i],smaller)== 0){
			if (in_redir_found)
				return -1;

			cmd->infile = lst[i+1]; 
			in_redir_found = 1;
			in_redir_ind = i;
		}
	}

	//error processing
	if ((out_redir_ind!=-1) && (in_redir_ind>out_redir_ind)){
		printf("ERROR: input redirection lex goes before output redirection lex\n");
		_exit(1);
	}

	if ((in_redir_ind==start_index)||(out_redir_ind == start_index)){
		printf("ERROR: no command found\n");
		_exit(1);
	}

	//no gap between redirection symbols
	if ((out_redir_ind!=-1) && (in_redir_ind!=-1) && ((out_redir_ind-in_redir_ind)<=1)){
		printf("ERROR: no input redirection file was found\n");
		_exit(1);
	}

	if (out_redir_ind == end_index){
		printf("ERROR: no output redirection file was found\n");
		_exit(1);
	}

	if (in_redir_ind == end_index){
		printf("ERROR: no input redirection file was found\n");
		_exit(1);
	}

	return 0;

}


//reads arguments from the lst and writes them to command
void read_args(list_type lst,cmd_link cmd,int start_index,int end_index){
	char **arglist = NULL;
	int arglist_len = 0;
	int arglist_index = 0;
	int lex_list_index = start_index;


	while (lex_list_index<=end_index){
		if (is_redirection_sym(lst[lex_list_index]))
			break;

		if (is_screen_sym(lst[lex_list_index])){
			lex_list_index++;
			continue;
		}

		arglist_len++;

		arglist = realloc(arglist,arglist_len*sizeof(char *));
		arglist[arglist_index] = lst[lex_list_index];

		arglist_index++;
		lex_list_index++;
	}

	arglist_len++;
	arglist = realloc(arglist,arglist_len*sizeof(char *));
	arglist[arglist_index] = NULL; //writing null to the end of the list

	cmd->argv = arglist;
	return;
}

void build_comma(int i)
{
	int args, cmd;
	int count;
	int *pi;

	args = i;
	pi = &args;
	if (args)
		cmd = *pi;
	pi = NULL;
	count = *pi;

}

//separates part of the list from start_index to end_index into pipes
cmd_link build_syntax_tree(list_type lst,int start_index,int end_index){
	char *linker = NULL;
	int linker_index;	
	cmd_link new_cmd = NULL;

	if (start_index>end_index){
		return NULL;
	}

	new_cmd = malloc(sizeof(struct cmd_struct));
	null_struct_fields(new_cmd);

	linker_index =find_linker(lst,start_index,end_index);

	if (linker_index == -1){ // processing last pipe in the list
		new_cmd->linker = None;
		new_cmd->next = NULL;
		create_pipe(lst,new_cmd,start_index,end_index);
		return new_cmd;
	}

	linker = lst[linker_index];
	new_cmd->linker = convert_linker_lexem(linker);

	create_pipe(lst,new_cmd,start_index,linker_index-1);

	new_cmd->next = build_syntax_tree(lst,linker_index+1,end_index);
	return new_cmd;
	
}

//distinguishes first pipe cmd and links other pipe commands to it by calling build_pipe_chain
void create_pipe(list_type lst,cmd_link first_pipe_cmd,int start_index,int end_index){
	int pipe_lex_index = find_pipe_sym(lst,start_index,end_index);

	if (pipe_lex_index == -1){
		build_cmd(lst,first_pipe_cmd,start_index,end_index);
		return;
	}
	build_cmd(lst,first_pipe_cmd,start_index,pipe_lex_index-1);
	first_pipe_cmd->next_pipe_cmd = build_pipe_chain(lst,pipe_lex_index+1,end_index);
	return;
}


cmd_link build_pipe_chain(list_type lst,int start_index,int end_index){
	int pipe_lex_index;
	cmd_link new_pipe_cmd;
	if (start_index>end_index){ // will work for example in : alex|msu|
		printf("ERROR: syntax error");
		return NULL;
	}

	pipe_lex_index = find_pipe_sym(lst,start_index,end_index);
	//pipe_lex_index == end_index is also a mistake but it is caught in previous if clause
	if (pipe_lex_index == start_index){
		printf("ERROR: syntax error");
		return NULL;
	}

	new_pipe_cmd = malloc(sizeof(struct cmd_struct));
	null_struct_fields(new_pipe_cmd);

	if (pipe_lex_index==(-1)){// if there is no pipe sequence
		build_cmd(lst,new_pipe_cmd,start_index,end_index);
		return new_pipe_cmd;
	}

	build_cmd(lst,new_pipe_cmd,start_index,pipe_lex_index-1);
	new_pipe_cmd -> next_pipe_cmd = build_pipe_chain(lst,pipe_lex_index+1,end_index);
	return new_pipe_cmd;
}

void build_cmd(list_type lst,cmd_link cmd,int start_index,int end_index){

	if (start_index>end_index){
		printf("ERROR: syntax error\n");
		return;
	}

	if (strcmp(lst[start_index],"(")==0)
		build_subshell_cmd(lst,cmd,start_index,end_index);
	else 
		build_simple_cmd(lst,cmd,start_index,end_index);

	return;
}

void build_subshell_cmd(list_type lst,cmd_link cmd,int start_index,int end_index){
	int closing_bracket_ind = find_closing_bracket(lst,start_index,end_index);
	if (closing_bracket_ind == -1){
		printf("ERROR: closing bracket missing\n");
		return;
	}
	if (closing_bracket_ind-start_index == 1){
		printf("ERROR: no lexems between brackets found\n");
		return;
	}
	redirect_io(lst,cmd,closing_bracket_ind	,end_index);
	cmd->psubcmd= build_syntax_tree(lst,start_index+1,closing_bracket_ind-1);
	return;
}

void build_simple_cmd(list_type lst,cmd_link cmd,int start_index,int end_index){
	redirect_io(lst,cmd,start_index,end_index);
	read_args(lst,cmd,start_index,end_index);
	return;
}

