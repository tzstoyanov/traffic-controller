/*
 * hex2.c
 *
 *  Created on: 03.08.2012
 *      Author: ivm
 */

#include <string.h>
#include <stdio.h>

void hexout(char *outbuff, char buff[], int memory_location, int end);
int parseHexLine(char *theline, char bytes[], int *addr, int *num, int *rectype);

static char dest[1000];
static const char testHexPattern[][44] = {
    ":10010000214601360121470136007EFE09D2190140",
    ":100110002146017EB7C20001FF5F16002148011988",
    ":10012000194E79234623965778239EDA3F01B2CAA7",
    ":100130003F0156702B5E712B722B732146013421C7",
    ":00000001FF"
};

struct testBinPattern_t
{
    int addr;
    int len;
    int type;
    unsigned char data[16];
};

static struct testBinPattern_t testBinPattern[] =
{
    {0x100, 0x10, 0x00, {0x21, 0x46, 0x01, 0x36, 0x01, 0x21, 0x47, 0x01, 0x36, 0x00, 0x7e, 0xfe, 0x09, 0xd2, 0x19, 0x01}},
    {0x00000110, 0x00000010, 0x00000000, {0x21, 0x46, 0x01, 0x7e, 0xb7, 0xc2, 0x00, 0x01, 0xff, 0x5f, 0x16, 0x00, 0x21, 0x48, 0x01, 0x19}},
    {0x00000120, 0x00000010, 0x00000000, {0x19, 0x4e, 0x79, 0x23, 0x46, 0x23, 0x96, 0x57, 0x78, 0x23, 0x9e, 0xda, 0x3f, 0x01, 0xb2, 0xca}},
    {0x00000130, 0x00000010, 0x00000000, {0x3f, 0x01, 0x56, 0x70, 0x2b, 0x5e, 0x71, 0x2b, 0x72, 0x2b, 0x73, 0x21, 0x46, 0x01, 0x34, 0x21}},
    {0x00000000, 0x00000000, 0x00000001, {0}},
};


int testParse(void)
{
    int err = 0;
    int n = sizeof(testHexPattern) / sizeof(*testHexPattern);
    int addr;
    int num;
    int rectype;
    int i;
    for (i = 0; i < n; i++)
    {
		memset(dest,16,16);
        err |= !parseHexLine((char *) (testHexPattern[i]), dest, &addr, &num, &rectype);
		err |= !!strncmp(dest,testBinPattern[i].data, testBinPattern[i].len);
    }


    return err;
}

int testDump(void)
{
    int err = 0;

    int n = sizeof(testHexPattern) / sizeof(*testHexPattern);
    int i;
    for (i = 0; i < n; i++)
    {
		memset(dest,0,sizeof(dest));
	    hexout(dest, testBinPattern[i].data, testBinPattern[i].addr, testBinPattern[i].len);
		err |= !!strcmp(dest,testHexPattern[i]);
    }
    

    return err;
}

int main()
{
    int err = 0;
    puts("***");

    printf("Hex decode: %s \n", (err |= testParse())?"failed":"Passed");
    printf("Hex encode: %s \n", (err |= testDump())?"failed":"Passed");

    return !!err;
}
