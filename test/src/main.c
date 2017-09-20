#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include <osal.h>
#include <httpd.h>

void start_tests(void);

int main()
{
	printf("Application Entry\n");
	start_tests();
	pthread_exit(NULL);
	return 0;
}
