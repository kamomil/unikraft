#include <stdio.h>

/* Import user configuration: */
#include <uk/config.h>

int main(int argc, char *argv[])
{
	printf("Hello world!\n");

#if APPHELLOWORLD_PRINTARGS
	int i;

	printf("Arguments: ");
	for (i=0; i<argc; ++i)
		printf(" \"%s\"", argv[i]);
	printf("\n");
#endif
	int a;
	int b = 1;
	int c = 0;
	a = b/c;
	printf("a = %d\n",a);
	volatile int j = 1;
	while(j == 1){}
}
