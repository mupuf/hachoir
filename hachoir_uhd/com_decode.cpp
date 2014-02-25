#include "com_decode.h"
#include <stdio.h>

void process_burst_sc16(burst_sc16_t *burst)
{
	char filename[100];
	sprintf(filename, "burst_%i.dat", burst->burst_id);
	FILE *f = fopen(filename, "wb");
	fwrite(burst->samples, sizeof(std::complex<short>), burst->len, f);
	fclose(f);
}
