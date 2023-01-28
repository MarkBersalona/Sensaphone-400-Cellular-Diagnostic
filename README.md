# Sensaphone 400 Cellular Diagnostic tool
This is the Diagnostic tool for the Sensaphone 400 Cellular
- Runs in a Debian-based Linux distribution, like Linux Mint. (It can probably run in an Arch- or Fedora- or other non-Debian Linux distro, I simply haven't tried building it in one yet.)
- Connects to the serial debug port of the 400 Cellular via serial-to-USB converter; expects 
connection to the ttyUSB0 device

## Prerequisites
Requires the GTK 3 library. To install:

```
sudo apt-get update
sudo apt-get install libgtk-3-dev
```

## Build instructions
1. Clone Sensaphone-400-Cellular-Diagnostic. If you're reading this, you're already likely in my 
GitHub project for Sensaphone-400-Cellular-Diagnostic, but if you're not it's at 
https://github.com/MarkBersalona/Sensaphone-400-Cellular-Diagnostic
3. In a terminal move to the directory in which the repository has been cloned. Example: on my Linux 
laptop I cloned it to /home/mark/GTKProjects/Sensaphone-400-Cellular-Diagnostic
4. From the terminal use the following command: `make`
5. If all goes well, the application '400_cellular_diagnostic' will be in ../dist/Debug/GNU-Linux *and* in the current directory
6. Run 400_cellular_diagnostic
   - Will need a USB-to-serial cable and a Sensaphone serial card
   - Plug USB end into the Linux PC, the serial end to the Sensaphone serial card; plug the wire header onto the 400 Cellular serial debug port (take care to orient correctly!)
   - Run the 400_cellular_diagnostic app in the application directory, the one with the .glade and .css files. The app expects to find and read these files in the same directory where it itself is located.

## 400 Cellular Description

The 400 Cellular monitors the following inputs, or *zones*:
- 0: Power - the DC mains input; raw value is 0 if disconnected, 0x7FFF if onnected
- 1:Battery - the rechargeable battery, used as power backup if mains power is disconnected; expressed as a percentage, from 0-100
- 2: Lithium - the CR2 lithium cell, used to maintain the real-time clock; expressed as a percentage, from 0-100
- 3: Internal temperature - the temperature reading from the on-board humidity sensor; value is in Celsius
- 4: Internal humidity - the humidity reading from the on-board humidity sensor; expressed as a percentage, from 0-100
- 5: Analog input 1, colloquially "Zone 1"
- 6: Analog input 2, colloquially "Zone 2"
- 7: Analog input 3, colloquially "Zone 3"
- 8: Analog input 4, colloquially "Zone 4"

The 400 Cellular also controls the following output zone:
- 9: Output relay; 0 for off/open; 1 for on/closed

