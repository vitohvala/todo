#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <ctype.h>

#define CTRL_KEY(x) ((x) & 0x1f)
#define SPACE 0x20
#define printi(x) printf("%d\n", x)
#define printc(x) printf("%c\n", x)


static struct termios old_termios;
int end = 1, cursorx = 1, cursory = 2;
int *arr, len = 0;

enum direction{
    UP = 1001,
    DOWN,
    RIGHT,
    LEFT
};


//clear the screen and put cursor on 0,0
void clear(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);
}

//clear the screen print the error and exit
void die(char *s){
    clear();
    perror(s);
    exit(0);
}

/*
 * reset color changes 
 * reset all terminal changes to its previous state
 * */

void reset_terminal(){
    printf("\033[m");
    fflush(stdout);
    clear();
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
   
    new_termios.c_iflag &= ~(IXON);
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
                case 'C': return RIGHT;
                case 'D': return LEFT;
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


/*
 * Handle input 
 * get terminal size and control user input
 * */
void input_handle(){
    int c = read_key();

    struct winsize size;
    
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == -1) {
        die("ioctl");
        end = 0;
    }


    switch (c) {
        case RIGHT:
            if (cursorx < size.ws_col) cursorx++;
            break;
        case DOWN:
            if(cursory < size.ws_row) cursory++;
            break;
        case UP:
            if(cursory) cursory--;
            break;
        case LEFT: 
            if (cursorx) cursorx--;
            break;
        case CTRL_KEY('c'):
        case CTRL_KEY('q'):
            end = 0;
            break;
        case SPACE:
            put_x();
    }

}

void file_open(){
    FILE *fp;
    char c[1024];
    if((fp = fopen("lista.txt", "r")) == NULL){
        die("fopen");
    }
    

    while(fgets(c, sizeof(c), fp) != NULL){
        //printf("[ ] ");
        printf("[ ] %s", c);
    }
    fclose(fp);
}

int main(int argc, char **argv){
    (void)argc; (void)argv; 

    configure_terminal();
    clear(); 
    puts("Use ctrl-q or ctrl-c to exit\n");
    file_open();
    while(end){
        print_cursor_positions();
        input_handle();
    }
    return 0;
}
