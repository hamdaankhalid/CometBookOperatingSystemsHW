#include <crypwalk.h>

int main() {
	int res = encrypt_file("./examples/test_file", "foobars");
	if (res != 0) {
		return res;
	}
	res = decrypt_file("./examples/test_file.crenc", "foobars");
	return res;
}
