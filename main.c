#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

#define ESC				"\x1b"

/* editor modes */
#define COMMAND_MODE			0x0
#define INSERT_MODE			0x1

void enable_raw_mode();
void disable_raw_mode();
char getch();
void write_text(void);

int main(void)
{
    	enable_raw_mode();
	if(atexit(disable_raw_mode) != 0) {	/* Automatically restores terminal settings when the program exits. */
		printf("atexit() function registration failed");
		exit(1);
	}

	/* Disable output buffering so characters appear immediately */	
	setvbuf(stdout, NULL, _IONBF, 0);

	/* erase the screen with the background color and move the cursor to the top-left corner */
	printf(ESC "[2J" ESC "[H");

	int mode = COMMAND_MODE;	/* editor mode */

	/* by default, we are in the command mode. Wait for a command. */
	while(true) {
		char cmd = getch();

		if(cmd == 'i') {	/* insert command */
		 	mode = INSERT_MODE;
			write_text();
		 	mode = COMMAND_MODE;
		}
	}

	return 0;
}

/* enable raw mode (disable line buffering) */
void enable_raw_mode() {
	struct termios raw;
	tcgetattr(STDIN_FILENO, &raw);	/* get current terminal settings */
	raw.c_lflag &= ~(ICANON | ECHO); /* disable canonical mode (line buffering) and echo */
	tcsetattr(STDIN_FILENO, TCSANOW, &raw);	/* apply changes immediately */
}

/* restore terminal settings */
void disable_raw_mode() {
	struct termios original;
	tcgetattr(STDIN_FILENO, &original);
	original.c_lflag |= (ICANON | ECHO);	/* re-enable normal mode */
	tcsetattr(STDIN_FILENO, TCSANOW, &original);
}

/* get a single character without pressing Enter */
char getch() {
	char ch;
	read(STDIN_FILENO, &ch, 1);
	return ch;
}

void write_text(void)
{
	while(true) {
		char ch = getch();

		if(ch == 27) {	/* esc key exits insert mode */
			break;
		} else if(ch == 127) {	/* handle backspace */
			printf("\b \b");	/* Move back, erase character */
		}
		putchar(ch);
	}
}
