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

/* bfcc - brainfuck to C compiler */

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdarg.h>
#include <getopt.h>

/* runtime function names */
#define BFCCLIB_ENTRY_NAME "__bfcc_entry"
#define BFCCLIB_PUT_NAME "__bfcc_put"
#define BFCCLIB_GET_NAME "__bfcc_get"
#define BFCCLIB_INC_NAME "__bfcc_inc"
#define BFCCLIB_DEC_NAME "__bfcc_dec"
#define BFCCLIB_DUMP_NAME "__bfcc_stack_dump"
#define BFCCLIB_STATUS_NAME "__bfcc_status"
#define BFCCLIB_NAME "bfcc"

/* constants */
#define DEFAULT_STACK_SIZE 30000	/* standart stack size for brainfuck */
#define DEFAULT_OUTPUT_NAME "a.out"
#define DEFAULT_CC "cc"
#define WARN_BUF_LEN 1024

static const char* bfcc_entry  = BFCCLIB_ENTRY_NAME;
static const char* bfcc_put    = BFCCLIB_PUT_NAME;
static const char* bfcc_get    = BFCCLIB_GET_NAME;
static const char* bfcc_inc    = BFCCLIB_INC_NAME;
static const char* bfcc_dec    = BFCCLIB_DEC_NAME;
static const char* bfcc_dump   = BFCCLIB_DUMP_NAME;
static const char* bfcc_status = BFCCLIB_STATUS_NAME;
static const char* bfcc_lib    = BFCCLIB_NAME;
static int stack_size          = DEFAULT_STACK_SIZE;
static const char* cc          = DEFAULT_CC;
static const char* cflags      = "";
static bool checks             = false;
static bool stack_dump         = false;
static bool verbose            = false;

static const char* invocation_name;	/* the name through which the application
					   was called from the console */

/* warning function with va_list */
static void vwarn(const char* format, va_list args)
{
	char buf[WARN_BUF_LEN];

	vsnprintf(buf, sizeof(buf), format, args);
	fprintf(stderr, "%s: %s\n", invocation_name, buf);
}

/* warning function */
static void warn(const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vwarn(format, args);
	va_end(args);
}

/* error function (just warning + exit) */
static void err(int status, const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vwarn(format, args);
	va_end(args);
	
	exit(status);
}

/* check backet syntax */
static bool backcheck(const char* str, char open, char close)
{
	const char* s;
	int depth = 0;

	for (s = str; *s != '\0'; s++)
		if (*s == open)		depth++;
		else if (*s == close)	depth--;

	return depth == 0;
}

/* macroses for inserting c code */
#define c_comment(s) fprintf(__CODE_FILE, "/* %s */\n", (s))
#define c_new_line() fputc('\n', __CODE_FILE)

#define c_code(s) fprintf(__CODE_FILE, "\t%s;\n", s)
#define c_end() fputs("}\n", __CODE_FILE)

#define c_bf_entry_begin() fprintf(__CODE_FILE, "void %s(void)\n{\n", bfcc_entry)

#define c_bf_stack() fprintf(__CODE_FILE, "char stack[%d] = {0};\nchar* stack_ptr = stack;\n", stack_size)

#define c_bf_loop() fputs("\twhile (*stack_ptr) {\n", __CODE_FILE);

#define c_bf_put() fprintf(__CODE_FILE, "\t%s(*stack_ptr);\n", bfcc_put)
#define c_bf_get() fprintf(__CODE_FILE, "\t%s(stack_ptr);\n", bfcc_get)

#define c_bf_inc() fprintf(__CODE_FILE, "\t%s(&stack_ptr, stack, %d);\n", bfcc_inc, stack_size)
#define c_bf_dec() fprintf(__CODE_FILE, "\t%s(&stack_ptr, stack, %d);\n", bfcc_dec, stack_size)

#define c_bf_add(x) fprintf(__CODE_FILE, "\t*stack_ptr += %d;\n", x)
#define c_bf_sub(x) fprintf(__CODE_FILE, "\t*stack_ptr -= %d;\n", x)

#define c_bf_stack_dump() fprintf(__CODE_FILE, "\t%s(stack_ptr, stack, %d);\n", bfcc_dump, stack_size)

#define c_bf_status() fprintf(__CODE_FILE, "\t%s(*stack_ptr);\n", bfcc_status)

