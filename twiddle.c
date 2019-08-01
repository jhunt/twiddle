#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

/* opcodes for virtual machine */
#define END    0
#define HALT   1
#define IPUSH  2
#define PRINT  3
#define IADD   4
#define ISUB   5
#define CALL   6
#define RET    7
#define JMP    8
#define JE     9
#define JNE   10
#define IMUL  11
#define POP   12
#define LOAD  13
#define STORE 14
#define POPF  15
#define IDIV  16
#define IMOD  17
#define IAND  18
#define IOR   19
#define IXOR  20
#define ILSH  21
#define IRSH  22

struct vm {
	int  ip;
	int  sp;
	int  fp;

	int *code;
	int  stack[4096];
};

#define SPACE  0
#define TOKEN  1
#define NUMBER 2

static struct {
	const char * keyword;
	int          nargs;
	int          opcode;
} LEXICON[] = {
	{ "halt",  0, HALT  },
	{ "ipush", 1, IPUSH },
	{ "print", 0, PRINT },
	{ "iadd",  0, IADD  },
	{ "isub",  0, ISUB  },
	{ "call",  2, CALL  },
	{ "ret",   0, RET   },
	{ "jmp",   1, JMP   },
	{ "je",    1, JE    },
	{ "jne",   1, JNE   },
	{ "imul",  0, IMUL  },
	{ "pop",   0, POP   },
	{ "load",  1, LOAD  },
	{ "store", 1, STORE },
	{ "popf",  0, POPF  },
	{ "idiv",  0, IDIV  },
	{ "imod",  0, IMOD  },
	{ "iand",  0, IAND  },
	{ "ior",   0, IOR   },
	{ "ixor",  0, IXOR  },
	{ "ilsh",  0, ILSH  },
	{ "irsh",  0, IRSH  },
	{ NULL, 0, 0 },
};

void dumpstack(struct vm *vm) {
	int i;

	fprintf(stderr, "[ ");
	for (i = 0; i <= vm->sp; i++) {
		fprintf(stderr, "%d ", vm->stack[i]);
	}
	fprintf(stderr, "]\n");
}

