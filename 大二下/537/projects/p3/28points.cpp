
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>
#include <errno.h>
#define LINE_LENGTH 520
#define MAX_BG 32
#define BUFFER_SIZE 1024
char prompt[6] = "mysh> ";
char* command[900];
char* words[516];
static const char INCORRECT_NUM_ARG[] = "Usage: mysh [batch-file]\n";
static const char TOO_LONG_NUM_ARG[] = "Command line maximum length is 512\n";
int is_interactive = 0;
char error[105] = "Redirection misformatted.\n";
int is_batch = 0;
int split_line(char* line, char** words) {
        int count = 0;
        char* input = strdup(line);
        while(1){
                while(isspace(*input)) {
                    input++;
                }
                if(*input == '\0') {
                    return count;
                }
                words[count++] = input;
                while(!isspace(*input) && *input != '\0'){
                    input++;
                }
                if(*input == '\0'){
                    return count;
                }
                *input = '\0';
                input++;
        }
}
char* get_value(char* line, char** words) {
        int count = 0;
        char* input = strdup(line);
        while(1){
                if(count == 2){
                    return input;
                }
                while(isspace(*input)) {
                    input++;
                }
                if(*input == '\0') {
                    return NULL;
                }
                words[count++] = input;
                while(!isspace(*input) && *input != '\0'){
                    input++;
                }
                if(*input == '\0'){
                    return NULL;
                }
                *input = '\0';
                input++;
        }
}
typedef struct Alias alias_t;
typedef struct Alias* alias_ptr_t;

// singly linked list to hold a list of alias names and commands
struct Alias {
    char* name;
    char* command;
    alias_ptr_t next;
};
// returns a pointer to a heap allocated alias struct
// filled with a copy of the passed name and command
alias_ptr_t alias_create(char* name, char* command) {
    alias_ptr_t new_alias_ptr = (alias_ptr_t) malloc(sizeof(alias_t));
    *new_alias_ptr = (alias_t) {.name = name,.command = command,.next = NULL};
    return new_alias_ptr;
}

// frees the particular alias struct
void alias_free(alias_ptr_t alias_ptr) {
    if(alias_ptr != NULL) {
        free(alias_ptr->name);
        free(alias_ptr->command);
        *alias_ptr = (alias_t) {.name = NULL,.command = NULL,.next = NULL};
        free(alias_ptr);
    }
}
// recursively frees the entire alias data structure with the
// passed argument as the head of the list
alias_ptr_t alias_destroy(alias_ptr_t alias_ptr) {
    if(alias_ptr) {
        alias_destroy(alias_ptr->next);
        alias_free(alias_ptr);
    }
    return NULL;
}
// removes elements from the alias list that have a matching name
// and returns the new head of the list
alias_ptr_t alias_remove(alias_ptr_t alias_ptr, const char* name) {
    alias_ptr_t iterator = alias_ptr;
    if(alias_ptr == NULL) {
        // empty list
        return NULL;
    }
    if(strcmp(alias_ptr->name, name) == 0) {
        // first element matches
        alias_ptr_t next = alias_ptr->next;
        alias_free(alias_ptr);
        return next;
    }
    while(iterator->next != NULL) {
        if(strcmp(iterator->next->name, name) == 0) {
            alias_ptr_t next = iterator->next->next;
            alias_free(iterator->next);
            iterator->next = next;
            break;
        }
        iterator = iterator->next;
    }
    return alias_ptr;
}
// adds a new (name, command) pair to the list of aliases
// by first removing any element with the passed name and
// returns the new head of the list of aliases
alias_ptr_t add(alias_ptr_t ptr, const char* name, const char* command) {
    // substitude value if alias exits
    ptr = alias_remove(ptr, name);
    alias_ptr_t new_alias_ptr = alias_create(strdup(name), strdup(command));
    if(ptr == NULL) {
        // list was empty
        return new_alias_ptr;
    }
    // going to the end of the alias list
    alias_ptr_t end_ptr = ptr;
    while(end_ptr->next) {
        end_ptr = end_ptr->next;
    }
    // adding new_alias_ptr to the end
    end_ptr->next = new_alias_ptr;
    return ptr;
}
char* concat(const char *s1, const char *s2) {
    char *result = malloc(strlen(s1) + strlen(s2)+2); // +1 for the null-terminator
    // in real code you would check for errors in malloc here
    strcpy(result, s1);
    strcat(result," ");
    strcat(result, s2);
    return result;
}
// recursively displays the entire alias data structure
void alias_display(const alias_ptr_t alias_ptr) {
    if (alias_ptr == NULL) {
        return;
    } else {
        // printf("%s \"%s\"\n", alias_ptr->name, alias_ptr->command);
        char* s = concat(alias_ptr->name, alias_ptr->command);
        // do things with s
        // char* cmd = s;
        //int i = 0;
        //char input[20];
        //char* end = "";
        //while (strcmp(s, end) != 0) {
          //  input[i] = *s;
            //s += 1;
           // i += 1;
        //}
        //free(s);  // deallocate the string
        //write(STDOUT_FILENO, input, strlen(input));
        fprintf(stdout, "%s",s);
        fflush(stdout);

        alias_display(alias_ptr->next);
    }
}

