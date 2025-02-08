#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <fcntl.h>

#define ESC				"\x1b"

/* editor modes */
#define NORMAL_MODE			0x0
#define INSERT_MODE			0x1
#define CMD_LINE_MODE			0x2

struct text_buffer_struct {
	char *base;
	int pos;	/* position in text buffer */
} text_buffer;

struct cmdline_cmd_struct {
	char cmd[100];
	int pos;
} cmdline_cmd;

void enable_raw_mode();
void disable_raw_mode();
char getch();
void write_text(void);
void free_text_buffer(void);
void get_cmd_line_cmd(void);
void move_cursor_to_bottom_left(void);
void process_cmdline_cmd(void);

int main(void)
{
    	enable_raw_mode();
	if(atexit(disable_raw_mode) != 0) {	/* Automatically restores terminal settings when the program exits. */
		printf("atexit() function registration failed");
		exit(1);
	}

	atexit(free_text_buffer);

	/* Disable output buffering so characters appear immediately */	
	setvbuf(stdout, NULL, _IONBF, 0);

	text_buffer.base = (char *) malloc(2400);	/* allocate text buffer */
	if(text_buffer.base == NULL) {
		printf("error: couldn't allocate text buffer!\n");
		exit(1);
	}

	text_buffer.pos = 0;

	/* initialize the commandline command position variable */
	cmdline_cmd.pos = 0;

	/* erase the screen with the background color and move the cursor to the top-left corner */
	printf(ESC "[2J" ESC "[H");

	int mode = NORMAL_MODE;	/* editor mode */

	/* by default, we are in the normal mode. Wait for a command. */
	while(true) {
		char cmd = getch();

		if(cmd == 'i') {	/* insert command */
		 	mode = INSERT_MODE;
			write_text();
		 	mode = NORMAL_MODE;
		} else if(cmd == ':') {	/* command-line mode */
			mode = CMD_LINE_MODE;	
			get_cmd_line_cmd();
			process_cmdline_cmd();
			mode = NORMAL_MODE;
		}
	}

	return 0;
}

void move_cursor_to_bottom_left(void) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); // Get terminal size
    printf("\x1b[%d;1H", w.ws_row); // Move to last row, column 1
}

void process_cmdline_cmd(void)
{
	char filename[100];

	if(cmdline_cmd.cmd[0] == 'w') {	/* write command */
		/* get the filename to save file */
		strncpy(filename, cmdline_cmd.cmd + 2, cmdline_cmd.pos);

		/* terminate text buffer with newline */
		text_buffer.base[text_buffer.pos++] = '\n';

		/* copy text buffer into file */
		/* create or open the file */
		int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		write(fd, text_buffer.base, text_buffer.pos);

		close(fd);
	}
}

void get_cmd_line_cmd(void)
{
	move_cursor_to_bottom_left();
	printf(":");

	cmdline_cmd.pos = 0;

	while(true) {
		char ch = getch();


		if(ch == '\n') {	/* enter is pressed */
			cmdline_cmd.cmd[cmdline_cmd.pos++] = '\0';
			break;
		}

		cmdline_cmd.cmd[cmdline_cmd.pos++] = ch;

		putchar(ch);
	}
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
			text_buffer.pos--;
		} else {
			putchar(ch);
			text_buffer.base[text_buffer.pos] = ch;
			text_buffer.pos++;
		}
	}
}

void free_text_buffer(void)
{
	free(text_buffer.base);
}
