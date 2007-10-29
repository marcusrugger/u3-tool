#include "display_progress.h"
#include <stdio.h>

void display_progress(unsigned int cur, unsigned int total) {
        unsigned int procent;
	unsigned int bar_len;
	int i;

	if (total == 0) return;

	procent = (cur * 100) / total;

	putchar('|');
	bar_len = (cur * PROGRESS_BAR_WIDTH) / total;
	for (i =0; i < bar_len; i++) {
		putchar('*');
	}
	for (i = bar_len; i < PROGRESS_BAR_WIDTH; i++) {
		putchar(' ');
	}
	putchar('|');

	printf(" %d%%\r", procent);
	fflush(stdout);
}
