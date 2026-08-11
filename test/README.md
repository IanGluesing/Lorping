# Testing

Testing assumes the following hardware is available:

- Nvidia Jetson Orin Nano Super Dev Kit
- 2x Arduino Giga R1 Wifi
- Host Computer

The Jetson will be connected to each Giga with a Ground and GPIO in order to simulate a PPS and act as a stand in GNSS module.

Each Giga board will be power over serial by your host computer in order to monitor debug serial logs.

TODO: 2 Pictures here showing the jetson connected to each giga