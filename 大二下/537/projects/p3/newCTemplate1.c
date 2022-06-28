#ifndef __MY_ALIAS_H_
#define __MY_ALIAS_H_
struct Alias *head = NULL;
struct Alias *cur = NULL;
struct Alias *pre = NULL;

// singly linked list to hold a list of alias names and commands
struct Alias {
    char name[128];
    char value[512];
    struct Alias *next;
};

extern void print_prompt();
extern int split_line(char* line, char** words);
extern void alias_display();
extern int is_exist(char* alias);
extern void insertAlias(char *alias, char *cmd);
extern void removeAlias(char *alias);
extern char* get_value(char *alias);
extern void execute_alias_command(char *input);
extern void exe_loop(FILE *fp, char *input);
extern void cmd_execute(char *cmd[]);
extern void cmd_redirection_execute(char *cmd[], char *fileName[]);

#endif