#define c_bf_forward_decls() 									\
	fprintf(__CODE_FILE, "extern void %s(char);\n", bfcc_put);				\
	fprintf(__CODE_FILE, "extern void %s(char*);\n", bfcc_get);				\
	fprintf(__CODE_FILE, "extern void %s(char**, char*, int);\n", bfcc_inc);		\
	fprintf(__CODE_FILE, "extern void %s(char**, char*, int);\n", bfcc_dec);		\
	fprintf(__CODE_FILE, "extern void %s(char*, char*, int);\n", bfcc_dump);		\
	fprintf(__CODE_FILE, "extern void %s(char);\n", bfcc_status);

/* compile body */
static void compile_body(const char* code, FILE* out)
{
	const char* s;
	int i;

#	define __CODE_FILE out

	for (s = code; *s != '\0'; s++) switch (*s) {
	case '+':
		for (i = 0; *s == '+'; i++, s++);
		s--;
		c_bf_add(i);

		break;

	case '-':
		for (i = 0; *s == '-'; i++, s++);
		s--;
		c_bf_sub(i);

		break;

	case '<':
		if (checks)
			c_bf_dec();
		else
			c_code("stack_ptr--");

		break;

	case '>':
		if (checks)
			c_bf_inc();
		else
			c_code("stack_ptr++");
		
		break;

	case '.':
		c_bf_put();
		break;

	case ',':
		c_bf_get();
		break;

	case '[':
		c_bf_loop();
		break;

	case ']':
		c_end();
		break;

	case '#':
		c_bf_stack_dump();
		break;

	case '?':
		c_bf_status();
		break;

	default:
		break;
	}

#	undef __CODE_FILE
}

/* function for printing output c code to file */
static bool compile(const char* code, FILE* out)
{
	const char* s;
	int i;

	if (!backcheck(code, '[', ']')) {
		warn("compilation error: unclosed backets");
		return false;
	}

#	define __CODE_FILE out

	c_comment("this code generated by bfcc, do not edit!");
	c_new_line();

	c_comment("forward declarations");
	c_bf_forward_decls();
	c_new_line();

	c_comment("brainfuck stack");
	c_bf_stack();
	c_new_line();

	c_comment("brainfuck entry point");
	c_bf_entry_begin();

	compile_body(code, out);

	if (stack_dump)
		c_bf_stack_dump();

	c_end();

#	undef __CODE_FILE

	return true;
}

/* function for read file to buffer */
static char* read_file(FILE* file)
{
	char* src;
	size_t size;

	fseek(file, 0, SEEK_END);
	size = ftell(file);
	fseek(file, 0, SEEK_SET);

	src = malloc(size + 1);
	fread(src, size, size, file);
	src[size] = '\0';
	
	return src;
}

/* function for compiling code to c and writing it to file */
static bool compile_c(FILE* out, const char* in_name)
{
	FILE* in_file;
	char* code;
	bool status;

	if ((in_file = fopen(in_name, "r")) == NULL)
		err(EXIT_FAILURE, "can't open input file");

	code = read_file(in_file);
	fclose(in_file);

	status = compile(code, out);

	free(code);

	return status;
}

/* command lenght */
#define COMMAND_BUF_LEN 1024

/* function for compiling object file */
static void compile_object(const char* out_name, const char* in_name)
{
	char buf[COMMAND_BUF_LEN];
	char tmp_name[L_tmpnam + 1];
	FILE* tmp_file;

	if ((tmp_file = fopen(tmpnam(tmp_name), "w")) == NULL)
		err(EXIT_FAILURE, "can't open temp file");

	if (!compile_c(tmp_file, in_name))
		goto fail;

	fclose(tmp_file);

	snprintf(buf, COMMAND_BUF_LEN, "%s %s -o %s -c -x c %s", cc, cflags, out_name, tmp_name);

	if (verbose)
		fprintf(stderr, "%s\n", buf);

	system(buf);

	return;
fail:
	fclose(tmp_file);
}

/* function for compiling executable */
static void compile_full(const char* out_name, const char* in_name)
{
	char buf[COMMAND_BUF_LEN];
	char tmp_name[L_tmpnam + 1];
	FILE* tmp_file;

	if ((tmp_file = fopen(tmpnam(tmp_name), "w")) == NULL)
		err(EXIT_FAILURE, "can't open temp file");

	if (!compile_c(tmp_file, in_name))
		goto fail;

	fclose(tmp_file);

	snprintf(buf, COMMAND_BUF_LEN, "%s %s -o %s -x c %s -l%s", cc, cflags, out_name, tmp_name, bfcc_lib);

	if (verbose)
		fprintf(stderr, "%s\n", buf);

	system(buf);

	return;
fail:
	fclose(tmp_file);
}

