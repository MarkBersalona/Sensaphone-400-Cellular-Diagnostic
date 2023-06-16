# Sensaphone 400 Cellular Diagnostic tool
<img src="Sensaphone 400 Cellular Diagnostic 20230314.png" alt="Sensaphone 400 Cellular screenshot, 2023.03.14" />

This is the Diagnostic tool for the <b>Sensaphone 400 Cellular</b>

- Runs in a Debian-based Linux distribution, like Linux Mint. (It can probably run in an Arch- or Fedora- or other non-Debian Linux distro, I simply haven't tried building it in one yet.)
- Connects to the serial debug port of the 400 Cellular via serial-to-USB converter; expects 
connection to the ttyUSB0 device
- Works best on a 1920x1080 or larger display

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
- 0: Power - the DC mains input; raw value is 0 if disconnected, 0x1FFF if connected
- 1:Battery - the rechargeable battery, used as power backup if mains power is disconnected; expressed as a percentage, from 0-100
- 2: Lithium - the CR2032 lithium cell, used to maintain the real-time clock; expressed as a percentage, from 0-100
- 3: Internal temperature - the temperature reading from the on-board humidity sensor; value is in Celsius
- 4: Internal humidity - the humidity reading from the on-board humidity sensor; expressed as a percentage, from 0-100
- 5: Analog input 1, colloquially "Zone 1"
- 6: Analog input 2, colloquially "Zone 2"
- 7: Analog input 3, colloquially "Zone 3"
- 8: Analog input 4, colloquially "Zone 4"

The 400 Cellular also controls the following output zone:
- 9: Output relay; 0 for off/open; 1 for on/closed

NOTE: Zone numbering internal to the device is 0-based, but zone numbering in the Zone Update and Alarm POSTs is 1-based. For example, the internal input zone for Power is zone 0, but in the POSTs Power is zone 1. The dichotomy comes from the Sentinel devices, and since 400 Cellular at both the device and server ends is based on Sentinel, this dichotomy is carried over.

Numeric values in Zone Update and Alarm POSTs from the device to the server are generally represented as integers, with the least 3 digits representing decimal values. For example, the value of pi (3.1415926...) is represented as 3141 or possibly rounded to 3142; the value of 95.832% is represented as 95832.

## Diagnostic Description

<img src="400 Cellular Diagnostic block diagram.png" alt="Sensaphone 400 Cellular Diagnostic block diagram" />

### Summary

The <b>Diagnostic tool reads the debug serial output of the 400 Cellular</b>, which consists of ASCII strings ending with CRLF (carriage return/linefeed) (as if outputting to a serial terminal).
- Each line is scanned for relevant data to be extracted and displayed individually, such as the device MAC/SN or zone values.
- A status display shows important status of the device under test and of the Diagnostic tool, such as warning and error messages.
- Each debug printout line is shown in the Receive display.
- If enabled, each debug printout line is also saved to a log file (with CRLF line terminations).

The <b>Diagnostic tool can accept operator inputs</b> to pass along to the device under test.
- AT commands to be passed to the SARA-R5 cellular transceiver
- The simple MENU commands, which require only the MENU command to be selected and then sent to the device under test
- the MENU command to write a new MAC/SN to the device under test, which requires the operator to enter a valid MAC address in the entry box provided
- the MENU command to write a new Board revision to the device under test, which requires the operator to enter a board revision letter (A-Z) in the entry box provided

### Details

The <b>Diagnostic tool expects to connect to the Linux ttyUSB0 device</b>, the device name for a serial-to-USB converter. The connection status to ttyUSB0 will be given in the Status display.

<b>All debug printouts</b> received by the Diagnostic tool <b>are first stored in a software FIFO</b> by main_receive_msg_write(). The FIFO was added in anticipation of possible system slowdowns when writing the debug printouts to a log file. In practice system buffers and caches seem to mitigate any throughput bottlenecks with the log file, but the FIFO probably helps make the Diagnostic tool robust. At 200 entries deep, the FIFO doesn't seem to get much above 10% usage.

The <b>serial receive callback function</b> serial_read() is essentially the <em><b>interrupt service routine (ISR) for received serial data</b></em>. It collects the received serial data; when CRLF is received it strips off the CRLF, terminates the string with a NULL and saves the string to the FIFO.
- As an ISR, this routine must spend as little time as possible executing. Setting variables, moving small amounts of data around are OK; time delays or waiting around for user inputs are bad; any processing that could be done at the task level or otherwise outside the ISR should be moved out of the ISR. "Get in, do what's needed, get out."

The <b>periodic function</b> main_periodic() of the Diagnostic tool <b>checks for fresh data in the FIFO</b>. If there are any, it reads each string from the FIFO, parses it for any relevant data to display, displays the string in the Receive display and optionally saves it to a log file (with CRLF line terminations). The periodic function also <b>checks the serial connection to the device under test</b>: if the connection to ttyUSB0 is good, it is assumed the device under test is connected.

<b>Processing of operator inputs</b> is performed by the callback routines triggered by the button_clicked events of the relevant Diagnostic tool buttons.
- Operator-entered <b>MAC address and board rev are validated</b> before passing these to the device under test. A MAC address must consist of 6 hex values separated by delimiters, of the form XX-XX-XX-XX-XX-XX. A board rev must be a single letter in range [A-Z], though case-insensitive.
- Operator-entered <b>AT command is passed</b> to the device under test, to the SARA-R5 cellular transceiver, <b>as is</b>.

Both the Status and Receive text windows adjust to display the most recent text string; that is, both move to the bottom of the respective text buffer. The operator may scroll either to view older text strings, but when a new text string is to be displayed, the display will move back to the bottom of the text buffer. Additionally, the periodic function moves the Status window to the bottom of the Status text buffer, so in practice manually scrolling the Status window tends to be futile.
- The code in both display_status_write() and display_receive_write() to move the text window to the bottom of the buffer is insufficient to move <b>immediately and reliably</b>. GTK seems to do a *lazy update* when it comes to moving text windows, updating (or not!) only when higher-priority display tasks have finished. To ensure the Status text window always shows the latest status, the periodic function also moves the Status text window to the bottom of the buffer. Thus, the Status display reliably shows the latest status.

To ensure the Receive text buffer doesn't get too large, the Receive text buffer is emptied every 5 minutes.

When enabled, the logfile filename uses the local date and time to prefix "400 Cellular.txt", so an example would be "20230327 0807 400 Cellular.txt" which will be in the same directory as the Diagnostic tool.

On startup or reboot, the 400 Cellular debug port outputs 3 instances of "*Sensaphone 400 Cellular starting...*" which the Diagnostic tool uses to detect device startup/reboot and reset its displays.
- There are 3 instances to ensure the device startup/reboot is detected even in the case of a garbled transmission or reception: assumes at least 1 complete message will be detected.


## Suggested changes
- Update Status timestamp on the real-time minute instead of elapsed time minutes
- Capture and display GNSS timestamp along with lat/long; if lat/long unavailable, do NOT clear lat/long/GNSS timestamp
- Option to dump Status contents to log file
- Reduce the initial height of the Receive window and the overall window, so Diagnostic can fit on a 1366x768 display. (Still manually resizable; maybe get fancy and automatically extend height to available display height??)
- Add an 'About' button to display version and date of Sensaphone 400 Cellular Diagnostic to Status

