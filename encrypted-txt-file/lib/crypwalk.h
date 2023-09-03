#ifndef CRYPWALK
#define CRYPWALK

typedef enum {
	ENCRYPTION_SUCCESS = 0,
	ENCRYPTION_FOPEN_ERR = -1,
	ENCRYPTION_ALLOC_ERR = -2,
	ENCRYPTION_INVALID_KEY = -3,
	ENCRYPTION_ALGO_ERR = -4,
	ENCRYPTION_WRITE_FILE_ERR = -5,
	ENCRYPTION_FILE_ERR = -6,
} ENCRYPT_FILE_RETURN;

int encrypt_file(const char* file_name, const char* encryption_key);

int decrypt_file(const char* file_name, const char* encryption_key);

#endif
