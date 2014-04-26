#include "rxtimedomain.h"

#include <sys/time.h>
#include <string.h>
#include <iostream>

#include "demodulations/ook.h"
#include "demodulations/fsk.h"
#include "utils/manchester.h"
#include "utils/sig_proc.h"

#define BURST_MIN_ALLOC_SIZE 100000

#define DC_OFFSET_SAMPLE_COUNT 32768
#define NOISE_AVR_SAMPLE_COUNT 8192
#define NOISE_THRESHOLD_FACTOR 3.0

#define COMS_DETECT_MIN_SAMPLES 100
#define COMS_DETECT_SAMPLES_UNDER_THRS 20
#define COMS_DETECT_COALESCING_TIME_US 10000

RXTimeDomain::RXTimeDomain(RXTimeDomainMessageCallback cb, void *userData) :
	_userCb(cb), _userData(userData)
{
}

phy_parameters_t RXTimeDomain::phyParameters()
{
	return _phy;
}

void RXTimeDomain::setPhyParameters(const phy_parameters_t &phy)
{
	_phy = phy;
}

void RXTimeDomain::reset()
{

}

static inline
uint64_t sample_count_from_time(float sample_rate, uint64_t time_us)
{
	return time_us * sample_rate / 1000000;
}

bool RXTimeDomain::processSamples(uint64_t time_us, std::complex<short> *samples,
		     size_t count)
{
	/* detection + current state */
	static float noise_mag_max = -1.0, com_thrs = 100000.0, noise_cur_max = 0.0;
	static float I_avr = 0.0, Q_avr = 0.0, I_sum = 0.0, Q_sum = 0.0;
	static size_t IQ_count = 0, noise_mag_count = 0, detect_samples_under = 0;
	static rx_state_t state = LISTEN;

	/* communication */
	static size_t com_sample = 0, com_sub_sample = 0;
	static float com_mag_sum = 0.0;
	size_t com_blk_start = 0;
	size_t com_coalescing_samples_count = sample_count_from_time(_phy.sample_rate,
								     COMS_DETECT_COALESCING_TIME_US);
	/* for every sample */
	for (size_t i = 0; i < count; i++) {
		float I = samples[i].real() - I_avr;
		float Q = samples[i].imag() - Q_avr;
		float mag = sqrtf(I*I + Q*Q);

		/* calculate the I/Q DC-offsets */
		I_sum += samples[i].real();
		Q_sum += samples[i].imag();
		if (IQ_count++ % DC_OFFSET_SAMPLE_COUNT == DC_OFFSET_SAMPLE_COUNT - 1) {
			I_avr = I_sum / IQ_count;
			Q_avr = Q_sum / IQ_count;
			IQ_count = 0;
			I_sum = Q_sum = 0.0;
			//fprintf(stderr, "I_avr = %f, Q_avr = %f\n", I_avr, Q_avr);
		}

		//process_sc16_dump_samples(file, samples[i], mag, noise_avr, state == RX);

		/* calculate the noise level and thresholds */
		if (mag > noise_cur_max)
			noise_cur_max = mag;
		if (noise_mag_count++ % NOISE_AVR_SAMPLE_COUNT == NOISE_AVR_SAMPLE_COUNT -1) {
			if (noise_mag_max < 0 || noise_cur_max < noise_mag_max) {
				noise_mag_max = noise_cur_max;
				com_thrs = noise_mag_max * NOISE_THRESHOLD_FACTOR;
			}
			//fprintf(stderr, "noise_mag_max = %f, noise_cur_max = %f\n", noise_mag_max, noise_cur_max);
			noise_cur_max = 0.0;
			noise_mag_count = 0;
		}

		/* detect the beginning of new transmissions */
		if (mag >= com_thrs) {
			if (state == LISTEN) {
				state = RX;
				burst.start(_phy, noise_mag_max, time_us, i);
				com_mag_sum = 0.0;
				com_sample = 0;
				com_sub_sample = 0;
				com_blk_start = i;
			} else if(state == COALESCING) {
				size_t com_blk_len = i - com_blk_start;
				burst.append(samples + com_blk_start, com_blk_len);
				burst.subStart();
				com_sub_sample = 0;

				state = RX;
				com_blk_start = i;
			}
			detect_samples_under = 0;
		} else
			detect_samples_under++;

		/* code executed when we are receiving a transmission */
		if (state > LISTEN) {
			com_sample++;

			if (state == RX) {
				com_mag_sum += mag;
				com_sub_sample++;

				/* detect the end of the transmission */
				if (detect_samples_under >= COMS_DETECT_SAMPLES_UNDER_THRS) {
					size_t com_blk_len = i - com_blk_start;
					burst.append(samples + com_blk_start, com_blk_len);
					com_blk_start = i;

					if (com_sub_sample >= COMS_DETECT_MIN_SAMPLES) {
						burst.subStop(COMS_DETECT_SAMPLES_UNDER_THRS);
						detect_samples_under = 0;
					} else {
						burst.subCancel();
					}
					state = COALESCING;
				}
			} else if (state == COALESCING &&
				   detect_samples_under >= com_coalescing_samples_count) {
				state = LISTEN;

				if (com_sample >= COMS_DETECT_MIN_SAMPLES) {
					if (burst.subBursts().size() > 0) {

						burst.done();

						std::cerr << burst.burstID()
							  << ": new communication, time = "
							  << burst.startTimeUs()
							  << " µs, len = "
							  << burst.len()
							  << " samples, len_time = "
							  << burst.lenTimeUs()
							  << " µs, sub-burst = "
							  << burst.subBursts().size()
							  << ", avg_pwr = "
							  << (com_mag_sum / com_sample) * 100 / com_thrs
							  << "% above threshold ("
							  << com_mag_sum / com_sample
							  << " vs noise mag max "
							  << noise_mag_max
							  << ")"
							  << std::endl;

						if (!process_burst(burst))
							return false;

						/* change the phy parameters, if wanted */
						/*phy.central_freq = 869.5e6;
						phy.sample_rate = 1500000;
						phy.IF_bw = 2000000;
						phy.gain = 45;
						return RET_CH_PHY;
						*/
					}
				}
			}
		}

	}

	/* we arrived at the end of this block, but a communication is still
	 * going on. Let's copy the data so as we don't loose it!
	*/
	if (state > LISTEN) {
		burst.append(samples + com_blk_start, count - com_blk_start - 1);
	}

	return true;
}

