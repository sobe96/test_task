 #include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include "sillypare.h"


//TODO: organize error printing 
//TODO: organize processing of separator concatenation error (e.g &;)
#define SIZE 16
#define INCOMP_LEXEM_ARRLEN 8
//graph type
typedef enum { Start, Word, Pairable_sym} vertex;


static list_type lst; /* word list */
static buf_type buf; /* buffer that collects current lexem*/
static int sizebuf;  /* size of current lexem*/
static int sizelist; /* word list size*/
static int curbuf;   /* index of cur symbol in buf*/
static int curlist;  /* index of cur lexem in array*/

//symbols that can be used to create normal words (no special symbols included)
static char word_char_array[] = "abcdefghijklmnopqrstuvwxyz0123456789$.,/_\\";

void clearlist() {
    int i;
    sizelist = 0;
    curlist = 0;
    if (lst == NULL)
        return;
    for (i = 0; lst[i] != NULL; i++)
        free(lst[i]);
    free(lst);
    lst = NULL;
}

void null_list() {
    sizelist = 0;
    curlist = 0;
    lst = NULL;
}

void termlist() {
    if (lst == NULL)
        return;
    if (curlist > sizelist - 1)
        lst = realloc(lst, (sizelist + 1) * sizeof(*lst));
    lst[curlist] = NULL;
    lst = realloc(lst, (sizelist = curlist + 1) * sizeof(*lst));
}

void nullbuf() {
    buf = NULL;
    sizebuf = 0;
    curbuf = 0;
}

void addsym(int cur_char) {
    if (curbuf > sizebuf - 1)
        buf = realloc(buf, sizebuf += SIZE);
    buf[curbuf++] = cur_char;
}

void addword() {
    if (curbuf > sizebuf - 1)
        buf = realloc(buf, sizebuf += 1);

    buf[curbuf++] = '\0';
    buf = realloc(buf, sizebuf = curbuf);

    if (curlist > sizelist - 1)
        lst = realloc(lst, (sizelist += SIZE) * sizeof(*lst)); 

    lst[curlist++] = buf;
}
    
int is_word_symbol(int cur_char) {
    cur_char =tolower(cur_char);
    char *normal_sym_link = index(word_char_array,cur_char);

    //if symbol!= eof and can be found in array of word symbols - return true

    if (cur_char!=EOF && normal_sym_link!=NULL)
        return 1;

    else
        return 0;
}

int get_lexem_list_len(list_type lst){
    int len = 0;
    char *list_elem;
    if (lst==NULL)
        return 0;


    while ((list_elem = lst[len])!=NULL){
        if (strcmp(list_elem,"#") == 0) //all lexems after # are ignored
            break;
        
        len++;
    }

    return len;
}

void print_lexem_list(list_type lst) {
    int i;

    if (lst == NULL)
        return;

    for (i = 0; i < get_lexem_list_len(lst); i++)
        printf("%s\n", lst[i]);
}

//this function checks pairing of double quote
list_type create_lexem_list(status *program_status){
	int cur_char,prev_char,sym_in_string,sym_screened;    
    sym_in_string= sym_screened = 0;

    null_list();
    vertex V = Start;
    cur_char = getchar();

    while (1) {
        switch (V) {

        case Start:
            switch(cur_char){
                case ' ':case '\t':
                    cur_char = getchar();
                    break;

                case EOF:
                    if (*program_status!=Error)
                        *program_status = Finish;
                    termlist();
                    return lst;

                case '\n':
                    if (*program_status!=Error)
                        *program_status = Success;
                    termlist();
                    return lst;

                case '&': case '|': case '>':
                    nullbuf(); 
                    addsym(cur_char);
                    V=Pairable_sym;
                    prev_char = cur_char;
                    cur_char=getchar();
                    break;

                case '"':
                    sym_in_string = 1;
                    nullbuf();
                    V = Word;
                    cur_char =getchar();
                    break;

                // used to screen linker lexems
                case '\\':
                    nullbuf();
                    addsym(cur_char);
                    addword();
                    cur_char = getchar();
                    V = Word;
                    sym_screened = 1;
                    nullbuf();
                    break;

                /*Single_special_characters - special character that can't be doubled, for example '<'*/
                case '<': case '(': case ')': case ';' : case '#':
                    nullbuf();
                    addsym(cur_char);
                    addword();
                    cur_char =getchar();
                    break;

                //processing of word symbols 
                //in this section error is raised in case symbol doesn't belong to word symbol set
                default:            
                    nullbuf();
                    V = Word;
                    break;
                        
            }
            break; 

        //in this node words and quoted char sequences are processed
        //is we are working with quoted sequence, is_word_symbol check isn't carried out!
        case Word:
            if ( (cur_char == EOF) || (cur_char == '\n')){
                addword();
                if (sym_in_string){
                    printf("ERROR: double quote balance error");
                    *program_status = Error;
                }
                V = Start;
                continue;
            }
            
            //if \ was inputted before cur_char
            if (sym_screened){

                sym_screened = 0;
                addsym(cur_char);
                cur_char = getchar();
                continue;
            }

            if (cur_char == '\\'){
                sym_screened = 1;
                cur_char = getchar();
                continue;
            }

            if (sym_in_string){

                if (cur_char == '"'){
                    sym_in_string = 0;
                    addword();
                    V = Start;
                }
                
                else
                    addsym(cur_char);
                
                cur_char = getchar();
                continue;
            }

            if (is_word_symbol(cur_char)) {
                addsym(cur_char);
                cur_char = getchar();
            }
            else{
                addword();
                V = Start;
            }

            break;
                
        case Pairable_sym:
            if (prev_char == cur_char){
                addsym(cur_char);
                cur_char = getchar();
            }

            addword();
            V = Start; 
            break; 
        }
    }
}
//returns 1 if lex = lst[index] is screeened
int bracket_screened(list_type lst,int index){
    if (index == 0)
        return 0;

    if (strcmp(lst[index-1],"\\") == 0)
        return 1;
    else 
        return 0;
}

//returns -1 if brackets are balanced
int check_bracket_balance(list_type lst){
    int br_count[3];
    int br_counter = 0;
    int br_balance = 0;
    int list_len = get_lexem_list_len(lst);
    for (int i =0;i<list_len;i++){
        if (strcmp(lst[i],"(")== 0){
            br_count[br_counter]=i;
            br_counter++;
            if (!bracket_screened(lst,i))
                br_balance++;
        }
        if (strcmp(lst[i],")")== 0){
            br_count[br_counter]=i;
            br_counter++;
            if (!bracket_screened(lst,i))
                br_balance--;
        }

        if (br_balance<0)
            return 0;
    }
    if (br_count[0] == 0)
        br_counter = 0;
    if (br_balance>0)
        return 0;
    else
        return 1;
}


list_type get_lexem_list(status *program_status){
    int brackets_balanced;
    list_type lexem_list = create_lexem_list(program_status);
    //print_lexem_list(lst); 

    brackets_balanced= check_bracket_balance(lexem_list);
    if (!brackets_balanced)
        *program_status = Error;
    return lexem_list;
}
