#ifndef __SECURE_INPUT_H__
#define __SECURE_INPUT_H__

#include <sys/types.h>

/**
 * Read string from standard input while hiding the input. The read
 * input is returned in 'buf' as a \0 terminated string. The maximum string
 * length will be 'buf_size' - 1.
 *
 * @param buf		target buffer
 * @param buf_size	Size of the target buffer
 */
void secure_input(char *buf, size_t buf_size);

#endif // __SECURE_INPUT_H__