/* usage text */
#define USAGE_SMALL	"usage: bfcc [-C][-c][-h][-o file][-S stack_size][-g cc]"		\
				"[-e fn][-p fn][-G fn][-i fn][-d fn][-D fn][-l library]"	\
				"[-z][-s][-f cflags][-v] file\n"
#define USAGE		"	-C		compile to C source code\n"			\
			"	-c		compile to object file\n"			\
			"	-h		print help\n"					\
			"	-o file		set output file\n"				\
			"	-S stack_size	set stack size\n"				\
			"	-g cc		set C compiler\n"				\
			"	-e fn		set generated code entry point\n"		\
			"	-p fn		set generated code put function\n"		\
			"	-G fn		set generated code get function\n"		\
			"	-i fn		set generated code inc function\n"		\
			"	-d fn		set generated code dec function\n"		\
			"	-D fn		set generated code stack dump function\n"	\
			"	-l library	set generated code bfcc library\n"		\
			"	-z		enable more checks\n"				\
			"	-s		print stack dump at end of program\n"		\
			"	-f		set flags for c compiler\n"			\
			"	-v		print executing commands\n"			\
			"	file		brainfuck file to compile\n"

/* print usage */
static void usage(FILE* stream, bool small)
{
	fputs(small ? USAGE_SMALL : USAGE_SMALL USAGE, stream);
}

/* err + small usage */
static void usage_err(int status, const char* format, ...)
{
	va_list args;

	va_start(args, format);
	vwarn(format, args);
	va_end(args);

	usage(stderr, true);
	
	exit(status);
}

int main(int argc, char* argv[])
{
	const char* output_name = DEFAULT_OUTPUT_NAME;
	const char* file_name;
	FILE* output_file;
	enum {
		MODE_COMPILE_FULL = 0,	/* compile executable */
		MODE_COMPILE_C,		/* compile c source code */
		MODE_COMPILE_OBJECT,	/* compile object file */
	} mode = MODE_COMPILE_FULL;

	invocation_name = argv[0];

	int c;
	opterr = 0;
	while ((c = getopt(argc, argv, "Ccho:S:g:e:p:G:i:d:D:l:zsf:v")) != -1) switch (c) {
	case 'C':				/* compile c source */
		mode = MODE_COMPILE_C;
		break;

	case 'c':				/* compile object file */
		mode = MODE_COMPILE_OBJECT;
		break;

	case 'o':				/* set output name */
		output_name = optarg;
		break;

	case 'S':				/* set stack size */
		stack_size = atoi(optarg);
		break;

	case 'g':				/* set c compiler */
		cc = optarg;
		break;

	case 'e':				/* entry function name */
		bfcc_entry = optarg;
		break;

	case 'p':				/* put function name */
		bfcc_put = optarg;
		break;

	case 'G':				/* get function name */
		bfcc_get = optarg;
		break;

	case 'i':				/* inc function name */
		bfcc_inc = optarg;
		break;

	case 'd':				/* dec function name */
		bfcc_dec = optarg;
		break;

	case 'D':				/* set stack dump function name */
		bfcc_dump = optarg;
		break;

	case 'l':				/* library name */
		bfcc_lib = optarg;
		break;

	case 'z':				/* enable checks */
		checks = true;
		break;

	case 's':				/* enable stack dump */
		stack_dump = true;
		break;

	case 'f':				/* set flags for c compiler */
		cflags = optarg;
		break;

	case 'v':				/* set verbose mode */
		verbose = true;
		break;

	case 'h':				/* print help */
		usage(stdout, false);
		exit(EXIT_SUCCESS);

	case '?':				/* invalid argument */
		usage_err(EXIT_FAILURE, "invalid flag %c", optopt);
	}

	/* check input file exsists */
	if ((file_name = argv[optind]) == NULL)
		usage_err(EXIT_FAILURE, "source file name not found");

	/* open output file */
	if ((output_file = fopen(output_name, "w")) == NULL)
		usage_err(EXIT_FAILURE, "can't open output file");

	/* run compilation */
	switch (mode) {
	case MODE_COMPILE_C:
		compile_c(output_file, file_name);
		break;

	case MODE_COMPILE_OBJECT:
		compile_object(output_name, file_name);
		break;

	case MODE_COMPILE_FULL:
		compile_full(output_name, file_name);
		break;
	}

	/* close output file */
	fclose(output_file);

	return 0;
}
