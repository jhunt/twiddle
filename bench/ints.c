void bench(int r) {
	int n = r;
	int junk;
	while (n > 0) {
		junk = 7;
		junk /= 2;
		junk *= 15;
		junk += r;
		n--;
	}
}

int main() {
	bench(10000000);
	return 0;
}
