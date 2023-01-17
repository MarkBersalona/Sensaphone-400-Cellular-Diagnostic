# Sensaphone 400 Cellular Diagnostic tool
This is the Diagnostic tool for the Sensaphone 400 Cellular
- Runs in a Debian-based Linux distribution, like Linux Mint
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
5. If all goes well, the application '400_cellular_diagnostic' will be in ../dist/Debug/GNU-Linux
6. Move or copy '400_cellular_diagnostic' back up to the Sensaphone-400-Cellular-Diagnostic directory, the one 
with 400CellularDiagnostic.glade and theme.css.
   - Will need a USB-to-serial cable and a Sensaphone serial card
   - Plug USB end into the Linux PC, the serial end to the Sensaphone serial card; plug the wire header onto the 400 Cellular serial debug port (take care to orient correctly!)
   - Run 400_cellular_diagnostic