// recursively searches for a command with the passed name
// in the entire alias data structure and returns NULL if
// not found
alias_ptr_t alias_query(const alias_ptr_t alias_ptr, const char* name) {
    if(alias_ptr == NULL) {
        // not found
        return NULL;
    }
    if(strcmp(alias_ptr->name, name) == 0) {
        // match found
        return alias_ptr;
    }
    // search in the rest
    return alias_query(alias_ptr->next, name);
}
// executes alias commands
alias_ptr_t execute_alias_command(char* command, alias_ptr_t alias_ptr) {
    // int incorrect_usage = 0;
    char* alias_line[515];
    int cnt = split_line(command, alias_line);
    char long_arg[50] = "unalias: Incorrect number of arguments.\n";
    char danger[50] = "alias: Too dangerous to alias that.\n";
    // ERROR case: alias alias, unalias or exit
    if (cnt > 1 && strncmp(alias_line[1], "alias", strlen("alias")) == 0 ) {
        write(STDERR_FILENO, danger,strlen(danger));
            return NULL;
    } else if (cnt > 1 && strncmp(alias_line[1], "unalias", strlen("unalias")) == 0) {
        write(STDERR_FILENO, danger,strlen(danger));
            return NULL;
    } else if (cnt > 1 && strncmp(alias_line[1], "exit", strlen("exit")) == 0) {
            write(STDERR_FILENO, danger,strlen(danger));
            return NULL;
    }
    // case1: alias + word
    if(strncmp(alias_line[0], "alias", strlen("alias")) == 0 && cnt == 1) {
           alias_ptr_t display = alias_ptr;
           if (alias_ptr) {
              alias_display(alias_ptr);
           } else {
                return alias_ptr;
            }
            //return alias_ptr;   
        
    } else if (strncmp(alias_line[0], "unalias", strlen("unalias")) == 0) {
        // case2: unalias
        // alias_ptr_t remove = alias_ptr;
        if(cnt != 2) {
            //not specify <alias-name> or too many arguments
            write(STDERR_FILENO, long_arg, strlen(long_arg));
            return alias_ptr;//just continue
        } else if (alias_query(alias_ptr, alias_line[1])) {
            // get the alias and remove it
            // char alias_name[BUFFER_SIZE];
            // sscanf(command, "unalias %s", alias_name);
            alias_ptr = alias_remove(alias_ptr, alias_line[1]);
            return alias_ptr;
        } else {   
            // <alias-name> does not exist as an alias
            // just continue
            return alias_ptr;
        }
    } else if(strncmp(alias_line[0], "alias", strlen("alias")) == 0 && cnt == 2) {  // case: display aliases
        alias_ptr_t query = alias_query(alias_ptr, alias_line[1]);
            if(query != NULL) {
                //if(alias_ptr) {
                char* alias_name_1;
                char* alias_command_1;
                // start of alias name
                alias_name_1 = query->name;
                alias_command_1 = query->command;
                char* all = concat(alias_name_1, alias_command_1);
                write(STDOUT_FILENO, all, strlen(all));
                //write(STDOUT_FILENO, alias_line[1], strlen(alias_line[1]));
                   // printf(stdout,"%s %s", query->name, query->command);
                   // fflush(stdout);
                //}
        }else{
            return alias_ptr;
        }
    } else {
        // case4: add alias to list
        // set alias
        char* alias_name;
        char* alias_command;
        // start of alias name
        alias_name = alias_line[1];
        char* value[515];
        alias_command = get_value(command, value);
        // alias_command = value[2];
        //char* try = strdup(command);
        //printf("Tokens:\n");
        //char *token = strtok(str, " \t\n");     // initial call: give a pointer to original string
        //while (token != NULL) {                 //               , and a string of delimiters
          //  printf("%s\n", token);
           // token = strtok(NULL, " \t\n");      // subsequent calls: give NULL as the first arg
       // }
        alias_ptr = add(alias_ptr, alias_name, alias_command);
    }
    return alias_ptr;
}
// executes non-alias-prefixed commands using system
//void execute_other_command(char* command, const alias_ptr_t alias_ptr) {
   // alias_ptr_t query = alias_query(alias_ptr, command);

   // if (query != NULL) {
      //  command = query;
    //}
  //  system(command);