void RXTimeDomain::burst_dump_samples(const Burst &burst)
{
	char filename[100];
	sprintf(filename, "burst_%i.dat", burst.burstID());
	FILE *f = fopen(filename, "wb");
	fwrite(burst.samples(), sizeof(std::complex<short>), burst.len(), f);
	fclose(f);

	sprintf(filename, "burst_%i.csv", burst.burstID());
	f = fopen(filename, "wb");
	size_t b = 0;
	for (size_t i = 0; i < burst.len(); i++) {
		size_t inSubBurst = 0;
		if (i >= burst.subBursts()[b].start) {
			inSubBurst = (i < burst.subBursts()[b].end) ? 127 : 0;
			if (i > burst.subBursts()[b].end && b < burst.subBursts().size())
				b++;
		}

		fprintf(f, "%i, %i, %i\n", burst.samples()[i].real(), burst.samples()[i].imag(), inSubBurst);
	}
	fclose(f);
}

bool RXTimeDomain::process_burst(Burst &burst)
{
	// List of available demodulators
	OOK ook;
	FSK fsk;
	Demodulator *demod[] = {
		&ook,
		//&fsk,
		// Add demodulations here
	};

	// dump the samples to files
	burst_dump_samples(burst);

	Demodulator *fittest = NULL;
	uint8_t bestScore = 0;

	for (size_t i = 0; i < sizeof(demod) / sizeof(Demodulator *); i++) {
		Demodulator *d = demod[i];
		uint8_t score = d->likeliness(burst);
		if (!fittest || score > bestScore) {
			bestScore = score;
			fittest = d;
		}
		if (score == 255)
			break;
	}

	// bail out if the score is very low!
	if (!fittest || bestScore < 128) {
		float freq, freq_std;
		freq_get_avr(burst, freq, freq_std);
		std::cerr << "Demod: burst ID "
			  << burst.burstID()
			  << " has an unknown modulation. Len = "
			  << burst.lenTimeUs()
			  << " µs, sub-burst count = "
			  << burst.subBursts().size()
			  << ", freq = "
			  << freq / 1000000.0
			  << " Mhz"
			  << std::endl << std::endl;

		return true;
	}

	std::vector<Message> msgs = fittest->demod(burst);

	std::cerr << "New message: modulation = '" << fittest->modulationString()
		  << "', sub messages = " << msgs.size() << std::endl;
	for (size_t i = 0; i < msgs.size(); i++) {
		std::cerr << "Sub msg " << i;
		if (msgs[i].modulation())
			std::cerr << ", " << msgs[i].modulation()->toString();
		std::cerr << ": len = " << msgs[i].size() << ": " << std::endl
		<< "BIN: " << msgs[i].toString(Message::BINARY) << std::endl
		<< "HEX: " << msgs[i].toString(Message::HEX) << std::endl;

		Message man;
		if (Manchester::decode(msgs[i], man)) {
			std::cerr << "Manchester code detected:" << std::endl
				  << "BIN: " << man.toString(Message::BINARY) << std::endl
				  << "HEX: " << man.toString(Message::HEX) << std::endl;
			if (_userCb) {
				if (!_userCb(man, _phy, _userData))
					return false;
			}
		} else if (_userCb) {
			if (!_userCb(msgs[i], _phy, _userData))
					return false;
		}


		std::cerr << std::endl;
	}

	std::cerr << std::endl;

	return true;
}
