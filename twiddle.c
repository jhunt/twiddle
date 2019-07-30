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

struct vm {
	int  ip;
	int  sp;
	int *code;
	int  stack[4096];
};

void run(struct vm *vm, int trace) {
	int op;
	int a, b;

	vm->ip = 0;
	vm->sp = -1;
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
		}
	}
}

int main(int argc, char **argv) {
	fprintf(stderr, "Twiddle: a stack VM for language discovery\n");

	int prog[] = {
		IPUSH, 42,
		IPUSH, 10,
		IADD,
		IPUSH, 5,
		ISUB,
		PRINT,
		HALT,
		END,
	};

	struct vm vm;
	vm.code = prog;
	run(&vm, TRACE);
	return 0;
}
