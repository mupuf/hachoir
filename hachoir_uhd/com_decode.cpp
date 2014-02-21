#include "com_decode.h"
#include <stdio.h>

void process_burst_sc16(burst_sc16_t *burst)
{
	static int i = 0;
	char filename[100];
	sprintf(filename, "burst_%i.dat", i++);
	FILE *f = fopen(filename, "wb");
	fwrite(burst->samples, sizeof(std::complex<short>), burst->len, f);
	fclose(f);
}
