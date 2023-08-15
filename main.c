#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_INPUT_BUFFER 4096
#define printi(x) printf("%d\n", x)
#define printc(x) printf("%c\n", x)
#define GO_UP printf("\033[A")
#define GO_DOWN printf("\033[B")
#define GO_RIGHT printf("\033[C")
#define GO_LEFT printf("\033[D")

static struct termios old_termios;
int end = 1;

/*
 * reset color changes 
 * reset all terminal changes to its previous state
 * */

void reset_terminal(){
    printf("\033[m");
    fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

/*
 * save the old state of the terminal so we can reset
 * turn off the echo and  non-canonical mode
 *
 * */

void configure_terminal(){
    
    tcgetattr(STDIN_FILENO, &old_termios);
    struct termios new_termios = old_termios;
   
    new_termios.c_lflag &= ~(ICANON | ECHO);
    new_termios.c_cc[VMIN] = 1;
    new_termios.c_cc[VTIME] =  0;

    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    atexit(reset_terminal);
}

void signal_handler(__attribute__((unused)) int signum){
    end = 0;
}

int main(int argc, char **argv){
    (void)argc; (void)argv;
   

    signal(SIGINT, signal_handler);
    configure_terminal();
    char c;
    while(end){
        c = 0;
        read(STDIN_FILENO, &c, 1);
        if(c == 3) end = 0;
        else if (c > 32) {
            printc(c);
        }
        if(c == 27)
            GO_UP;
    }

    return 0;
}