void run(struct vm *vm, int trace) {
	int op;
	int a, b;

	vm->ip = 0;
	vm->sp = -1;
	vm->fp = -1;
	memset(vm->stack, ~0, sizeof(vm->stack) / sizeof(int));
	while ((op = vm->code[vm->ip])) {
		if (trace) {
			fprintf(stderr, "[%04x] %04x", vm->ip, op);
		}

		vm->ip++;
		switch (op) {
		case HALT:
			if (trace) {
				fprintf(stderr, " HALT\n");
			}
			return;

		case IPUSH:
			a = vm->code[vm->ip++];
			if (trace) {
				fprintf(stderr, " IPUSH %d\n", a);
			}
			vm->stack[++vm->sp] = a;
			if (trace) {
				dumpstack(vm);
			}
			break;

		case PRINT:
			if (trace) {
				fprintf(stderr, " PRINT\n");
			}
			printf("%d\n", vm->stack[vm->sp--]);
			if (trace) {
				dumpstack(vm);
			}
			break;

		case IADD:
			if (trace) {
				fprintf(stderr, " IADD\n");
			}
			b = vm->stack[vm->sp--];
			a = vm->stack[vm->sp--];
			vm->stack[++vm->sp] = a + b;
			if (trace) {
				dumpstack(vm);
			}
			break;

		case ISUB:
			if (trace) {
				fprintf(stderr, " ISUB\n");
			}
			b = vm->stack[vm->sp--];
			a = vm->stack[vm->sp--];
			vm->stack[++vm->sp] = a - b;
			if (trace) {
				dumpstack(vm);
			}
			break;

		case CALL:
			if (trace) {
				fprintf(stderr, " CALL [%04x] %d\n", vm->code[vm->ip], vm->code[vm->ip+1]);
			}
			a = vm->code[vm->ip++]; /* fn addr */
			b = vm->code[vm->ip++]; /* # args  */

			vm->stack[++vm->sp] = b;       /* number of parameters  */
			vm->stack[++vm->sp] = vm->fp;  /* previous frame pointer */
			vm->stack[++vm->sp] = vm->ip;  /* return address         */
			vm->fp = vm->sp;
			if (trace) {
				dumpstack(vm);
				fprintf(stderr, "   ret to [%04x]\n", vm->ip);
			}
			vm->ip = a;
			if (trace) {
				fprintf(stderr, "    at ip [%04x]\n", vm->ip);
				fprintf(stderr, "       fp %d was %d\n", vm->fp, vm->stack[vm->fp]);
				fprintf(stderr, "       sp %d\n", vm->sp);
			}
			break;

		case RET:
			if (trace) {
				fprintf(stderr, " RET (fp=%d)\n", vm->fp);
			}
			a = vm->stack[vm->sp];        /* pop return value   */
			vm->sp = vm->fp;              /* reposition stack   */
			vm->ip = vm->stack[vm->sp--]; /* pop return address */
			vm->fp = vm->stack[vm->sp--]; /* pop frame pointer  */
			vm->sp -= vm->stack[vm->sp];  /* pop arguments      */
			vm->stack[++vm->sp] = a;      /* push return value  */

			if (trace) {
				dumpstack(vm);
				fprintf(stderr, "    to ip [%04x]\n", vm->ip);
				fprintf(stderr, "       fp %d\n", vm->fp);
				fprintf(stderr, "       sp %d\n", vm->sp);
			}
			break;

		case JMP:
			if (trace) {
				fprintf(stderr, " JMP [%04x]\n", vm->code[vm->ip]);
			}
			vm->ip = vm->code[vm->ip];
			break;

		case JE:
			if (trace) {
				fprintf(stderr, " JE [%04x]\n", vm->code[vm->ip]);
			}
			a = vm->stack[vm->sp];
			b = vm->stack[vm->sp - 1];
			if (a == b) {
				vm->ip = vm->code[vm->ip];
			} else {
				vm->ip++;
			}
			break;

		case JNE:
			if (trace) {
				fprintf(stderr, " JNE [%04x]\n", vm->code[vm->ip]);
			}
			a = vm->stack[vm->sp];
			b = vm->stack[vm->sp - 1];
			if (a != b) {
				vm->ip = vm->code[vm->ip];
			} else {
				vm->ip++;
			}
			break;

		case IMUL:
			if (trace) {
				fprintf(stderr, " IMUL\n");
			}
			b = vm->stack[vm->sp--];
			a = vm->stack[vm->sp--];
			vm->stack[++vm->sp] = a * b;
			if (trace) {
				dumpstack(vm);
			}
			break;

		case POP:
			if (trace) {
				fprintf(stderr, " POP\n");
			}
			vm->sp--;
			if (trace) {
				dumpstack(vm);
			}
			break;

		case LOAD:
			a = vm->code[vm->ip++];
			if (trace) {
				fprintf(stderr, " LOAD [%04x]%+d\n", vm->fp, a);
			}
			vm->stack[++vm->sp] = vm->stack[vm->fp+a];
			if (trace) {
				dumpstack(vm);
			}
			break;

		case STORE:
			a = vm->code[vm->ip++];
			if (trace) {
				fprintf(stderr, " STORE [%04x]%+d\n", vm->fp, a);
			}
			vm->stack[vm->fp+a] = vm->stack[vm->sp--];
			if (trace) {
				dumpstack(vm);
			}
			break;

		case POPF:
			if (trace) {
				fprintf(stderr, " POPF\n");
			}
			vm->sp = vm->fp;
			break;

		case IDIV:
			if (trace) {
				fprintf(stderr, " IDIV\n");
			}
			b = vm->stack[vm->sp--];
			a = vm->stack[vm->sp--];
			if (b == 0) {
				fprintf(stderr, "#error div/0.\n");
				return;
			}
			vm->stack[++vm->sp] = a / b;
			if (trace) {
				dumpstack(vm);
			}
			break;

		case IMOD:
			if (trace) {
				fprintf(stderr, " IMOD\n");
			}
			b = vm->stack[vm->sp--];
			a = vm->stack[vm->sp--];
			vm->stack[++vm->sp] = a % b;
			if (trace) {
				dumpstack(vm);
			}
			break;

		case IAND:
			if (trace) {
				fprintf(stderr, " IAND\n");
			}
			b = vm->stack[vm->sp--];
			a = vm->stack[vm->sp--];
			vm->stack[++vm->sp] = a & b;
			if (trace) {
				dumpstack(vm);
			}
			break;

		case IOR:
			if (trace) {
				fprintf(stderr, " IOR\n");
			}
			b = vm->stack[vm->sp--];
			a = vm->stack[vm->sp--];
			vm->stack[++vm->sp] = a | b;
			if (trace) {
				dumpstack(vm);
			}
			break;

		case IXOR:
			if (trace) {
				fprintf(stderr, " IXOR\n");
			}
			b = vm->stack[vm->sp--];
			a = vm->stack[vm->sp--];
			vm->stack[++vm->sp] = a ^ b;
			if (trace) {
				dumpstack(vm);
			}
			break;

		case ILSH:
			if (trace) {
				fprintf(stderr, " ILSH\n");
			}
			b = vm->stack[vm->sp--];
			a = vm->stack[vm->sp--];
			vm->stack[++vm->sp] = a << b;
			if (trace) {
				dumpstack(vm);
			}
			break;

		case IRSH:
			if (trace) {
				fprintf(stderr, " IRSH\n");
			}
			b = vm->stack[vm->sp--];
			a = vm->stack[vm->sp--];
			vm->stack[++vm->sp] = a >> b;
			if (trace) {
				dumpstack(vm);
			}
			break;

		default:
			if (trace) {
				fprintf(stderr, " UNKNOWN!!\n");
			}
			return;
		}
	}
}