//}
void print_error() {
    write(STDERR_FILENO, error, strlen(error));
}

void print_prompt() {
    write(STDOUT_FILENO, prompt, strlen(prompt));
}
int main(int argc, char** argv) {
    int fd_stdout;
    pid_t child;
    char input[LINE_LENGTH];
    FILE *input_file = NULL;
    int is_built_in = 0;
    int status;
    // declaring the alias
    alias_ptr_t alias_ptr = NULL;
    // declaring the input buffer
    char command[BUFFER_SIZE];
    // check argc
    if (argc == 1) {
        //interactive mode
        is_interactive = 1;
        input_file = stdin;
    }
    else if (argc == 2) {
        //batch mode
        is_interactive = 0;
        input_file = fopen(argv[1], "r");
        if(input_file == NULL) {
            char open_error[256];
            sprintf(open_error, "Error: Cannot open file %s.\n", argv[1]);
            write(STDERR_FILENO, open_error, strlen(open_error));
            exit(1);
            //fprintf(stderr, "Error: Cannot open file %s\n.", argv[1]);
            //fflush(stderr);
            //return 1;
        }
    }
    else {
        // invalid arguments number
        write(STDERR_FILENO, INCORRECT_NUM_ARG, strlen(INCORRECT_NUM_ARG));
        exit(1);
    }
    if (is_interactive) {
        print_prompt();
    }
    // evaluate and execute lines
    while(fgets(input, LINE_LENGTH, input_file)) {
        //fprintf(stdout,"%s", input);
        //fflush(stdout);
        if (!is_interactive) {
            write(STDOUT_FILENO, input, strlen(input));
        }
        
        // if input too large
	if(strlen(input) > 512){
	    write(STDERR_FILENO, TOO_LONG_NUM_ARG, strlen(TOO_LONG_NUM_ARG));
	    if(is_interactive) print_prompt();
                continue;
        }
        char* tmp = strdup(input);
        int num_word = split_line(input, words);
        // case: empty input
        if(num_word == 0) {
	    if(is_interactive) {
               print_prompt();
            }
            continue;
	}       
        // check redirection
        char* pre_token = strtok(tmp, ">");
        char* post_token = NULL;
        // has sign ">"
	if(strlen(pre_token) != strlen(input)) {
            // no word before ">"
            char* tmp_words_2[50];
            int count_2 = split_line(pre_token, tmp_words_2);
            if (count_2 == 1) {
                print_error();
                if(is_interactive) {
                    print_prompt();
                }
                continue;
            }
            post_token = strtok(NULL, ">");
            char* tmp_words_1[50];
            int count = split_line(post_token, tmp_words_1);
            //case: more than one redirection operators
	    if(strtok(NULL, ">")) {
		print_error();
		if(is_interactive) {
                    print_prompt();
                }
                continue;
	    }
            // case: no file or more than one file
            if(count != 1) {  
                print_error();
                if (is_interactive) {
                    print_prompt();
                }
                continue;
            }
            // implement redirection
            fd_stdout = dup(1);
      int fd = open(words[num_word-1], O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR);
            if (fd < 0) {
                print_error();
		if (is_interactive) {
                    print_prompt();
                }
                continue;
            }
            else if(dup2(fd, 1) < 0) {
                print_error();
		if(is_interactive) {
                    print_prompt();
                }
                continue;
            }
		num_word -= 2; 
            }	
            // removing newline
        words[num_word] = NULL;
        // build-in commands		
        // exit
        if(strcmp("exit", words[0]) == 0 && num_word == 1) {
                is_built_in = 1;
                exit(0);
        }
        // alias
        // words[num_word] = '\0';
        // routing commands
        if(strcmp("alias", words[0]) == 0 || strcmp("unalias", words[0]) == 0) {
            is_built_in = 1;
            alias_ptr = execute_alias_command(input, alias_ptr);
           // if(alias_ptr == NULL) {
            //    continue;
            //}
        }
        // Non build-in commands
        if(!is_built_in) {
            child = fork();
            if(child == 0) {	
                execv(words[0], words);
                char error_info[50];
                sprintf(error_info, "%s: Command not found.\n", words[0]);
                write(STDERR_FILENO, error_info, strlen(error_info));
                _exit(1);
            } else if (child == (pid_t)(-1)) {
                print_error();
            } else {
                waitpid(child, &status, 0);
            }
	}
        dup2(fd_stdout, 1); //redirect back
        if(is_interactive) {
            print_prompt();
        }
        }
        // destroying the alias
        alias_ptr = alias_destroy(alias_ptr);
        return 0;
}

