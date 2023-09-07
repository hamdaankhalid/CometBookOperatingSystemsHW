#include <crypwalk.h>

int main() {
	int res = encrypt_file("./examples/test_file", "foobars");
	if (res != 0) {
		return res;
	}
	
	printf("ENCRYPTION COMPLETED SUCCESSFULLY\n");

	res = decrypt_file("./examples/test_file.crenc", "foobars");
	if (res != 0) {
		return res;
	}


	printf("DECRYPTION COMPLETED SUCCESSFULLY\n");
}
