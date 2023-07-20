 //header file with constants and functions used by syntax_tree.c module
#include "sillypare.h"
#include "hardcore_northcutt.h"

static char and_linker[] = "&&";
static char or_linker[] = "||";
static char next_linker[] = ";";
static char background_process_linker[] = "&";
static char opening_bracket[] = "(";
static char closing_bracket[] = ")";
static char greater[] = ">";
static char double_greater[] = ">>";
static char smaller[] = "<";
static char reverse_slash[] = "\\";

int is_linker(char *lexem);
int is_redirection_sym(char *lexem);
void null_struct_fields(cmd_link cmd);
void rm_syntax_tree(cmd_link tree_root);
int find_closing_bracket(list_type lst,int start_index,int end_index);
void print_list_slice(list_type lst,int start_index,int end_index);
int find_linker(list_type lst,int start_index,int end_index);

int redirect_io(list_type lst,cmd_link cmd,int start_index,int end_index);
void read_args(list_type lst,cmd_link cmd,int start_index,int end_index);
cmd_link build_syntax_tree(list_type lst,int start_index,int end_index);
void create_pipe(list_type lst,cmd_link first_pipe_cmd,int start_index,int end_index);
cmd_link build_pipe_chain(list_type lst,int start_index,int end_index);
void build_cmd(list_type lst,cmd_link cmd,int start_index,int end_index);
void build_subshell_cmd(list_type lst,cmd_link cmd,int start_index,int end_index);
void build_simple_cmd(list_type lst,cmd_link cmd,int start_index,int end_index); 
void build_comma(int i);
