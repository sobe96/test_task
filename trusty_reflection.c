//Автор этого шедевра - Грищенко Алексей 209 группа

#include <stdio.h>
#include <setjmp.h>
#include "sillypare.h"
#include "hardcore_northcutt.h"

list_type get_lexem_list(status *program_status);

void print_lexem_list(list_type lst);

int get_lexem_list_len(list_type lst);

cmd_link build_syntax_tree(list_type lst,int start_index,int end_index);

void print_syntax_tree(cmd_link cmd_tree_root,int shift);

void rm_syntax_tree(cmd_link tree);

void run_tree(cmd_link tree);

int main(int argc,char *argv[]){
	int tree_shift = 0;
 	int lexem_list_len = 0;	
	status program_status = Success;

 	list_type lexem_list = NULL;
	cmd_link tree_root = NULL;

	while(1){
		printf("=>");
		lexem_list = get_lexem_list(&program_status);

		if ( (program_status == Success) || (program_status==Finish)){

			lexem_list_len = get_lexem_list_len(lexem_list);

			tree_root = build_syntax_tree(lexem_list,0,lexem_list_len-1);

			run_tree(tree_root);

			rm_syntax_tree(tree_root);

			if (program_status==Finish)
				break;
		}

		else{
			printf("ERROR");
			break;
		}
		
	}
	
	

	return 0;
}
