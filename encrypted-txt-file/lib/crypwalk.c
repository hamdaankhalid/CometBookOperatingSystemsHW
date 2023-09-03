#include "crypwalk.h"

#include <stddef.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

# define ENCRYPTED_FILE_EXTENSION ".enc"
# define BUFFER_SIZE 1024
# define DES_BLOCK_BYTES 8


// DES algorithm takes 64 bits / 8 bytes at a time and suffles it
int data_encryption_standard(unsigned char* block) {
	unsigned char* copy = (unsigned char*)malloc(8 * sizeof(unsigned char)); // 8 bytes allocated for 64 bits
	if (copy == NULL) {
		return -1;
	}
	// manipulate copy data
	
	// initial permutation
	
	// rest of the algorithm

	// end of algorithm
	void* result = memcpy(block, copy, 8 * sizeof (unsigned char));
	free(copy);
	if (result == NULL) {
		return -1;
	}
	return 0;
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
	while (read != 0) {
		if (concatenated_contents == NULL) {
			concatenated_contents = (unsigned char*)malloc(read * sizeof(unsigned char));
			if (concatenated_contents == NULL) {
				fclose(file);
				return ENCRYPTION_ALLOC_ERR;
			}
			curr_size += sizeof (concatenated_contents);
		} else {
			// adjust concatenated_contents for increase in size
			size_t requested_incr = read * sizeof (unsigned char);
			unsigned char* new_adjusted_concatenated = realloc(concatenated_contents, curr_size + requested_incr);
			if (new_adjusted_concatenated == NULL) {
				free(concatenated_contents);
				fclose(file);
				return ENCRYPTION_ALLOC_ERR;
			}

			concatenated_contents = new_adjusted_concatenated;
			curr_size += requested_incr;
		}
		read = fread(&buffer, sizeof (unsigned char), BUFFER_SIZE, file);
	}

	// divide the contents into des input blocks scatter -> gather the computed result	

	return ENCRYPTION_SUCCESS;
}

