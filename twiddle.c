#include <stdio.h>

/* opcodes for virtual machine */
#define END  0
#define HALT 1

struct vm {
	int  ip;
	int *code;
};

int main(int argc, char **argv) {
	fprintf(stderr, "Twiddle: a stack VM for language discovery\n");

	int prog[] = {
		HALT,
		END,
	};

	struct vm vm;
	vm.code = prog;
	return 0;
}
