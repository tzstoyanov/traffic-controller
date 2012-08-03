/*
 * ihex.c
 *
 *  Created on: 03.08.2012
 *      Author: ivm
 */

static const char hexSymb[] = "0123456789ABCDEF";

static char ctohex(unsigned char x) {
	return hexSymb[x & 15];
}

/* some ansi prototypes.. maybe ought to make a .h file */

char *itohex4(char *str, unsigned short x) {
	*str++ = ctohex((unsigned char) (x >> 12));
	*str++ = ctohex((unsigned char) (x >> 8));
	*str++ = ctohex((unsigned char) (x >> 4));
	*str++ = ctohex((unsigned char) (x));
	return str;
}

char *itohex2(char *str, unsigned char x) {
	*str++ = ctohex((unsigned char) (x >> 4));
	*str++ = ctohex((unsigned char) (x));
	return str;
}

int hextoi(char *str) {
	int y = 0;
	int t;
	for (; *str; ++str) {
		t = *str;
		if (t < 'A')
			t -= '0';
		else if (t < 'a')
			t -= 'A';
		else
			t -= 'a';
		y <<= 4;
		y |= t;
	}
	return y;
}

/* parses a line of intel hex code, stores the data in bytes[] */
/* and the beginning address in addr, and returns a 1 if the */
/* line was valid, or a 0 if an error occured.  The variable */
/* num gets the number of bytes that were stored into bytes[] */

int parseHexLine(char *theline, int bytes[], int *addr, int *num, int *code) {
	int sum, len, cksum;
	char *ptr;

	*num = 0;
	if (theline[0] != ':')
		return 0;
	if (strlen(theline) < 11)
		return 0;
	ptr = theline + 1;
	len = hextoi(ptr);
	ptr += 2;
	if ((int) strlen(theline) < (11 + (len * 2)))
		return 0;
	itohex4(ptr, *addr);
	ptr += 4;
	/* printf("Line: length=%d Addr=%d\n", len, *addr); */
	*code = hextoi(ptr);
	ptr += 2;
	sum = (len & 255) + ((*addr >> 8) & 255) + (*addr & 255) + (*code & 255);
	while (*num != len) {
		bytes[*num] = hextoi(ptr);
		ptr += 2;
		sum += bytes[*num] & 255;
		(*num)++;
		if (*num >= 256)
			return 0;
	}
	cksum = hextoi(ptr);

	if (((sum & 255) + (cksum & 255)) & 255)
		return 0; /* checksum error */
	return 1;
}

/* produce intel hex string output... call this routine with */
/* each byte to output and it's memory location.  The buff */
/* pointer outbuff must have enought size.  After */
/* all data is written, call with end=1 (normally set to 0) */
/* so it will flush the data from its static buffer */

#define MAXHEXLINE 32	/* the maximum number of bytes to put in one line */

void hexout(char *outbuff, char byte_buffer[16] , int memory_location, int len) {
	register int i=0, sum=0, type = 0 >= len;

	if (len > 16 ) len = 16;

	/* it's time to dump the buffer to a line in the file */
	outbuff[0] = ':';
	itohex2(&outbuff[1], len);
	itohex4(&outbuff[1 + 2], memory_location);
	itohex2(&outbuff[1 + 2 + 4], type); /* type */
	sum = type + len + ((memory_location >> 8) & 255) + (memory_location & 255);
	for (i = 0; i < len; i++) {
		itohex2(&outbuff[1 + 2 + 4 + 2 + 2 * i], byte_buffer[i] & 255);
		sum += byte_buffer[i] & 255;
	}
	itohex2(&outbuff[1 + 2 + 4 + 2 + 2 * i], (-sum) & 255);
}

