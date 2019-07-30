#include <stdio.h>

/* flags for run() */
#define TRACE   1
#define NOTRACE 0

/* opcodes for virtual machine */
#define END   0
#define HALT  1
#define IPUSH 2
#define PRINT 3
#define IADD  4
#define ISUB  5
#define CALL  6
#define RET   7

struct vm {
	int  ip;
	int  sp;
	int  fp;

	int *code;
	int  stack[4096];
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
	memset(vm->stack, ~0, sizeof(vm->stack) / sizeof(int));
	while (op = vm->code[vm->ip]) {
		if (trace) {
			fprintf(stderr, "[%04x] %04x", vm->ip, op);
		}

		vm->ip++;
		switch (op) {
		case HALT:
			fprintf(stderr, " HALT\n");
			return;

		case IPUSH:
			a = vm->code[vm->ip++];
			fprintf(stderr, " IPUSH %d\n", a);
			vm->stack[++vm->sp] = a;
			break;

		case PRINT:
			fprintf(stderr, " PRINT\n");
			printf("%d\n", vm->stack[vm->sp--]);
			break;

		case IADD:
			fprintf(stderr, " IADD\n");
			b = vm->stack[vm->sp--];
			a = vm->stack[vm->sp--];
			vm->stack[++vm->sp] = a + b;
			break;

		case ISUB:
			fprintf(stderr, " ISUB\n");
			b = vm->stack[vm->sp--];
			a = vm->stack[vm->sp--];
			vm->stack[++vm->sp] = a - b;
			break;

		case CALL:
			/* FRAME layout

			  +----------------+
			  | argn           |
			  +----------------+
			  | arg2           |
			  +----------------+
			  | arg1           |
			  +----------------+
			  | nargs          |
			  +----------------+
			  | return address |
			  +----------------+
			  | saved fp       | <--- fp
			  +----------------+
			  | local1         |
			  +----------------+
			  | local2         |
			  +----------------+
			  | local3         | <--- sp
			  +----------------+

			 */
			fprintf(stderr, " CALL [%04x] %d\n", vm->code[vm->ip], vm->code[vm->ip+1]);
			dumpstack(vm);
			a = vm->code[vm->ip++]; /* fn addr */
			b = vm->code[vm->ip++]; /* # args  */

			vm->fp = vm->sp;
			vm->stack[++vm->sp] = b;       /* number of parameeters  */
			vm->stack[++vm->sp] = vm->ip;  /* return address         */
			vm->stack[++vm->sp] = vm->fp;    /* previous frame pointer */
			vm->fp = vm->sp;
			dumpstack(vm);
			fprintf(stderr, "   ret to [%04x]\n", vm->ip);
			vm->ip = a;
			fprintf(stderr, "    at ip [%04x]\n", vm->ip);
			fprintf(stderr, "       fp [%04x]\n", vm->fp);
			fprintf(stderr, "       sp [%04x]\n", vm->sp);
			break;

		case RET:
			fprintf(stderr, " RET\n");
			a = vm->stack[vm->sp];
			vm->ip = vm->stack[vm->fp-1];
			vm->fp = vm->stack[vm->fp];
			vm->sp = vm->fp;
			vm->stack[++vm->sp] = a;
			fprintf(stderr, "    to ip [%04x]\n", vm->ip);
			fprintf(stderr, "       fp [%04x]\n", vm->fp);
			fprintf(stderr, "       sp [%04x]\n", vm->sp);
			break;

		default:
			fprintf(stderr, " UNKNOWN!!\n");
			return;
		}
	}
}

int main(int argc, char **argv) {
	fprintf(stderr, "Twiddle: a stack VM for language discovery\n");

	int prog[] = {
		/*
		   f() { return 10; }
		   40 + 2 + f() - 5;
		 */
		IPUSH, 40,   /*  0 */
		IPUSH, 2,    /*  2 */
		IADD,        /*  4 */
		CALL, 14, 0, /*  5 */
		IADD,        /*  8 */
		IPUSH, 5,    /*  9 */
		ISUB,        /* 11 */
		PRINT,       /* 12 */
		HALT,        /* 13 */

		/* (λ () 10) */
		IPUSH, 10,   /* 14 */
		RET,         /* 16 */
		END          /* 17 */
	};

	struct vm vm;
	vm.code = prog;
	run(&vm, TRACE);
	return 0;
}
