#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h> 
#include <unistd.h>

#include <sys/types.h>
#include <sys/wait.h>

#include <fcntl.h>

#include <termios.h>

#include <dirent.h>

//#include <time.h>


#define MAX_SIZE 1024
#define MAX_MATCHES 64

bool EchoFunction(char **args);
bool BuiltinFunction(char **args, int fd, int target_fd);
bool TypeFunction(char *input);
bool ProgramFunction(char **args, int fd, int target_fd);
bool PWDFunction();
bool CDFunction(char *input);

char** ParseInput(char *input);
void FreeArgs(char **args);

int CheckOutputRedirect(char **args, int *target_fd);
void RestoreStd(int fd, int saved_std, int target_fd);

int HandleTabCompletion(char *input, int i, int tab_counter);
void CheckBuiltinMatches(char *input, int i, char matches[][MAX_SIZE], int *match_count);
void CheckPathMatches(char *input, int i, char matches[][MAX_SIZE], int *match_count);
int ResolveCompletion(char *input, int i, char matches[][MAX_SIZE], int match_count, int tab_counter);
int LongestCommonPrefix(char matches[][MAX_SIZE], int match_count);

struct termios orig_termios;
bool is_interactive_global;

void RestoreTerminal(void) {
    if (is_interactive_global) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
}

//int argc, char *argv[]
int main() {
    char input[MAX_SIZE];
    char ch;

    bool is_interactive = isatty(STDIN_FILENO); //is fd refering to terminal (tty) or something else
    is_interactive_global = is_interactive;
    
    struct termios raw;
    if (is_interactive) {
        tcgetattr(STDIN_FILENO, &orig_termios);
        raw = orig_termios;                       // Copy for returing terminal to canonical mode
        raw.c_lflag &= ~(ECHO | ICANON);          // Disable Canonical Mode
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
        atexit(RestoreTerminal);    
    }

  
    while(1) {
        setbuf(stdout, NULL);
        printf("$ ");

        if(is_interactive) {
            int i = 0;
            ch = '\0';
            int tab_counter = 0;

            while (ch != '\n') {
                read(STDIN_FILENO, &ch, 1);

                if (ch == 9) { //tab
                    tab_counter++;
                    i = HandleTabCompletion(input, i, tab_counter);
                    continue;
                }
                else if (ch == 27) {  // Start of esc sequence, cant move with arrows freely trough terminal
                    char seq[2];
                    read(STDIN_FILENO, &seq[0], 1);
                    read(STDIN_FILENO, &seq[1], 1);
                    // za sad: samo ignoriši celu sekvencu, ne radi ništa
                    continue;
                }
                else if(ch == 127) { //backspace
                    if (i > 0) {
                        i--;
                        input[i] = '\0';
                        printf("\b \b");
                        fflush(stdout);

                        tab_counter = 0;
                    }
                } else {
                    if (i < MAX_SIZE - 1) { 
                        input[i++] = ch;
                        input[i] = '\0';    
                        printf("%c", ch);
                        
                        tab_counter = 0;
                    }
                }
                //printf("%d\n", ch);
            }
        } else {
            if (fgets(input, MAX_SIZE - 1, stdin) == NULL ) {
                break; //EOF or error 
            }
        }
        input[strcspn(input, "\n")] = '\0';


        
        //if input is blank or only spaces
        bool only_white_spaces = true;
        for (int i = 0; input[i] != '\0'; i++) {
            if (input[i] != ' ') {
                only_white_spaces = false;
                break;
            }
        }
        if (only_white_spaces) {continue;}
        

        char **args = ParseInput(input);
        int target_fd = 1;
        int fd = CheckOutputRedirect(args, &target_fd);

        if (BuiltinFunction(args, fd, target_fd)) {
            FreeArgs(args);
            continue;
        }
        else if(ProgramFunction(args, fd, target_fd)) {
            FreeArgs(args);
            continue;
        }

        printf("%s: command not found\n", input);

        if (fd != -1) close(fd); //if neither builtin nor program func closes it
        FreeArgs(args); //freeing the memory from func ParseInput
    }

    return 0;
}

bool EchoFunction(char **args) {

    for(int i = 1; args[i] != NULL; i++) {
        printf("%s", args[i]);

        if (args[i + 1] != NULL) printf(" ");
    }

    printf("\n");
    return true;
}

