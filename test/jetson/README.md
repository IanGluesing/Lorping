# Jetson Pseudo PPS

TODO: Add requirements.txt

If a GPS module is not available or you dont want to buy one(like me), a jetson orin nano super dev kit is used in its place, creating a simulated 1Hz PPS signal to each giga board. 

I am aware of the flaws with this approach but it is good enough for simple testing.

A driver script: `pseudo_pps.py` is included to simulate the PPS signal from the jetson to the giga board.

```
# Create environment
python3 -m venv env
source env/bin/activate
pip3 install -r requirement.txt

# Run test script
python3 pseudo_pps.py

# If you are having trouble with permissions
sudo busybox devmem 0x2434040 w 0x4
sudo busybox devmem 0x2430070 w 0x8
sudo ./env/bin/python3 pseudo_pps.py
```