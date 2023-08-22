#include <signal.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdint.h>

#define CTRL_KEY(x) ((x) & 0x1f)
#define SPACE 0x20
#define DEL_KEY 0x7f
#define printi(x) printf("%d\n", x)
#define printc(x) printf("%c\n", x)


static struct termios old_termios;
int end = 1, cursorx = 1, cursory = 2;
uint8_t all_sel = 0;
int *arr, len = 0, lines = 0;
char *list[128];

enum KEYS{
    UP = 1001,
    DOWN,
};


void init();
void file_open();

//clear the screen and put cursor on 0,0
void clear(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

//clear the screen print the error and exit
void die(char *s){
    perror(s);
    exit(0);
}

/*
 * reset color changes 
 * reset all terminal changes to its previous state
 * */

void reset_terminal(){
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

/*
 * save the old state of the terminal
 * turn off the echo and non-canonical mode
 * disable ctrl-q, ctrl-z, ctrl-c, ctrl-v signals
 *
 * */

void configure_terminal(){
    
    tcgetattr(STDIN_FILENO, &old_termios);
    struct termios new_termios = old_termios;
   
    new_termios.c_lflag &= ~(ICANON | ECHO | ISIG);
    new_termios.c_cc[VMIN] = 0;
    new_termios.c_cc[VTIME] =  1;

    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    atexit(reset_terminal);
}

/*
 * prints the cursor positions based on cursorx and cursory values
 *
 * */

void print_cursor_positions(){
    
    char buf[32];
    snprintf(buf, sizeof(buf), "\x1b[%d;%dH", cursory + 1, cursorx+ 1);
    
    write(STDOUT_FILENO, buf, strlen(buf));
}

/*
 * Read input
 * 
 * */

int read_key(){
    char c;
    int nread;

    while ((nread = read(STDIN_FILENO, &c, 1)) != 1){
        if(nread == -1) 
            die("read");
    }


    if(c == '\x1b'){
        char key[3] = {0};
        if((read(STDIN_FILENO, key, 3)) == -1) return -1;
        if(key[0] == '['){
            switch (key[1]) {
                case 'A': return UP;
                case 'B': return DOWN;
            }
        }
    }
    return c;
}

void replace(int *a, int *b){
    int temp;
    temp = (*a);
    (*a) = (*b);
    (*b) = temp;
}

int _check(){
    if(len == 0) return 1;
    else{
        for(int i = 0; i < len; i++){
            if(arr[i] == cursory) {
                replace(&arr[i], &arr[len - 1]);
                return 0;
            }
        }
    }
    return 1;
}


void put_x(){
    int check = _check();
    if(!check){
        printc(' ');
        len--;
        arr = realloc(arr, len * sizeof(int));
    }else{
        len++;
        printc('X');
        arr = realloc(arr, len * sizeof(int));
        arr[len - 1] = cursory;
    }

}

void add_new(){
    char s[128];
    reset_terminal(); 
    for(int i = cursory; i < lines; i++) printf("\n");
    printf("\n\n\nAdd new[128ch max]: ");
    fgets(s, 128, stdin);
    size_t s_len = strlen(s);

    if(s_len > 0 && s[s_len - 1] == '\n')
        s[s_len - 1] = '\0';

    FILE * fp;

    if((fp = fopen("lista.txt", "a")) == NULL){
        die("fopen");
    }

    fprintf(fp, "%s\n", s);
    fclose(fp);
    cursory = 2;
}

void replace_s(char *s1, char *s2){
    char temp[128];
    strcpy(temp, s1);
    strcpy(s1, s2);
    strcpy(s2, temp);
}

int check_delete(int ind){
    for(int i = 0; i < len; i++){
        int indeks = arr[i] - 2;
        if(indeks == ind) return 0;
    }
    return 1;
}

void delete_list(){

    if(len == lines && lines != 0){
        FILE *fp = fopen("temp.txt", "w");
        fclose(fp);
        remove("lista.txt");
        rename("temp.txt", "lista.txt");

    }else{
        FILE *fp = fopen("temp.txt", "w");
        for(int i = 0; i < lines; i++){
           if(check_delete(i) == 0)
                list[i] = "";
        }
        remove("lista.txt");
        for(int i = 0; i < lines; i++){
            if(strcmp(list[i], ""))
                fprintf(fp, "%s\n", list[i]);
        }
        fclose(fp);
        rename("temp.txt", "lista.txt");
    }
    cursory = 2;
}

void select_all(){
    if(!all_sel){
        len = lines;
        arr = malloc(len * sizeof(int));
        for(int i = cursory; i < len; i++){
            arr[i] = i;
        }
        lines = 0;
        clear(); 
        all_sel = 1;
        puts("Use ctrl-q or ctrl-c to exit, i to add new, ctrl-a to check/uncheck all, del key to delete\n");
        file_open();

    }else{
        all_sel = 0;
        init();
    }
    cursory = 2;
}

/*
 * Handle input 
 * get terminal size and control user input
 * */
void input_handle(){
    int c = read_key();


    switch (c) {

        case DOWN:
            if(cursory <=  lines) cursory++;
            break;
        case UP:
            if(cursory > 2) cursory--;
            break;
        case CTRL_KEY('c'):
        case CTRL_KEY('q'):
            end = 0;
            break;
        case SPACE:
            put_x();
            break;
       case 'i':
            add_new();
            configure_terminal();
            init();
            break;
        case DEL_KEY:
            delete_list();
            memset(list, 0, sizeof(list));
            init();
            break;
        case CTRL_KEY('a'):
            select_all();
            break;
    }

}

void file_open(){
    FILE *fp;
    char c[1024];
    if((fp = fopen("lista.txt", "r")) == NULL){
        die("fopen");
    }
    
    char x = (all_sel) ? 'X' : ' ';

    while(fgets(c, sizeof(c), fp) != NULL){
        //printf("[ ] ");
        printf("[%c] %s", x,c);
        list[lines] = malloc(strlen(c) + 1);
        strcpy(list[lines], c);
        char *newline = strchr(list[lines], '\n');
        if(newline != NULL) *newline = '\0';
        lines++;
    }
    

    fclose(fp);
}

void init(){
    lines = 0;
    len = 0;
    clear(); 
    puts("Use ctrl-q or ctrl-c to exit, i to add new, ctrl-a to check/uncheck all, del key to delete\n");
    file_open();
}

int main(int argc, char **argv){
    (void)argc; (void)argv; 

    configure_terminal();
    init();

    while(end){
        if(lines == 0){
            all_sel = 0;
            add_new();
            configure_terminal();
            init();
        }
        print_cursor_positions();
        input_handle();
    }
    init();
    return 0;
}