bool BuiltinFunction(char **args, int fd, int target_fd) {
    bool is_builtin = (strcmp(args[0], "exit") == 0 ||
                        strcmp(args[0], "echo") == 0 ||
                        strcmp(args[0], "type") == 0 ||
                        strcmp(args[0], "pwd")  == 0 ||
                        strcmp(args[0], "cd")   == 0);

    if (!is_builtin) {
        return false; // fd ostaje netaknut, ProgramFunction ga koristi
    }

    bool result = false;
    
    int saved_std = -1;
    if( fd != -1) {  //fd = fd of file
        saved_std = dup(target_fd); //target fd 1 or 2exo
        dup2(fd, target_fd); //
    }

    if (strcmp(args[0], "exit") == 0) {
        exit(0);
    }
    else if (strcmp(args[0], "echo") == 0) {
        result = EchoFunction(args);
    } 
    else if (strcmp(args[0], "type") == 0) {
        result = TypeFunction(args[1]);
    } 
    else if (strcmp(args[0], "pwd") == 0)  {
        result = PWDFunction();
    }
    else if (strcmp(args[0], "cd") == 0)  {
        result = CDFunction(args[1]);
    }

    RestoreStd(fd, saved_std, target_fd);
    return result;
}

bool TypeFunction(char *input) {

    // BUILTIN
    char *builtins[] = {"echo", "exit", "type", "pwd", "cd"};
    int length = sizeof(builtins) / sizeof(builtins[0]);
    
    for(int i = 0; i < length; i++) {
        if (strcmp(builtins[i], input) == 0) {
            printf("%s is a shell builtin\n", input );
            return true;
        }
    }

        
    // CHECKING PATH env var
    char *path_env = getenv("PATH"); // Getting PATH value (String) trough getenv func
    char path_env_cpy[MAX_SIZE];
    strcpy(path_env_cpy, path_env);

    char *all_paths = strtok(path_env_cpy, ":"); 
    
    while(all_paths != NULL) {
        char full_path[MAX_SIZE];
        
        snprintf(full_path, sizeof(full_path), "%s/%s", all_paths, input);

        if(access(full_path, F_OK) == 0 && access(full_path, X_OK) == 0) {
            printf("%s is %s\n", input, full_path);
            return true;
        }

        all_paths = strtok(NULL, ":");
    }
    printf("%s: not found\n", input);
    

    return true;
}

bool ProgramFunction(char **args, int fd, int target_fd) {
    // CHILD PROCESS
    pid_t pid = fork();
    
    if (pid == -1) {
        perror("fork");
        //return false;

    } else if(pid == 0) {  //this does the child process
        if( fd != -1) {
            dup2(fd, target_fd);
        }
        
        execvp(args[0], args);

        //if it gets here it means it failed
        close(fd);
        _exit(127);

    } else {
        if (fd != -1) { close(fd); }

        int status;
        wait(&status);

        if(WIFEXITED(status) && WEXITSTATUS(status) == 127) {
            return false;
        }
    }

    return true;
}

bool PWDFunction() {
    char *cwd = getcwd(NULL, 0);

    if (cwd != NULL) {
        printf("%s\n", cwd);
        free(cwd);
        return true;
    }

    perror("getcwd() error");
    return false;
}

bool CDFunction(char *input) {
 
    if (input == NULL) {
        fprintf(stderr, "cd: missing argument\n");
        return true;
    }

    if (strcmp(input, "~") == 0 ) {
        input = getenv("HOME");
    }
    
    if ( chdir(input) == 0) { //works on relative paths aswell
    } else {
        fprintf(stderr, "cd: %s: No such file or directory\n", input);
    }
    
    return true;
}

