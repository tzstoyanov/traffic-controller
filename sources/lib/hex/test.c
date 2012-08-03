/*
 * hex2.c
 *
 *  Created on: 03.08.2012
 *      Author: ivm
 */

#include <stdio.h>

void hexout(char *outbuff, char buff[], int memory_location, int end);

static char bigbuff[1000];
static char array[1000] = {1,2,3,4,5,6,7,8};

int main()
{
	hexout(bigbuff, array, 0, 180);
	puts("***");
	puts(bigbuff);
	return 0;
}
