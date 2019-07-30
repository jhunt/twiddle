#include <stdio.h>

/* flags for run() */
#define TRACE   1
#define NOTRACE 0

/* opcodes for virtual machine */
#define END  0
#define HALT 1

struct vm {
	int  ip;
	int *code;
};

void run(struct vm *vm, int trace) {
	int op;

	vm->ip = 0;
	while (op = vm->code[vm->ip]) {
		if (trace) {
			fprintf(stderr, "[%04x] %04x\n", vm->ip, op);
		}

		vm->ip++;
		switch (op) {
		case HALT:
			return;
		}
	}
}

int main(int argc, char **argv) {
	fprintf(stderr, "Twiddle: a stack VM for language discovery\n");

	int prog[] = {
		HALT,
		END,
	};

	struct vm vm;
	vm.code = prog;
	run(&vm, TRACE);
	return 0;
}
