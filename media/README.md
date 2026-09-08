# Examples

## Ping

Any hopping pattern may be used, with simple hop slot incrementing shown below:

![Spectrum Analyzer and Waterfall Display](ping_hopping.gif)
*Spectrum analyzer and waterfall visualization of a 10Hz frequency-hopped `ping -c 10 -W 2000 10.99.0.2` command between two LoRa network peers*

A pseudo-random pattern can also be used, and verified with tcpdump:

![Spectrum Analyzer and Waterfall Display](ping_30hz_random_hopping.gif)
*Spectrum analyzer and waterfall visualization of a 30Hz frequency-hopped `ping -c 10 -W 2000 10.99.0.2` command between two LoRa network peers*