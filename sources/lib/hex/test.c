/*
 * hex2.c
 *
 *  Created on: 03.08.2012
 *      Author: ivm
 */

#include <string.h>
#include <stdio.h>

void hexout(char *outbuff, char buff[], int memory_location, int end);

static char bigbuff[1000];
static char dest[1000];
static char array[1000] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17};

int main()
{
	puts("***");
	memset(bigbuff,0,16*2+4+2+2+1+1);
	hexout(bigbuff, array, 0, 16);
	parseHexLine(array, dest,)
	puts(bigbuff);
	
	memset(bigbuff,0,16*2+4+2+2+1+1);
	hexout(bigbuff, array, 16, 16);
	puts(bigbuff);

	memset(bigbuff,0,16*2+4+2+2+1+1);
	hexout(bigbuff, array, 0, 0);
	puts(bigbuff);
	return 0;
}
