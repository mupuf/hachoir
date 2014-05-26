hachoir
=======

This repository contains my experimentations towards achieving a real-life cognitive radio network:

- gr-gtsrc: A GNU Radio bloc that performs sensing in the frequency domain
- hachoir_uhd: A time-domain transmission detection for OOK, FSK (WIP) and PSK(WIP) and transmission (any modulation). It supports Ettus' USRPs, Nuand Bladerf and the RTLSDR TNT dongles.

My goal is to finish hachoir_uhd to get a reliable link using FSK/PSK/QAM4 and then use transmission detection in the frequency domain to achieve sensing and reception of multiple transmissions at the same time.