char** ParseInput(char *input) {
    int capacity = 10;
    int num_of_args = 0;
    char **arguments = malloc(capacity * sizeof(char *));

    
    char const single_qoute = '\'';
    char const double_qoute = '\"';
    char const backslash    = '\\';

    bool in_single_quotes   = false;
    bool in_double_quotes   = false;
    //bool backslash_escaping = false;

    char arg[MAX_SIZE];
    int i_arg = 0;

    for (int i = 0; input[i] != '\0'; i++) { 

    // Backslash handling. Ponašanje zavisi od konteksta (van navodnika / u duplim / u single),
    // zato je ovo grananje po in_double_quotes umesto jednog opšteg pravila.
    // (in_single_quotes se ovde uopšte ne obrađuje jer je backslash unutar '...' potpuno
    // literalan i hvata ga grana ispod za obične karaktere - zato je !in_single_quotes uslov gore.)
        if (input[i] == backslash && !in_single_quotes) {
            // Unutar "..." backslash escape-uje SAMO ove karaktere: \ " $ `
            // Za sve ostalo (npr. \n, \t, obično slovo) backslash ostaje literalan
            // i sledeći karakter se NE preskace ovde - obradiće se normalno
            // u narednoj iteraciji petlje.
            if(in_double_quotes) {
                if(input[i+1] == '\\' ||
                   input[i+1] == '\"' ||
                   input[i+1] == '$' ||
                   input[i+1] == '`') {
                    arg[i_arg++] = input[++i];
                } else {
                    arg[i_arg++] = input[i];
                }
            } else {
                if (input[i+1] != '\0') {
                    arg[i_arg++] = input[++i];
                } else {
                    // backslash je poslednji karakter u inputu - nema šta da escape-uje,
                    // dalje parsiranje bi čitalo leftover smeće iz starog input bafera
                    break;
                }
            }

            continue;
        }

        if (input[i] == single_qoute && !in_double_quotes) { //inside double quotes treated like literal
            in_single_quotes = !in_single_quotes;
            continue;
        } else if (input[i] == double_qoute  && !in_single_quotes) {
            in_double_quotes = !in_double_quotes;
            continue;
        }

        if (input[i] == ' ' && !in_single_quotes && !in_double_quotes) {
            if (i_arg > 0) {

                arg[i_arg] = '\0';
                arguments[num_of_args] = malloc(strlen(arg) + 1); //strdup does this line and line below
                strcpy(arguments[num_of_args], arg);
                num_of_args++;
                strcpy(arg, "");
                i_arg = 0;
         
         
                if (num_of_args >= capacity - 1) {
                    capacity *= 2;
                    arguments = realloc(arguments, capacity * sizeof(char *));
                }
         
            }

            continue;
        }

        arg[i_arg++] = input[i];
    }

    if(i_arg > 0) {
        arg[i_arg] = '\0';
        arguments[num_of_args] = malloc(strlen(arg) + 1); //strdup does this line and line below
        strcpy(arguments[num_of_args], arg);
        num_of_args++;
    }

    // for (int i = 0; i < num_of_args; i++) {
    //     printf("%s|\n", arguments[i]);
    // }
    
    arguments[num_of_args] = NULL;
    return arguments;
}

void FreeArgs(char **args) {
    for (int i = 0; args[i] != NULL; i++) {
        free(args[i]);
    }
    free(args);
}

int CheckOutputRedirect(char **args, int *target_fd) {
    for (int i = 0; args[i] != NULL; i++) {
        if(( strcmp(args[i], ">") == 0 || strcmp(args[i], "1>") == 0 ) && args[i+1] != NULL) {
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644); //read only, create if doesnt exist, truncate if it exists, para for create
            
            args[i] = NULL;
            args[i+1] = NULL;

            *target_fd = 1;  //stdout
            return fd;  
        }
        else if(( strcmp(args[i], "2>") == 0) && args[i+1] != NULL) {
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644); //read only, create if doesnt exist, truncate if it exists, para for create
            
            args[i] = NULL;
            args[i+1] = NULL;
            
            *target_fd = 2;  //stderr
            return fd;
        }
        else if((strcmp(args[i], ">>") == 0 || strcmp(args[i], "1>>") == 0) && args[i+1] != NULL) {
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644); //read only, create if doesnt exist, truncate if it exists, para for create
            
            args[i] = NULL;
            args[i+1] = NULL;
            
            *target_fd = 1;  //stdout
            return fd;
        }
        else if(( strcmp(args[i], "2>>") == 0) && args[i+1] != NULL) {
            int fd = open(args[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644); //read only, create if doesnt exist, truncate if it exists, para for create
            
            args[i] = NULL;
            args[i+1] = NULL;
            
            *target_fd = 2;  //stderr
            return fd;
        }
    }
    return -1;
}

