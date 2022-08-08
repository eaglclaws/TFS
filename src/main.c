#include "sha256.h"
#include <stdio.h>
int
main(int argc, char **argv)
{
	//const char *foo = "2c26b46b68ffc68ff99b453c1d30413413422d706483bfa0f98a5e886266e7ae";
	//printf("%d\n", strcmp(sha256("foo"), foo));
	char test[65];
	sha256_file(argv[1], test);
	printf("%s\n", test);
	return 0;
}
