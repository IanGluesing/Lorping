# Tun Testing

The Giga boards, acting as a modem, can take advantage of your host computers networking stack. We can create two `utun` user tunnel interfaces, apply basic networking routing rules, and have our python test script act as a bridge, taking packets bound for those tunnels, and routing them through the serial connection to the Giga boards. The full data flow will look like this:

```
# Packets going out
Host computer: ping 10.99.0.2 -> utun6 -> python test script -> Giga Serial -> Giga Board A -> Sx1262 -> ))))) 

# Packets coming in
))))) -> Sx1262 -> Giga Board B -> Giga Serial -> python test script -> utun7 -> Generate ping response

# Sending Packet response
utun7 -> python test script -> Giga Serial -> Giga Board B -> Sx1262 -> )))))

# Receiving Packet Response
))))) -> Sx1262 -> Giga Board A -> Giga Serial -> python test script -> utun6 -> Host computer receives ping response
```

## Tun Setup

A `tun_test.py` script is included to setup networking accross two Giga boards using two `utun` user tunnel interfaces. Instructions and walkthrough for this are below, with examples following later.

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

### Sending a Ping

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

### Curl on a Simple Server

After the test has been setup, run the included simple_server python script in one terminal:

```
user@computer current_folder % python3 test_server.py
Listening on 0.0.0.0:8000
```

From another terminal, you may run the following and verify the expected response:

```
user@computer current_folder % curl http://10.99.0.2:8000/
Hello from the LoRa network!
```

Traffic can be verified by running the tcpdump utility on the `utun6` tunnel:

