#!/bin/bash
sudo minicom -D /dev/ttyACM0 -b 19200 -Hw
