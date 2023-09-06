#include "crypwalk.h"

#include <stddef.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

# define ENCRYPTED_FILE_EXTENSION ".crenc"
# define DECRYPTED_FILE_EXTENSION ".drenc"
# define BUFFER_SIZE 1024
# define DES_BLOCK_BYTES 8

const int INITIAL_PERMUTATION_LOOKUP[64] = {40, 8, 48, 16, 56, 24, 64, 32,
											39, 7, 47, 15, 55, 23, 63, 31,
											38, 6, 46, 14, 54, 22, 62, 30,
											37, 5, 45, 13, 53, 21, 61, 29,
											36, 4, 44, 12, 52, 20, 60, 28,
											35, 3, 43, 11, 51, 19, 59, 27,
											34, 2, 42, 10, 50, 18, 58, 26,
											33, 1, 41, 9,  49, 17, 57, 25};

const int INVERSE_INITIAL_PERMUTATION_LOOKUP[64] = {58, 50, 42, 34, 26, 18, 10, 2,
													60, 52, 44, 36, 28, 20, 12, 4,
													62, 54, 46, 38, 30, 22, 14, 6,
													64, 56, 48, 40, 32, 24, 16, 8,
													57, 49, 41, 33, 25, 17, 9, 1,
													59, 51, 43, 35, 27, 19, 11, 3,
													61, 53, 45, 37, 29, 21, 13, 5,
													63, 55, 47, 39, 31, 23, 15, 7};

typedef struct {
	unsigned char* buffer;
	size_t buffer_size;
} DynamicBufferResult;

// forward decl
int block_encrypt(unsigned char* block);
int block_decrypt(unsigned char* block);
DynamicBufferResult read_dynamic_buffer(const char* file_name);
unsigned long generateHash(const char* pswd);
int verifyHash(const char* pswd, unsigned long hash);

void printDebugBinary(unsigned char byte) {
	for (int i = 7; i >= 0; i--) {
        // Use bitwise AND to check the value of each bit
        int bit = (byte >> i) & 1;
        printf("%d", bit);
    }
    printf("\n");
}

typedef struct __attribute__((packed)) {
	unsigned int hash_size;
	unsigned int data_offset;
	unsigned long hash;
} CrypwalkHeader;

ENCRYPT_FILE_RETURN encrypt_file(const char *file_name, const char* encryption_key) {
	if (encryption_key == NULL || strlen(encryption_key) > 7) {
		return ENCRYPTION_INVALID_KEY;
	}

	FILE* file = fopen(file_name, "rb");
	if (file  == NULL) {
		return ENCRYPTION_FOPEN_ERR;
	}
	
	DynamicBufferResult buf = read_dynamic_buffer(file_name);
	unsigned char* concatenated_contents = buf.buffer;
	size_t curr_size = buf.buffer_size;

	if (concatenated_contents == NULL) {
		return ENCRYPTION_FILE_ERR;
	}
	
	// Later we can use concurrency here
	// iterate over the concatenated_contents over 8 byte blocks
	int num_rounds = curr_size / 8; // auto floor division
	for (int i = 0; i < num_rounds; i++) {
		unsigned char* block = concatenated_contents + (i*8); // move 8 bytes at a time
		int res = block_encrypt(block);
		if (res < 0) {
			free(concatenated_contents);
			return ENCRYPTION_ALGO_ERR;
		}
	}
	
	// write to a new file with mutated block and save with extension
	size_t file_name_len = strlen(file_name);
	char* encrypted_file_name = (char *)malloc(file_name_len + strlen(ENCRYPTED_FILE_EXTENSION) + 1);
	if (encrypted_file_name == NULL) {
		free(concatenated_contents);
		return ENCRYPTION_ALLOC_ERR;
	}
	
	strcpy(encrypted_file_name, file_name);
	strcpy(encrypted_file_name + file_name_len, ENCRYPTED_FILE_EXTENSION);
	
	// create encrypted file header
	CrypwalkHeader header;
	header.data_offset = sizeof(CrypwalkHeader);
	header.hash  = generateHash(encryption_key);
	header.hash_size = 13;

	FILE* encrypted_file = fopen(encrypted_file_name, "wb");

	// write header into the file
	if (fwrite(&header, sizeof(CrypwalkHeader), 1, encrypted_file) != 1) {
		fclose(encrypted_file);
		free(concatenated_contents);
		free(encrypted_file_name);
		return ENCRYPTION_WRITE_FILE_ERR;
	}

	// write encrypted contents into file
	if(fwrite(concatenated_contents, sizeof (unsigned char), curr_size, encrypted_file) != curr_size) {
		fclose(encrypted_file);
		free(concatenated_contents);
		free(encrypted_file_name);
		return ENCRYPTION_WRITE_FILE_ERR;
	}

	fclose(encrypted_file);
	free(concatenated_contents);
	free(encrypted_file_name);
	return ENCRYPTION_SUCCESS;
}

