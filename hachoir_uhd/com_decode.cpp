#include "com_decode.h"
#include <stdio.h>
#include <string.h>

#define MSG_MAX_LEN 100

struct message_t {
	uint8_t data[MSG_MAX_LEN];
	size_t bit_count;
};

static void message_reset(message_t *msg)
{
	memset(msg->data, 0, MSG_MAX_LEN);
	msg->bit_count = 0;
}

static void message_add_bit(message_t *msg, int b)
{
	size_t byte_idx = msg->bit_count / 8;
	size_t bit_idx = 7 - (msg->bit_count % 8);

	uint8_t avant = msg->data[byte_idx];
	msg->data[byte_idx] &= ~(1 << bit_idx);
	msg->data[byte_idx] |= (b << bit_idx);

	msg->bit_count++;
}

static void message_print(message_t *msg)
{
	fprintf(stderr, "new communication (len = %i):", msg->bit_count);

	for (int i = 0; i < msg->bit_count / 8; i++)
		fprintf(stderr, " %.2x", msg->data[i]);
	fprintf(stderr, "\n");
}

void process_burst_sc16(burst_sc16_t *burst)
{
	static message_t msg;
	static uint64_t last;

	/*fprintf(stderr, "")

	last = burst->stop_time_us;*/

	//fprintf(stderr, "%u ", burst->len);

	/*if (burst->len > 7000) {
		message_print(&msg);
		message_reset(&msg);
	} else if (burst->len > 500 && burst->len < 700)
		message_add_bit(&msg, 1);
	else if (burst->len > 250 && burst->len < 500)
		message_add_bit(&msg, 0);
	else
		fprintf(stderr, "unknown burst len %u\n", burst->len);*/

	char filename[100];
	sprintf(filename, "burst_%i.csv", burst->burst_id);
	FILE *f = fopen(filename, "wb");
	for (int i = 0; i < burst->len; i++) {
		fprintf(f, "%i, %i\n", burst->samples[i].real(), burst->samples[i].imag());
	}
	fclose(f);

	/*char filename[100];
	sprintf(filename, "burst_%i.dat", burst->burst_id);
	FILE *f = fopen(filename, "wb");
	fwrite(burst->samples, sizeof(std::complex<short>), burst->len, f);
	fclose(f);*/


}
