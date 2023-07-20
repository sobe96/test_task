#include "sillypare.h"
#include "hardcore_northcutt.h"

#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <string.h>


typedef cmd_link tree;
typedef struct cmd_struct node;


void make_shift(int n){
    while(n--)
        putc(' ', stderr);
    return;
}

void print_argv(char **p, int shift){
    char **q=p;
    if(p!=NULL){
        while(*p!=NULL){
            make_shift(shift);
            fprintf(stdout, "argv[%d]=%s\n",(int) (p-q), *p);
            p++;
        }
    }
}

void print_syntax_tree(tree t, int shift){
    char **p;
    if(t==NULL)
        return;

    p=t->argv;

    if(p!=NULL)
        print_argv(p, shift);

    else{
        make_shift(shift);
        fprintf(stderr, "psubshell\n");
    }

    make_shift(shift);

    if(t->infile==NULL)
        fprintf(stderr, "infile=NULL\n");
    else
        fprintf(stderr, "infile=%s\n", t->infile);

    make_shift(shift);

    if(t->outfile==NULL)
        fprintf(stderr, "outfile=NULL\n");
    else
        fprintf(stderr, "outfile=%s\n", t->outfile);

    make_shift(shift);

    fprintf(stderr, "append=%d\n", t->append_mode);

    make_shift(shift);


    if (t->linker == And)
        fprintf(stderr, "type=%s\n","AND");
    else if (t->linker == Or)
        fprintf(stderr, "type=%s\n","OR");
    else if (t->linker == Next)
        fprintf(stderr, "type=%s\n","NEXT");
    else if (t->linker == Backgnd)
        fprintf(stderr, "type=%s\n","BACKGND");
    else
        fprintf(stderr, "type=%s\n","NONE");

    make_shift(shift);

    if(t->psubcmd==NULL)
        fprintf(stderr, "psubcmd=NULL \n");

    else{
        fprintf(stderr, "psubcmd---> \n");
        print_syntax_tree(t->psubcmd, shift+5);
    }

    make_shift(shift);

    if(t->next_pipe_cmd==NULL)
        fprintf(stderr, "pipe=NULL \n");

    else{
        fprintf(stderr, "pipe---> \n");
        print_syntax_tree(t->next_pipe_cmd, shift+5);
    }

    make_shift(shift);

    if(t->next==NULL)
        fprintf(stderr, "next=NULL \n");

    else{
        fprintf(stderr, "next---> \n");
        print_syntax_tree(t->next, shift+5);
    }
}
                                                                                                                                                                                                                              