struct label {
	const char *a, *b;
	int         offset;
};

struct scanner {
	const char   *file;     /* the name of the file being scanned */
	int           fd;       /* file descriptor (ro) to file       */

	int           state;    /* state of the scanner FSM           */
	const char   *src;      /* mmap()'d pointer to source code    */
	const char   *head;     /* pointer into `src` of scan point   */
	int           line;     /* current line number, 1-indexed     */
	int           column;   /* current column, 1-indexed          */

	struct {
		const char *a, *b;
		int         token;
		union {
			int int32;
		} value;
	} lexeme;
};

#define SCANNING_SPACE   0
#define SCANNING_OPCODE  1
#define SCANNING_OPERAND 2
#define SCANNING_LABEL   3

#define T_OPCODE 101
#define T_NUMBER 102
#define T_EOL    103
#define T_LABEL  104
#define T_LREF   105
#define T_EOF    199

int token(struct scanner *sc) {
	const char *a;

	if (!sc->head) {
		sc->lexeme.token = T_EOF;
		return -1;
	}

	for (a = sc->head; *a; ) {
		switch (sc->state) {
		case SCANNING_SPACE:
			if (isalpha(*a)) {
				sc->state = SCANNING_OPCODE;
				sc->lexeme.a = sc->lexeme.b = a;
				sc->lexeme.token = T_OPCODE;

			} else if (isdigit(*a) || *a == '-') {
				sc->state = SCANNING_OPERAND;
				sc->lexeme.a = sc->lexeme.b = a;
				sc->lexeme.token = T_NUMBER;
				sc->lexeme.value.int32 = (*a == '-' ? -1 : *a - '0');

			} else if (*a == '_') {
				sc->state = SCANNING_LABEL;
				sc->lexeme.a = sc->lexeme.b = a;
				sc->lexeme.token = T_LREF; /* a guess */

			} else if (!isspace(*a)) {
				return -1;
			}
			a++;
			break;

		case SCANNING_OPCODE:
			if (isspace(*a)) {
				sc->head = sc->lexeme.b = a;
				sc->state = SCANNING_SPACE;
				return 0;
			}
			if (!isalnum(*a)) {
				return -1;
			}
			a++;
			break;

		case SCANNING_OPERAND:
			if (isspace(*a)) {
				sc->head = sc->lexeme.b = a;
				sc->state = SCANNING_SPACE;
				return 0;
			}
			if (!isdigit(*a)) {
				return -1;
			}
			sc->lexeme.value.int32 *= (*a - '0');
			a++;
			break;

		case SCANNING_LABEL:
			if (isspace(*a)) {
				sc->head = sc->lexeme.b = a;
				sc->state = SCANNING_SPACE;
				return 0;
			}
			if (*a == ':') {
				sc->lexeme.token = T_LABEL;
				sc->head = sc->lexeme.b = a;
				sc->head++;
				sc->state = SCANNING_SPACE;
				return 0;
			}
			if (!isalnum(*a) && *a != '_') {
				return -1;
			}
			a++;
			break;

		default:
			return -1;
		}
	}

	sc->head = sc->lexeme.b = a;
	switch (sc->state) {
	case SCANNING_SPACE:
		sc->lexeme.token = T_EOF;
		return -1;

	case SCANNING_OPCODE:
	case SCANNING_OPERAND:
	case SCANNING_LABEL:
		return 0;
		break;

	default:
		return -1;
	}
}

