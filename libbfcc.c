/*
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted.

 * THE SOFTWARE IS PROVIDED “AS IS” AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE
 * FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT
 * OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

/* libbfcc - support library for executables compiled by bfcc */

#include <stdio.h>
#include <stdlib.h>

/* function names */
#define BFCCLIB_ENTRY __bfcc_entry
#define BFCCLIB_PUT __bfcc_put
#define BFCCLIB_GET __bfcc_get
#define BFCCLIB_INC __bfcc_inc
#define BFCCLIB_DEC __bfcc_dec
#define BFCCLIB_DUMP __bfcc_stack_dump

#define STACK_DUMP_REDUCION_MIN 10

/* brainfuck programm entry */
extern void BFCCLIB_ENTRY(void);

/* put charcter to stdout */
void BFCCLIB_PUT(char c)
{
	putc(c, stdout);
	fflush(stdout);
}

/* get charcter from stdin */
void BFCCLIB_GET(char* c)
{
	*c = getc(stdin);
}

/* print stack dump */
void BFCCLIB_DUMP(char* ptr, char* stack, int stack_size)
{
	char *i, *j;
	int repart;

	fprintf(stderr, "\n==========\nbfcc stack dump:\n");
	fprintf(stderr, "stack_base:\t%p\n", stack);
	fprintf(stderr, "stack_ptr:\t%p (in stack %x)\n", ptr, (unsigned int) (ptr - stack));
	fprintf(stderr, "stack_end:\t%p\n", stack + stack_size);

	for (i = stack; i < stack + stack_size; i++) {
		for (j = i, repart = 0; j < stack + stack_size && *j == *i; j++, repart++);

		if (repart >= STACK_DUMP_REDUCION_MIN) {
			fprintf(stderr, "%x\t%x\n", (unsigned int) (i - stack), *i);
			fprintf(stderr, "...times %d...\n", repart);

			i = j - 1;
			continue;
		}

		fprintf(stderr, "%x\t%x\n", (unsigned int) (i - stack), *i);
	}

	fprintf(stderr, "==========\n");
}

/* print runtime_error */
static void runtime_error(const char* format)
{
	fprintf(stderr, "\n==========\nbfcc runtime error: %s\n==========\n", format);
	abort();
}

/* check stack pointer to valid */
static void check_stack_ptr(char* ptr, char* stack, int stack_size)
{
	if (ptr < stack || ptr >= (stack + stack_size)) {
		BFCCLIB_DUMP(ptr, stack, stack_size);
		runtime_error("stack pointer owerflowed");
	}
}

/* increment stack pointer */
void BFCCLIB_INC(char** ptr, char* stack, int stack_size)
{
	(*ptr)++;

	check_stack_ptr(*ptr, stack, stack_size);
}

/* decrement stack pointer */
void BFCCLIB_DEC(char** ptr, char* stack, int stack_size)
{
	(*ptr)--;

	check_stack_ptr(*ptr, stack, stack_size);
}

int main(int argc, char* argv[])
{
	/* call brainfuck programm */
	BFCCLIB_ENTRY();

	return 0;
}
