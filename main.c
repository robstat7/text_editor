#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>

#define ESC				"\x1b"

/* editor modes */
#define COMMAND_MODE			0x0
#define INSERT_MODE			0x1

int editor_mode = COMMAND_MODE;

void enable_raw_mode();
void disable_raw_mode();

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
	// fflush(stdout);	/* ensures the screen updates immediately. */

	/* by default, we are in the command mode. Wait for a command. */
	char cmd;

	while(true) {
		// cmd = getch();
		// printf(&cmd);


		// if(cmd == 'i') {	/* insert command */
		// 	// printf("%c[2K", 0x1b);	/* erase the entire current line to not print the `i` character */
		// 	putchar(cmd);
		// 	editor_mode = INSERT_MODE;
		// }
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
