# Tun Ping Test

The Giga boards, acting as a modem, can take advantage of your host computers networking stack. We can create two `utun` user tunnel interfaces, apply basic networking routing rules, and have our python test script act as a bridge, taking packets bound for those tunnels, and routing them through the serial connection to the Giga boards. The full data flow will look like this:

```
# Ping going out
Host computer: ping 10.99.0.2 -> utun6 -> python test script -> Giga Serial -> Giga Board A -> Sx1262 -> ))))) 

# Picking up the ping
))))) -> Sx1262 -> Giga Board B -> Giga Serial -> python test script -> utun7 -> Generate ping response

# Sending Ping response
utun7 -> python test script -> Giga Serial -> Giga Board B -> Sx1262 -> )))))

# Picking up the ping response
))))) -> Sx1262 -> Giga Board A -> Giga Serial -> python test script -> utun6 -> Host computer receives ping response
```

## Ping Test Setup

A `tun_test.py` script is included to demonstrate a networking ping accross two Giga boards using two `utun` user tunnel interfaces. Instructions and walkthrough for this are below

```
# Creating the virtual python environment
python3 -m venv env

# Source the virtual environment and install required packages
source env/bin/activate
pip3 install -r requirements.txt

# Running the script
sudo python3 tun_test.py
```

After running the attached python script, there will be some general information about the created interfaces printed to the screen, as well as other steps we need to do manually.

In the current state, we will have created two `utun` interfaces:

```
utun6: flags=8051<UP,POINTOPOINT,RUNNING,MULTICAST> mtu 1500
utun7: flags=8051<UP,POINTOPOINT,RUNNING,MULTICAST> mtu 1500
```

If we then assign these rules to them:

```
sudo ifconfig utun6 10.99.0.1 10.99.0.2
sudo ifconfig utun7 10.99.0.2 10.99.0.1
```

Our setup will now look like this:

```
utun6: flags=8051<UP,POINTOPOINT,RUNNING,MULTICAST> mtu 1500
	inet 10.99.0.1 --> 10.99.0.2 netmask 0xff000000
utun7: flags=8051<UP,POINTOPOINT,RUNNING,MULTICAST> mtu 1500
	inet 10.99.0.2 --> 10.99.0.1 netmask 0xff000000
```

## Sending a Ping

After the test has been setup, a ping can be sent across the two Giga boards:

```
user@computer current_folder % ping -c 1 10.99.0.2
PING 10.99.0.2 (10.99.0.2): 56 data bytes

--- 10.99.0.2 ping statistics ---
1 packets transmitted, 1 packets received, 0.0% packet loss, 1 packets out of wait time
round-trip min/avg/max/stddev = 330.848/330.848/330.848/0.000 ms
```

Debug logs will be printed to the python script window, showing raw serial data and packets sent/received between the Giga boards and python test script:

```
TunA Sending Packet to Giga A:
45 00 00 54 0B 5F 00 00 40 01 5A 82 0A 63 00 01 0A 63 00 02 08 00 50 18 56 BD 00 00 6A 82 58 FD 00 05 A2 A2 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35 36 37

RAW SERIAL 86 bytes:
00 54 45 00 00 54 0B 5F 00 00 40 01 5A 82 0A 63 00 01 0A 63 00 02 08 00 50 18 56 BD 00 00 6A 82 58 FD 00 05 A2 A2 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35 36 37
FRAME: length=84, buffer=86
TunB Got Packet from Giga B:
45 00 00 54 0B 5F 00 00 40 01 5A 82 0A 63 00 01 0A 63 00 02 08 00 50 18 56 BD 00 00 6A 82 58 FD 00 05 A2 A2 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35 36 37
UTUN B send() returned: 88/88 bytes
TunB Sending Packet to Giga B:
45 00 00 54 A3 96 00 00 40 01 C2 4A 0A 63 00 02 0A 63 00 01 00 00 58 18 56 BD 00 00 6A 82 58 FD 00 05 A2 A2 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35 36 37

RAW SERIAL 86 bytes:
00 54 45 00 00 54 A3 96 00 00 40 01 C2 4A 0A 63 00 02 0A 63 00 01 00 00 58 18 56 BD 00 00 6A 82 58 FD 00 05 A2 A2 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35 36 37
FRAME: length=84, buffer=86
TunA Got Packet from Giga A:
45 00 00 54 A3 96 00 00 40 01 C2 4A 0A 63 00 02 0A 63 00 01 00 00 58 18 56 BD 00 00 6A 82 58 FD 00 05 A2 A2 08 09 0A 0B 0C 0D 0E 0F 10 11 12 13 14 15 16 17 18 19 1A 1B 1C 1D 1E 1F 20 21 22 23 24 25 26 27 28 29 2A 2B 2C 2D 2E 2F 30 31 32 33 34 35 36 37
UTUN A send() returned: 88/88 bytes
```

Traffic can be verified by running the tcpdump utility on the `utun6` tunnel:

```
user@computer current_folder % sudo tcpdump -ni utun6 -vv -X icmp

tcpdump: listening on utun6, link-type NULL (BSD loopback), snapshot length 524288 bytes
19:43:50.125110 IP (tos 0x0, ttl 64, id 8200, offset 0, flags [none], proto ICMP (1), length 84)
    10.99.0.1 > 10.99.0.2: ICMP echo request, id 190, seq 0, length 64
	0x0000:  4500 0054 2008 0000 4001 45d9 0a63 0001  E..T....@.E..c..
	0x0010:  0a63 0002 0800 637a 00be 0000 6a82 5946  .c....cz....j.YF
	0x0020:  0001 e4fa 0809 0a0b 0c0d 0e0f 1011 1213  ................
	0x0030:  1415 1617 1819 1a1b 1c1d 1e1f 2021 2223  .............!"#
	0x0040:  2425 2627 2829 2a2b 2c2d 2e2f 3031 3233  $%&'()*+,-./0123
	0x0050:  3435 3637                                4567
19:43:50.455302 IP (tos 0x0, ttl 64, id 64673, offset 0, flags [none], proto ICMP (1), length 84)
    10.99.0.2 > 10.99.0.1: ICMP echo reply, id 190, seq 0, length 64
	0x0000:  4500 0054 fca1 0000 4001 693f 0a63 0002  E..T....@.i?.c..
	0x0010:  0a63 0001 0000 6b7a 00be 0000 6a82 5946  .c....kz....j.YF
	0x0020:  0001 e4fa 0809 0a0b 0c0d 0e0f 1011 1213  ................
	0x0030:  1415 1617 1819 1a1b 1c1d 1e1f 2021 2223  .............!"#
	0x0040:  2425 2627 2829 2a2b 2c2d 2e2f 3031 3233  $%&'()*+,-./0123
	0x0050:  3435 3637                                4567
```