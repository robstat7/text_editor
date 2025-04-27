#include <stdio.h>
#include <sys/stat.h>
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
void print_text_buffer(char *filename, int total_bytes);
void print_file_write_info(char *filename, bool file_new, int total_lines, int total_bytes);
void write_mode_line(void);
void undo_mode_line(void);
int get_num_lines_in_buffer(void);
bool file_exists (char *filename);

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
			write_mode_line();
			write_text();
			undo_mode_line();
		 	mode = NORMAL_MODE;
		} else if(cmd == ':') {	/* command-line mode */
			mode = CMD_LINE_MODE;	
			get_cmd_line_cmd();
			process_cmdline_cmd();
			mode = NORMAL_MODE;
		}

		/* normal mode navigation */
		else if(cmd == 'j') {
			printf(ESC "[B");
		}
		else if(cmd == 'k') {
			printf(ESC "[A");
		} else if(cmd == 'l') {
			printf(ESC "[C");
		} else if(cmd == 'h') {
			printf(ESC "[D");
		}

		/* normal mode keybindings */
		else if(cmd == 'd') {
			char cmd = getch();
			if (cmd == 'd') { /* dd - delete current line */
				/* do it in the terminal first */
				printf(ESC "[2K");
				/* TODO: do it in the buffer */
			}
		} else if (cmd == 'g') {
			char cmd = getch();
			if (cmd == 'g') {	/* gg - go to first line */
				printf(ESC "[H");
				text_buffer.pos = 0;
			}
		}
	}

	return 0;
}

void move_cursor_to_bottom_left(void) {
	struct winsize w;
	ioctl(STDOUT_FILENO, TIOCGWINSZ, &w); // Get terminal size
	printf("\x1b[%d;1H", w.ws_row); // Move to last row, column 1

	/* erase the entire current line */
	printf(ESC "[2K");
}

void process_cmdline_cmd(void)
{
	char filename[100];

	if(cmdline_cmd.cmd[0] == 'w') {	/* write command */
		/* get the filename to save file */
		strncpy(filename, cmdline_cmd.cmd + 2, cmdline_cmd.pos);

		/* get the number of lines in the buffer */
		int total_lines = get_num_lines_in_buffer();

		/* terminate text buffer with newline */
		text_buffer.base[text_buffer.pos++] = '\n';

		/* copy text buffer into file */
		/* create or open the file */
		int fd;
		bool file_new = false;
		if(!file_exists(filename)) {	/* if file doesn't exist */
			fd = open(filename, O_WRONLY | O_CREAT, 0644);
			file_new = true;
		} else { /* file already exists */
			fd = open(filename, O_WRONLY | O_TRUNC, 0644);
		}

		int total_bytes = write(fd, text_buffer.base, text_buffer.pos);

		--text_buffer.pos; /* decrement buffer pos after write operation */

		close(fd);

		print_file_write_info(filename, file_new, total_lines, total_bytes);

		/* restore cursor position after a save cursor */
		printf(ESC "[u");
		
	} else if(cmdline_cmd.cmd[0] == 'e') { /* edit command */
		strncpy(filename, cmdline_cmd.cmd + 2, cmdline_cmd.pos);

		/* open file into the editor */
		int fd = open(filename, O_RDONLY, 0644);

		off_t fsize = lseek(fd, 0, SEEK_END);

		lseek(fd, 0, SEEK_SET);	/* we will read file from beginning */

		int total_bytes = read(fd, (void *) text_buffer.base, (size_t) fsize);

		text_buffer.pos = (int) fsize - 1; /* pos begins from 0 */

		close(fd);

		print_text_buffer(filename, total_bytes);
	} else if (cmdline_cmd.cmd[0] == 'q') { /* quit command */
		exit(0);
	}
}

/* print text buffer after opening a file for editing */
void print_text_buffer(char *filename, int total_bytes)
{
	/* erase the screen from the current line (i.e. cmdline) up to the top of the screen */
	printf(ESC "[1J");

	/* move the cursor to the beginning of the bottom line */
	move_cursor_to_bottom_left();

	/* print opened file details in the bottom line */
	printf("\"%s\" %dL, %dB", filename, get_num_lines_in_buffer(), total_bytes);

	/* begin printing the file contents */

	/* move the cursor to upper left */	
	printf(ESC "[H");

	for(int i = 0; i < text_buffer.pos; i++) {
		putchar(text_buffer.base[i]);
	}

	/* place the blinking cursor at upper left */
	printf(ESC "[H");
}

/* print written file details in the bottom line */
void print_file_write_info(char *filename, bool file_new, int total_lines, int total_bytes)
{
	move_cursor_to_bottom_left();

	if (file_new) {
		printf("\"%s\" [New] %dL, %dB written", filename, total_lines, total_bytes);
	} else {
		printf("\"%s\" %dL, %dB written", filename, total_lines, total_bytes);
	}
}

void get_cmd_line_cmd(void)
{
	/* save current cursor pos */
	printf(ESC "[s");

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

void undo_mode_line(void)
{
	/* save current cursor pos */
	printf(ESC "[s");

	move_cursor_to_bottom_left();

	/* restore cursor position after a save cursor */
	printf(ESC "[u");
}

#define COLOR_BOLD  "\e[1m" /* note "\e" is the escape char */
#define COLOR_OFF   "\e[m"

void write_mode_line(void)
{
	/* save current cursor pos */
	printf(ESC "[s");

	move_cursor_to_bottom_left();
	printf(COLOR_BOLD "-- INSERT --" COLOR_OFF);

	/* restore cursor position after a save cursor */
	printf(ESC "[u");
}

int get_num_lines_in_buffer(void)
{
	int total_lines = 1;	/* assign 1 to handle the first line */

	for (int i = 0; i < text_buffer.pos; i++) {
		if (text_buffer.base[i] == '\n') {
			total_lines++;
		}
	}
	return total_lines;
}

/* check if file exists */
bool file_exists (char *filename) {
  struct stat   buffer;   
  return (stat (filename, &buffer) == 0);
}
