
#include <stdio.h>

int main(int argc, char **argv)
{
    FILE *test = fopen("test.file", "wx");

    if (test == NULL)
    {
	fprintf(stderr, "Couldn't open 'test.file' for exclusive write\n");
	return 1;
    }

    fclose(test);

    return 0;
}
