#include "secure_input.h"
#include <stdio.h>
#include <unistd.h>
#ifndef WIN32
# include <termios.h>
# include <sys/ioctl.h>
#endif

void
secure_input(char *buf, size_t buf_size)
{
#ifndef WIN32
	struct termio ttymode;
#endif
	int pos;
	int ch;

#ifndef WIN32
	// hide input
	ioctl(STDIN_FILENO, TCGETA, &ttymode);
	ttymode.c_lflag &= ~( ECHO | ECHOE | ECHONL );
	ioctl(STDIN_FILENO, TCSETAF, &ttymode);
#endif

	pos=0;
	ch=fgetc(stdin);
	while (pos < buf_size-1 && ch != '\n' && ch != EOF) {
		buf[pos] = ch;
		pos++;
		ch=fgetc(stdin);
	} 
	buf[pos] = '\0';

	// flush the stdin buffer of remaining unused characters
	while (ch != '\n' && ch != EOF)
		ch = fgetc(stdin);

#ifndef WIN32
	// unhide input
	ttymode.c_lflag |= ECHO | ECHOE | ECHONL;
	ioctl(STDIN_FILENO, TCSETAF, &ttymode);
	fputc('\n', stdout);
#endif
}