void RestoreStd(int fd, int saved_std, int target_fd) {
    if ( saved_std != -1) {
        dup2(saved_std, target_fd);
        close(fd);
        close(saved_std);
    }
}


int HandleTabCompletion(char *input, int i, int tab_counter) {
    //printf("[DEBUG input='%s' i=%d]", input, i);
    if (i == 0) {
        printf("\a"); //beep
        return i;
    }

    char matches[MAX_MATCHES][MAX_SIZE];
    int match_count = 0;

    CheckBuiltinMatches(input, i, matches, &match_count);
    if (match_count == 1) {
        return ResolveCompletion(input, i, matches, 1, tab_counter);
    }

    
    CheckPathMatches(input, i, matches, &match_count);
    

    //printf("[DEBUG match_count='%d' ]", match_count);
    return ResolveCompletion(input, i, matches, match_count, tab_counter);

}

void CheckBuiltinMatches(char *input, int i, char matches[][MAX_SIZE], int *match_count) {
    
    char *builtins[] = {"echo", "exit", "type", "pwd", "cd"};
    int length = sizeof(builtins) / sizeof(builtins[0]);

    for(int j = 0; j < length; j++) {
        if (strncmp(builtins[j], input, i) == 0) {
            strcpy(matches[*match_count], builtins[j]);
            (*match_count)++;
        }
    }
}

void CheckPathMatches(char *input, int i, char matches[][MAX_SIZE], int *match_count) {
    
    char *path_env = getenv("PATH"); // Getting PATH value (String) trough getenv func
    char path_env_cpy[MAX_SIZE];
    strcpy(path_env_cpy, path_env);

    char *all_paths = strtok(path_env_cpy, ":"); 
    char full_path[MAX_SIZE];

    while(all_paths != NULL) {
        DIR *dir = opendir(all_paths);
        if (dir != NULL) {
            struct dirent *entry;

            while ((entry = readdir(dir)) != NULL) {

                if (strncmp(entry->d_name, input, i) == 0) {
                    
                    snprintf(full_path, sizeof(full_path), "%s/%s", all_paths, entry->d_name);

                    if(access(full_path, F_OK) == 0 && access(full_path, X_OK) == 0) {
                        
                        bool already_seen = false;
                        for (int k = 0; k < *match_count; k++) {
                            if (strcmp(matches[k], entry->d_name) == 0) {
                                //printf("[DEDUP HIT: %s already at index %d]", entry->d_name, k);
                                already_seen = true;
                                break;
                            }
                        }

                        if(!already_seen && *match_count < MAX_MATCHES) {
                            strcpy(matches[*match_count], entry->d_name);
                            (*match_count)++;
                            
                        }

                    }
                }
            }
            
            closedir(dir);
        }

        all_paths = strtok(NULL, ":");
    }
}

int ResolveCompletion(char *input, int i, char matches[][MAX_SIZE], int match_count, int tab_counter) {
    
    if(match_count == 0) {
        printf("\a"); //beep
        return i;
    }
    else if (match_count == 1) {
        strcpy(input, matches[0]);
        strcat(input, " ");
        printf("%s ", matches[0] + i); 
        return strlen(input);
    
    } else {
        
        if (tab_counter % 2 == 0) {
            printf("\n");
            for(int k = 0; k < match_count; k++) {
                printf("%s  ", matches[k]);
            }
            printf("\n$ %s", input);

        } else {
            int ii = LongestCommonPrefix(matches, match_count);
            strcpy(input, matches[0]);
            printf("%s", matches[0] + i); 

            return ii;
        }

        return i;
    }
}

int LongestCommonPrefix(char matches[][MAX_SIZE], int match_count) {
    char LCP[MAX_SIZE];
    strcpy(LCP, matches[0]); // start with first candidate 

    for(int k = 1; k < match_count; k++) {
        
        int j = 0;
        while (LCP[j] != '\0' && matches[k][j] != '\0' && LCP[j] == matches[k][j]) {
            j++;
        }
        
        LCP[j] = '\0';
        
    }
    
    strcpy(matches[0], LCP);
    return strlen(LCP);
}
