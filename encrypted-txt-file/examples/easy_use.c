#include <crypwalk.h>

int main() {
	int res = encrypt_file("./examples/test_file", "foobars");
	return res;
}