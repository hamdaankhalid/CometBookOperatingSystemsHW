#include "crypwalk.h"

#include <stddef.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

# define ENCRYPTED_FILE_EXTENSION ".enc"
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

// forward decl
int block_encrypt(unsigned char* block, char key[8]);

void printDebugBinary(unsigned char byte) {
	for (int i = 7; i >= 0; i--) {
        // Use bitwise AND to check the value of each bit
        int bit = (byte >> i) & 1;
        printf("%d", bit);
    }
    printf("\n");
}

ENCRYPT_FILE_RETURN encrypt_file(const char *file_name, const char* encryption_key) {
	if (encryption_key == NULL || strlen(encryption_key) > 7) {
		return ENCRYPTION_INVALID_KEY;
	}

	FILE* file = fopen(file_name, "rb+");
	if (file  == NULL) {
		return ENCRYPTION_FOPEN_ERR;
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
				return ENCRYPTION_ALLOC_ERR;
			}
			concatenated_contents = memcpy(concatenated_contents, &buffer, read);
			if (concatenated_contents == NULL) {
				fclose(file);
				return ENCRYPTION_ALLOC_ERR;
			}
			curr_size += read;
		} else {
			// adjust concatenated_contents for increase in size
			unsigned char* new_adjusted_concatenated = realloc(concatenated_contents, curr_size + read);
			if (new_adjusted_concatenated == NULL) {
				free(concatenated_contents);
				fclose(file);
				return ENCRYPTION_ALLOC_ERR;
			}
			concatenated_contents = new_adjusted_concatenated;
			unsigned char* copied = memcpy(concatenated_contents + curr_size, &buffer, read);
			if (copied == NULL) {
				fclose(file);
				return ENCRYPTION_ALLOC_ERR;
			}
			curr_size += read;
		}
		printf("curr_size %zu, and was read %zu\n", curr_size, read);
		read = fread(&buffer, sizeof (unsigned char), BUFFER_SIZE, file);
	}
	fclose(file);
	if (concatenated_contents == NULL) {
		perror("no content read from file");
		return ENCRYPTION_FILE_ERR;
	}

	// iterate over the concatenated_contents over 8 byte blocks
	int num_rounds = curr_size / 8; // auto floor division
	for (int i = 0; i < num_rounds; i++) {
		unsigned char* block = concatenated_contents + (i*8); // move 8 bytes at a time
		int res = block_encrypt(block, "masterk");
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
	
	FILE* encrypted_file = fopen(encrypted_file_name, "wb");	
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

int block_encrypt(unsigned char* block, char key[8]) {
    // manipulate copy data, and then set the initial block as the manipulated copy data
    unsigned char copy[8] = {0};

    // initial permutation
    for (int i = 0; i < 64; i++) {
        int bit_to_read = INITIAL_PERMUTATION_LOOKUP[i] - 1;
        unsigned char read_byte = block[bit_to_read / DES_BLOCK_BYTES];
        unsigned char write_byte = copy[i / DES_BLOCK_BYTES];

        int value_at_bit = (read_byte >> (7 - (bit_to_read % 8))) & 1;

        if (value_at_bit == 0) {
			// clear
			write_byte = write_byte & ~(1 << (7 - (i % 8)));
		} else {
			// set
			write_byte = write_byte | (1 << (7 - (i % 8)));
		}		
	
        // copy back the mutated byte into copy block
        copy[i / DES_BLOCK_BYTES] = write_byte;
    }
	
	char sub_key[7];
	for (int i = 0; i < 16; i++) {
		char roundL_key[6];
		// do some shit
	}

    // Inverse initial permutation
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

    // End of algorithm
    return 0;
}