void reset(struct scanner *sc) {
	sc->state = SCANNING_SPACE;
	sc->head = sc->src;
}

int * scan(const char *path) {
	int i;
	off_t len;
	int ncode = 0, nlabel = 0;
	struct label *labels = NULL;;
	int *code = NULL;
	struct scanner sc;
	memset(&sc, 0, sizeof(struct scanner));

	sc.file = path;
	sc.fd = open(path, O_RDONLY);
	if (sc.fd < 0) { goto failed; }

	len = lseek(sc.fd, 0, SEEK_END);
	if (len < 0) { goto failed; }

	sc.src = mmap(NULL, len, PROT_READ, MAP_PRIVATE, sc.fd, 0);
	if (!sc.src) { goto failed; }

	reset(&sc);
	ncode = nlabel = 0;
	while (token(&sc) == 0) {
		switch (sc.lexeme.token) {
		case T_OPCODE:
		case T_NUMBER:
		case T_LREF:
			ncode++;
			break;

		case T_LABEL:
			nlabel++;
			break;
		}
	}
	if (sc.lexeme.token != T_EOF) {
		fprintf(stderr, "%s: last token was not EOF.\n", sc.file);
		fprintf(stderr, "%s: was (%d)\n", sc.file, sc.lexeme.token);
		goto failed;
	}

	labels = calloc(nlabel, sizeof(struct label));
	if (!labels) { goto failed; }

	code = calloc(ncode + 1, sizeof(int));
	if (!code) { goto failed; }

	reset(&sc);
	ncode = nlabel = 0;
	while (token(&sc) == 0) {
		switch (sc.lexeme.token) {
		case T_OPCODE:
		case T_NUMBER:
		case T_LREF:
			ncode++;
			break;

		case T_LABEL:
			labels[nlabel].a = sc.lexeme.a;
			labels[nlabel].b = sc.lexeme.b;
			labels[nlabel].offset = ncode;
			nlabel++;
			break;
		}
	}

	reset(&sc);
	ncode = 0;
	while (token(&sc) == 0) {
		switch (sc.lexeme.token) {
		case T_OPCODE:
			for (i = 0; LEXICON[i].keyword; i++) {
				if (strncmp(sc.lexeme.a, LEXICON[i].keyword, sc.lexeme.b - sc.lexeme.a) != 0) {
					continue;
				}

				code[ncode++] = LEXICON[i].opcode;
				/* FIXME do something with arity checks */
				break;
			}
			if (!LEXICON[i].keyword) {
				fprintf(stderr, "%s: opcode '%.*s' not found.\n", sc.file, sc.lexeme.a, sc.lexeme.b - sc.lexeme.a);
				goto failed;
			}
			break;

		case T_NUMBER:
			code[ncode++] = sc.lexeme.value.int32;
			break;

		case T_LREF:
			for (i = 0; i < nlabel; i++) {
				if (sc.lexeme.b - sc.lexeme.a != labels[i].b - labels[i].a) {
					/* if the labels are different length, it cannot match */
					continue;
				}
				if (memcmp(sc.lexeme.a, labels[i].a, sc.lexeme.b - sc.lexeme.a) != 0) {
					/* if the labels are different, they do not match */
					continue;
				}

				code[ncode++] = labels[i].offset;
				break;
			}
			if (i == nlabel) {
				fprintf(stderr, "%s: label '%.*s' not found.\n", sc.file, sc.lexeme.a, sc.lexeme.b - sc.lexeme.a);
				goto failed;
			}
			break;
		}
	}

	free(labels);
	code[ncode] = END;
	return code;

failed:
	if (sc.fd >= 0) { close(sc.fd); }
	if (labels) { free(labels); }
	if (code)   { free(code);   }
	return NULL;
}

int main(int argc, char **argv) {
	fprintf(stderr, "Twiddle: a stack VM for language discovery\n");
	if (argc != 2) {
		fprintf(stderr, "USAGE: %s path/to/program.tw\n", argv[0]);
		exit(1);
	}

	struct vm vm;
	vm.code = scan(argv[1]);
	if (!vm.code) {
		fprintf(stderr, "syntax error.\n");
		return 1;
	}
	run(&vm, !!getenv("TRACE"));
	return 0;
}
