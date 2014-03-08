#include <rtl-sdr.h>

#include <boost/program_options.hpp>
#include <boost/format.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <fstream>
#include <csignal>
#include <complex>

#include "../com_detect.h"

namespace po = boost::program_options;

static bool stop_signal_called = false;
void sig_int_handler(int){stop_signal_called = true;}

bool rtl_set_phy(rtlsdr_dev_t *dev, const phy_parameters_t &phy)
{

#if 0
	//set the center frequency
	std::cout << boost::format("Setting RX Freq: %f MHz...") % (phy.central_freq/1e6) << std::endl;
	uhd::tune_request_t tune_request(phy.central_freq);
	if(int_n_tuning) tune_request.args = uhd::device_addr_t("mode_n=integer");
	uhd::tune_result_t ret = usrp->set_rx_freq(tune_request);
	std::cout << boost::format("Actual RX Freq: %f MHz...") % (usrp->get_rx_freq()/1e6) << std::endl << std::endl;

	float tune_err = fabs(ret.target_rf_freq - ret.actual_rf_freq) / ret.target_rf_freq;
	if (tune_err > 0.01) {
		std::cerr << "Set_PHY: Tune error > 1% (" << tune_err * 100 << " %), abort" << std::endl;
		return false;
	}

	//set the sample rate
	if (phy.sample_rate <= 0.0){
		std::cerr << "Please specify a valid sample rate" << std::endl;
		return false;
	}
	std::cout << boost::format("Setting RX Rate: %f Msps...") % (phy.sample_rate/1e6) << std::endl;
	usrp->set_rx_rate(phy.sample_rate);
	std::cout << boost::format("Actual RX Rate: %f Msps...") % (usrp->get_rx_rate()/1e6) << std::endl << std::endl;

	//set the rf gain
	if (phy.gain >= 0.0) {
		std::cout << boost::format("Setting RX Gain: %f dB...") % phy.gain << std::endl;
		usrp->set_rx_gain(phy.gain);
		std::cout << boost::format("Actual RX Gain: %f dB...") % usrp->get_rx_gain() << std::endl << std::endl;
	}

	//set the IF filter bandwidth
	if (phy.IF_bw >= 0.0) {
		std::cout << boost::format("Setting RX Bandwidth: %f MHz...") % phy.IF_bw << std::endl;
		usrp->set_rx_bandwidth(phy.IF_bw);
		std::cout << boost::format("Actual RX Bandwidth: %f MHz...") % usrp->get_rx_bandwidth() << std::endl << std::endl;
	}

	// wait for lo_locked */
	std::cout << "Waiting on the LO: ";
	int i = 0;
	while (not usrp->get_rx_sensor("lo_locked").to_bool()){
		boost::this_thread::sleep(boost::posix_time::milliseconds(1));
		i++;
		if (i % 100)
			std::cout << "+ ";
		if (i > 1000) {
			std::cout << "failed!" << std::endl;
			return false;
		}
	}
	std::cout << "locked!" << std::endl << std::endl;
#endif
	return true;
}

int main(int argc, char *argv[])
{
	phy_parameters_t phy;
	rtlsdr_dev_t *dev;

	//setup the program options
	po::options_description desc("Allowed options");
	desc.add_options()
		("help", "help message")
		("rate", po::value<float>(&phy.sample_rate)->default_value(1e6), "rate of incoming samples")
		("freq", po::value<float>(&phy.central_freq)->default_value(0.0), "RF center frequency in Hz")
		("gain", po::value<float>(&phy.gain), "gain for the RF chain")
	;
	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	//print the help message
	if (vm.count("help")){
		std::cout << boost::format("UHD RX samples to file %s") % desc << std::endl;
		return ~0;
	}

	phy.IF_bw = -1.0;
	if (not vm.count("gain"))
		phy.gain = -1.0;

	std::signal(SIGINT, &sig_int_handler);
	std::cout << "Press Ctrl + C to stop streaming..." << std::endl << std::endl;

	bool phy_ok, start_over;
	do {
		phy_ok = rtl_set_phy(dev, phy);
		if (!phy_ok)
			continue;

		/* Treat */

		//finished
		std::cout << std::endl << "Done!" << std::endl << std::endl;
	} while (phy_ok && start_over);

	return EXIT_SUCCESS;
}
