# Testing

A driver script: `serial_tx_driver.py` is included to validate input sent to the TX Giga is received on the RX Giga.

```
# Create environment
python3 -m venv env
source env/bin/activate
pip3 install -r requirement.txt

# Run test script
python3 serial_tx_driver.py

# If you are having trouble with serial permissions
sudo ./env/bin/python3 serial_tx_driver.py
```