```
user@computer current_folder % sudo tcpdump -ni utun6 -vv -X
tcpdump: listening on utun6, link-type NULL (BSD loopback), snapshot length 524288 bytes
16:45:37.034038 IP (tos 0x0, ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 64)
    10.99.0.1.59154 > 10.99.0.2.8000: Flags [SEW], cksum 0x4fa7 (correct), seq 1738628816, win 65535, options [mss 1460,nop,wscale 6,nop,nop,TS val 102498028 ecr 0,sackOK,eol], length 0
	0x0000:  4500 0040 0000 4000 4006 25f0 0a63 0001  E..@..@.@.%..c..
	0x0010:  0a63 0002 e712 1f40 67a1 5ed0 0000 0000  .c.....@g.^.....
	0x0020:  b0c2 ffff 4fa7 0000 0204 05b4 0103 0306  ....O...........
	0x0030:  0101 080a 061b feec 0000 0000 0402 0000  ................
16:45:37.196080 IP (tos 0x0, ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 64)
    10.99.0.2.8000 > 10.99.0.1.59154: Flags [S.E], cksum 0x449d (correct), seq 762365735, ack 1738628817, win 65535, options [mss 1460,nop,wscale 6,nop,nop,TS val 2645326132 ecr 102498028,sackOK,eol], length 0
	0x0000:  4500 0040 0000 4000 4006 25f0 0a63 0002  E..@..@.@.%..c..
	0x0010:  0a63 0001 1f40 e712 2d70 c727 67a1 5ed1  .c...@..-p.'g.^.
	0x0020:  b052 ffff 449d 0000 0204 05b4 0103 0306  .R..D...........
	0x0030:  0101 080a 9dac 7934 061b feec 0402 0000  ......y4........
16:45:37.196225 IP (tos 0x0, ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 52)
    10.99.0.1.59154 > 10.99.0.2.8000: Flags [.], cksum 0x7c00 (correct), seq 1, ack 1, win 2059, options [nop,nop,TS val 102498190 ecr 2645326132], length 0
	0x0000:  4500 0034 0000 4000 4006 25fc 0a63 0001  E..4..@.@.%..c..
	0x0010:  0a63 0002 e712 1f40 67a1 5ed1 2d70 c728  .c.....@g.^.-p.(
	0x0020:  8010 080b 7c00 0000 0101 080a 061b ff8e  ....|...........
	0x0030:  9dac 7934                                ..y4
16:45:37.196538 IP (tos 0x2,ECT(0), ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 129)
    10.99.0.1.59154 > 10.99.0.2.8000: Flags [P.], cksum 0x2551 (correct), seq 1:78, ack 1, win 2059, options [nop,nop,TS val 102498191 ecr 2645326132], length 77
	0x0000:  4502 0081 0000 4000 4006 25ad 0a63 0001  E.....@.@.%..c..
	0x0010:  0a63 0002 e712 1f40 67a1 5ed1 2d70 c728  .c.....@g.^.-p.(
	0x0020:  8018 080b 2551 0000 0101 080a 061b ff8f  ....%Q..........
	0x0030:  9dac 7934 4745 5420 2f20 4854 5450 2f31  ..y4GET./.HTTP/1
	0x0040:  2e31 0d0a 486f 7374 3a20 3130 2e39 392e  .1..Host:.10.99.
	0x0050:  302e 323a 3830 3030 0d0a 5573 6572 2d41  0.2:8000..User-A
	0x0060:  6765 6e74 3a20 6375 726c 2f38 2e37 2e31  gent:.curl/8.7.1
	0x0070:  0d0a 4163 6365 7074 3a20 2a2f 2a0d 0a0d  ..Accept:.*/*...
	0x0080:  0a                                       .
16:45:37.620851 IP (tos 0x0, ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 129)
    10.99.0.1.59154 > 10.99.0.2.8000: Flags [P.], cksum 0x23a9 (correct), seq 1:78, ack 1, win 2059, options [nop,nop,TS val 102498615 ecr 2645326132], length 77
	0x0000:  4500 0081 0000 4000 4006 25af 0a63 0001  E.....@.@.%..c..
	0x0010:  0a63 0002 e712 1f40 67a1 5ed1 2d70 c728  .c.....@g.^.-p.(
	0x0020:  8018 080b 23a9 0000 0101 080a 061c 0137  ....#..........7
	0x0030:  9dac 7934 4745 5420 2f20 4854 5450 2f31  ..y4GET./.HTTP/1
	0x0040:  2e31 0d0a 486f 7374 3a20 3130 2e39 392e  .1..Host:.10.99.
	0x0050:  302e 323a 3830 3030 0d0a 5573 6572 2d41  0.2:8000..User-A
	0x0060:  6765 6e74 3a20 6375 726c 2f38 2e37 2e31  gent:.curl/8.7.1
	0x0070:  0d0a 4163 6365 7074 3a20 2a2f 2a0d 0a0d  ..Accept:.*/*...
	0x0080:  0a                                       .
16:45:37.789268 IP (tos 0x0, ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 52)
    10.99.0.2.8000 > 10.99.0.1.59154: Flags [.], cksum 0x77b7 (correct), seq 1, ack 78, win 2058, options [nop,nop,TS val 2645326728 ecr 102498615], length 0
	0x0000:  4500 0034 0000 4000 4006 25fc 0a63 0002  E..4..@.@.%..c..
	0x0010:  0a63 0001 1f40 e712 2d70 c728 67a1 5f1e  .c...@..-p.(g._.
	0x0020:  8010 080a 77b7 0000 0101 080a 9dac 7b88  ....w.........{.
	0x0030:  061c 0137                                ...7
16:45:37.900633 IP (tos 0x2,ECT(0), ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 190)
    10.99.0.2.8000 > 10.99.0.1.59154: Flags [P.], cksum 0x94c4 (correct), seq 1:139, ack 78, win 2058, options [nop,nop,TS val 2645326729 ecr 102498615], length 138
	0x0000:  4502 00be 0000 4000 4006 2570 0a63 0002  E.....@.@.%p.c..
	0x0010:  0a63 0001 1f40 e712 2d70 c728 67a1 5f1e  .c...@..-p.(g._.
	0x0020:  8018 080a 94c4 0000 0101 080a 9dac 7b89  ..............{.
	0x0030:  061c 0137 4854 5450 2f31 2e30 2032 3030  ...7HTTP/1.0.200
	0x0040:  204f 4b0d 0a53 6572 7665 723a 2042 6173  .OK..Server:.Bas
	0x0050:  6548 5454 502f 302e 3620 5079 7468 6f6e  eHTTP/0.6.Python
	0x0060:  2f33 2e31 302e 360d 0a44 6174 653a 2057  /3.10.6..Date:.W
	0x0070:  6564 2c20 3139 2041 7567 2032 3032 3620  ed,.19.Aug.2026.
	0x0080:  3231 3a34 353a 3337 2047 4d54 0d0a 436f  21:45:37.GMT..Co
	0x0090:  6e74 656e 742d 5479 7065 3a20 7465 7874  ntent-Type:.text
	0x00a0:  2f70 6c61 696e 0d0a 436f 6e74 656e 742d  /plain..Content-
	0x00b0:  4c65 6e67 7468 3a20 3239 0d0a 0d0a       Length:.29....
16:45:37.900715 IP (tos 0x0, ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 52)
    10.99.0.1.59154 > 10.99.0.2.8000: Flags [.], cksum 0x7615 (correct), seq 78, ack 139, win 2057, options [nop,nop,TS val 102498895 ecr 2645326729], length 0
	0x0000:  4500 0034 0000 4000 4006 25fc 0a63 0001  E..4..@.@.%..c..
	0x0010:  0a63 0002 e712 1f40 67a1 5f1e 2d70 c7b2  .c.....@g._.-p..
	0x0020:  8010 0809 7615 0000 0101 080a 061c 024f  ....v..........O
	0x0030:  9dac 7b89                                ..{.
16:45:38.452245 IP (tos 0x0, ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 219)
    10.99.0.2.8000 > 10.99.0.1.59154: Flags [FP.], cksum 0x57bf (correct), seq 1:168, ack 78, win 2058, options [nop,nop,TS val 2645327253 ecr 102498615], length 167
	0x0000:  4500 00db 0000 4000 4006 2555 0a63 0002  E.....@.@.%U.c..
	0x0010:  0a63 0001 1f40 e712 2d70 c728 67a1 5f1e  .c...@..-p.(g._.
	0x0020:  8019 080a 57bf 0000 0101 080a 9dac 7d95  ....W.........}.
	0x0030:  061c 0137 4854 5450 2f31 2e30 2032 3030  ...7HTTP/1.0.200
	0x0040:  204f 4b0d 0a53 6572 7665 723a 2042 6173  .OK..Server:.Bas
	0x0050:  6548 5454 502f 302e 3620 5079 7468 6f6e  eHTTP/0.6.Python
	0x0060:  2f33 2e31 302e 360d 0a44 6174 653a 2057  /3.10.6..Date:.W
	0x0070:  6564 2c20 3139 2041 7567 2032 3032 3620  ed,.19.Aug.2026.
	0x0080:  3231 3a34 353a 3337 2047 4d54 0d0a 436f  21:45:37.GMT..Co
	0x0090:  6e74 656e 742d 5479 7065 3a20 7465 7874  ntent-Type:.text
	0x00a0:  2f70 6c61 696e 0d0a 436f 6e74 656e 742d  /plain..Content-
	0x00b0:  4c65 6e67 7468 3a20 3239 0d0a 0d0a 4865  Length:.29....He
	0x00c0:  6c6c 6f20 6672 6f6d 2074 6865 204c 6f52  llo.from.the.LoR
	0x00d0:  6120 6e65 7477 6f72 6b21 0a              a.network!.
16:45:38.452347 IP (tos 0x0, ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 64)
    10.99.0.1.59154 > 10.99.0.2.8000: Flags [.], cksum 0x51f0 (correct), seq 78, ack 169, win 2057, options [nop,nop,TS val 102499447 ecr 2645327253,nop,nop,sack 1 {1:139}], length 0
	0x0000:  4500 0040 0000 4000 4006 25f0 0a63 0001  E..@..@.@.%..c..
	0x0010:  0a63 0002 e712 1f40 67a1 5f1e 2d70 c7d0  .c.....@g._.-p..
	0x0020:  b010 0809 51f0 0000 0101 080a 061c 0477  ....Q..........w
	0x0030:  9dac 7d95 0101 050a 2d70 c728 2d70 c7b2  ..}.....-p.(-p..
16:45:38.452703 IP (tos 0x0, ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 52)
    10.99.0.1.59154 > 10.99.0.2.8000: Flags [F.], cksum 0x71c2 (correct), seq 78, ack 169, win 2057, options [nop,nop,TS val 102499447 ecr 2645327253], length 0
	0x0000:  4500 0034 0000 4000 4006 25fc 0a63 0001  E..4..@.@.%..c..
	0x0010:  0a63 0002 e712 1f40 67a1 5f1e 2d70 c7d0  .c.....@g._.-p..
	0x0020:  8011 0809 71c2 0000 0101 080a 061c 0477  ....q..........w
	0x0030:  9dac 7d95                                ..}.
16:45:38.614163 IP (tos 0x0, ttl 64, id 0, offset 0, flags [DF], proto TCP (6), length 52)
    10.99.0.2.8000 > 10.99.0.1.59154: Flags [.], cksum 0x7093 (correct), seq 169, ack 79, win 2058, options [nop,nop,TS val 2645327555 ecr 102499447], length 0
	0x0000:  4500 0034 0000 4000 4006 25fc 0a63 0002  E..4..@.@.%..c..
	0x0010:  0a63 0001 1f40 e712 2d70 c7d0 67a1 5f1f  .c...@..-p..g._.
	0x0020:  8010 080a 7093 0000 0101 080a 9dac 7ec3  ....p.........~.
	0x0030:  061c 0477                                ...w
```
