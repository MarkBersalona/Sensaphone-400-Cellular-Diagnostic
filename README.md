# Sensaphone 400 Cellular Diagnostic tool
This is the Diagnostic tool for the Sensaphone 400 Cellular
- Runs in a Debian-based Linux distribution, like Linux Mint
- Connects to the serial debug port of the 400 Cellular via serial-to-USB converter; expects 
connection to the ttyUSB0 device

## Prerequisites
Requires the GTK 3 library. To install:

```
sudo apt-get update
sudo apt-get install libgtk-3 dev
```

## Build instructions
Move to the directory with the source code in a terminal and type `make` (RETURN) at the terminal prompt.

