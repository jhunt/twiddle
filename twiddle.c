#include <stdio.h>
#include <string.h>

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
#define JMP   8
#define JE    9
#define JNE  10
#define IMUL 11

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

			vm->fp = vm->sp;
			vm->stack[++vm->sp] = b;       /* number of parameeters  */
			vm->stack[++vm->sp] = vm->ip;  /* return address         */
			vm->stack[++vm->sp] = vm->fp;    /* previous frame pointer */
			vm->fp = vm->sp;
			if (trace) {
				dumpstack(vm);
				fprintf(stderr, "   ret to [%04x]\n", vm->ip);
			}
			vm->ip = a;
			if (trace) {
				fprintf(stderr, "    at ip [%04x]\n", vm->ip);
				fprintf(stderr, "       fp [%04x]\n", vm->fp);
				fprintf(stderr, "       sp [%04x]\n", vm->sp);
			}
			break;

		case RET:
			if (trace) {
				fprintf(stderr, " RET\n");
			}
			a = vm->stack[vm->sp];
			vm->ip = vm->stack[vm->fp-1];
			vm->fp = vm->stack[vm->fp];
			vm->sp = vm->fp;
			vm->stack[++vm->sp] = a;
			if (trace) {
				dumpstack(vm);
				fprintf(stderr, "    to ip [%04x]\n", vm->ip);
				fprintf(stderr, "       fp [%04x]\n", vm->fp);
				fprintf(stderr, "       sp [%04x]\n", vm->sp);
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
			a = vm->stack[vm->sp--];
			b = vm->stack[vm->sp--];
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
			a = vm->stack[vm->sp--];
			b = vm->stack[vm->sp--];
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

		default:
			if (trace) {
				fprintf(stderr, " UNKNOWN!!\n");
			}
			return;
		}
	}
}

int main(int argc, char **argv) {
	fprintf(stderr, "Twiddle: a stack VM for language discovery\n");

	int prog[] = {
		/*
		   print 6 * 9
		 */
		IPUSH, 6,
		IPUSH, 9,
		IMUL,
		PRINT,
		HALT,
	};

	struct vm vm;
	vm.code = prog;
	run(&vm, TRACE);
	return 0;
}
