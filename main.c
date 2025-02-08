#include <stdio.h>

int main(void)
{
	/* erase the screen with the background color and move the cursor to the top-left corner */
	printf("%c[2J", 0x1b);

	return 0;
}
