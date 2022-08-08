#include <stdio.h>
#include <stdlib.h>
#include <openssl/sha.h>

void
sha256_hash_string(unsigned char hash[SHA256_DIGEST_LENGTH], char outputBuffer[65])
{
	int i = 0;
	for (i = 0; i < SHA256_DIGEST_LENGTH; i++) {
		sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
	}
	outputBuffer[64] = 0;
}

int
sha256_file(char *path, char output[65])
{
	FILE *file = fopen(path, "rb");
	if (!file) return -1;

	unsigned char hash[SHA256_DIGEST_LENGTH];
	SHA256_CTX sha256;
	SHA256_Init(&sha256);
	const int bufSize = 32768;
	char *buffer = malloc(bufSize * sizeof(char));
	int bytesRead = 0;
	if(!buffer) return -1;
	while((bytesRead = fread(buffer, 1, bufSize, file))) {
		SHA256_Update(&sha256, buffer, bytesRead);
	}
	SHA256_Final(hash, &sha256);
	sha256_hash_string(hash, output);
	fclose(file);
	free(buffer);
	return 0;
}