DECRYPT_FILE_RETURN decrypt_file(const char *file_name, const char *encryption_key) {
	if (encryption_key == NULL || strlen(encryption_key) > 7) {
		return DECRYPTION_INVALID_KEY;
	}

	FILE* file = fopen(file_name, "rb");
	if (file  == NULL) {
		return DECRYPTION_FOPEN_ERR;
	}
	
	// from the file read the header and let it move the offset as required
	CrypwalkHeader header;
	int result = fread(&header, sizeof(CrypwalkHeader), 1, file);
	if (result != 1) {
		fclose(file);
		return DECRYPTION_FILE_ERR;
	}
	fclose(file);
	
	// false is 0
	if (verifyHash(encryption_key, header.hash) == 0) {
		return DECRYPTION_INCORRECT_KEY;
	}

	DynamicBufferResult buf = read_dynamic_buffer(file_name);
	unsigned char* concatenated_contents_start = buf.buffer;
	size_t curr_size = buf.buffer_size;

	if (concatenated_contents_start == NULL) {
		return DECRYPTION_FILE_ERR;
	}

	// skip past the header and hash
	int offset = sizeof(CrypwalkHeader);
	unsigned char* concatenated_contents = concatenated_contents_start + offset;
	curr_size -= offset;

	int num_rounds = curr_size / 8; // auto floor division
	for (int i = 0; i < num_rounds; i++) {
		unsigned char* block = concatenated_contents + (i*8); // move 8 bytes at a time
		int res = block_decrypt(block);
		if (res < 0) {
			free(concatenated_contents_start);
			return DECRYPTION_ALGO_ERR;
		}
	}
	
	int decrypt_file_name_sz = strlen(file_name) - strlen(ENCRYPTED_FILE_EXTENSION);
	char* decrypt_file_name = (char*) malloc(decrypt_file_name_sz + 1);
	strncpy(decrypt_file_name, file_name, decrypt_file_name_sz);
	decrypt_file_name[decrypt_file_name_sz] = '\0';
	
	FILE* new_file = fopen(decrypt_file_name, "wb");
	if (new_file == NULL) {
		free(decrypt_file_name);
		free(concatenated_contents_start);
		return DECRYPTION_ALLOC_ERROR;
	}

	// write to a new file
	if (fwrite(concatenated_contents, sizeof(unsigned char), curr_size, new_file) != curr_size) {
		free(decrypt_file_name);
		free(concatenated_contents_start);
		return DECRYPTION_FILE_ERR;
	}
	
	return 0;
}

DynamicBufferResult read_dynamic_buffer(const char* file_name) {
	DynamicBufferResult resp = {NULL, 0};

	FILE* file = fopen(file_name, "rb");
	if (file  == NULL) {
		return resp;
	}

	unsigned char* concatenated_contents = NULL;
	size_t curr_size = 0;

	char buffer[BUFFER_SIZE];	
	size_t read = fread(&buffer, sizeof (char), BUFFER_SIZE, file);
	while (read > 0) {
		if (concatenated_contents == NULL) {
			concatenated_contents = (unsigned char*)malloc(read * sizeof(unsigned char));
			if (concatenated_contents == NULL) {
				fclose(file);
				return resp;
			}
			concatenated_contents = memcpy(concatenated_contents, &buffer, read);
			if (concatenated_contents == NULL) {
				fclose(file);
				return resp;
			}
			curr_size += read;
		} else {
			// adjust concatenated_contents for increase in size
			unsigned char* new_adjusted_concatenated = realloc(concatenated_contents, curr_size + read);
			if (new_adjusted_concatenated == NULL) {
				free(concatenated_contents);
				fclose(file);
				return resp;
			}
			concatenated_contents = new_adjusted_concatenated;
			unsigned char* copied = memcpy(concatenated_contents + curr_size, &buffer, read);
			if (copied == NULL) {
				fclose(file);
				return resp;
			}
			curr_size += read;
		}
		read = fread(&buffer, sizeof (unsigned char), BUFFER_SIZE, file);
	}
	fclose(file);
	resp.buffer = concatenated_contents;
	resp.buffer_size = curr_size;
	return resp;
}

int block_encrypt(unsigned char* block) {
    // manipulate copy data, and then set the initial block as the manipulated copy data
    unsigned char copy[8] = {0};
	
	memcpy(copy, block, 8);

    // initial permutation
    for (int i = 0; i < 64; i++) {
        int bit_to_read = INITIAL_PERMUTATION_LOOKUP[i] - 1;
        unsigned char read_byte = copy[bit_to_read / DES_BLOCK_BYTES];
        unsigned char write_byte = block[i / DES_BLOCK_BYTES];

        int value_at_bit = (read_byte >> (7 - (bit_to_read % 8))) & 1;

        if (value_at_bit == 0) {
			// clear
			write_byte = write_byte & ~(1 << (7 - (i % 8)));
		} else {
			// set
			write_byte = write_byte | (1 << (7 - (i % 8)));
		}		
	
        block[i / DES_BLOCK_BYTES] = write_byte;
    }
	
    return 0;
}

int block_decrypt(unsigned char* block) {
	unsigned char copy[8] = {0};
	
	memcpy(copy, block, 8);

    for (int i = 0; i < 64; i++) {
        int bit_to_read = INVERSE_INITIAL_PERMUTATION_LOOKUP[i] - 1;
        unsigned char read_byte = copy[bit_to_read / DES_BLOCK_BYTES];
        unsigned char write_byte = block[i / DES_BLOCK_BYTES];

        int value_at_bit = (read_byte >> (7 - (bit_to_read % 8))) & 1;

        if (value_at_bit == 0) {
			// clear
			write_byte = write_byte & ~(1 << (7 - (i % 8)));
		} else {
			// set
			write_byte = write_byte | (1 << (7 - (i % 8)));
		}
		
		// back from copy to original block
        block[i / DES_BLOCK_BYTES] = write_byte;
    }

	return 0;
}

unsigned long generateHash(const char* pswd) {
	unsigned long hash_value = 5381;
	int c;
	while((c = *pswd++)) {
		hash_value = ((hash_value << 5) + hash_value) + c;
	}
	return hash_value;
}

int verifyHash(const char* pswd, unsigned long hash) {
	return generateHash(pswd) == hash;
}
