typedef struct cmd_struct *cmd_link;

typedef enum linker_type {And,Or,Next,Backgnd,None} linker_type;
typedef enum redir_lex {Greater,Double_greater,Smaller} redir_lex;

struct cmd_struct {
	char ** argv; // list containing command name and its arguments
	char *infile; // reassigned input file descriptor
	char *outfile; // reassigned output file descriptor
	int append_mode;
	cmd_link psubcmd; // commands for launch in subshell
	cmd_link next_pipe_cmd; // next command after “|”
	cmd_link next; // next command after “;” (of after &&)
	linker_type linker; // equals to ; && & or || or \0 (if there is no link)
};