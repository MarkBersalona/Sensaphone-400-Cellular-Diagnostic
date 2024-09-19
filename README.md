# Sensaphone 400 Cellular Diagnostic tool
<img src="Sensaphone 400 Cellular Diagnostic 20230314.png" alt="Sensaphone 400 Cellular screenshot, 2023.03.14" />

This is the Diagnostic tool for the **Sensaphone 400 Cellular**

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

### Problem recognizing ttyUSB0?
First, **verify the USB-to-serial cable is plugged into a USB port**. (I know, obvious, but I forgot to plug it in while testing these instructions.)

In some instances the tool might not immediately recognize ttyUSB0, the Linux device for a USB-to-serial converter, or allow non-root access to it. See https://askubuntu.com/questions/133235/how-do-i-allow-non-root-access-to-ttyusb0 for a detailed explanation, but what's worked for me is the following:
1. Confirm ttyUSB0 is in the user group 'dialout'
```
stat /dev/ttyUSB0
```
2. Add the user to the dialout group using the following command
```
sudo usermod -a -G dialout $USER
```
3. Logout and log back into the account. The tool should recognize ttyUSB0 and work correctly.
4. (Optional) Verify the user is included in the dialout group. The following command lists the groups to which the user belongs
```
sudo groups $USER
```


## 400 Cellular Description

The 400 Cellular monitors the following inputs, or *zones*:
- 0: Power - the DC mains input; raw value is 0 if disconnected, 0x1FFF if connected
- 1:Battery - the rechargeable battery, used as power backup if mains power is disconnected; expressed as a percentage, from 0-100
- 2: Lithium - the CR2032 lithium cell, used to maintain the real-time clock; expressed as a percentage, from 0-100
- 3: Analog input 1, colloquially "Zone 1"
- 4: Analog input 2, colloquially "Zone 2"
- 5: Analog input 3, colloquially "Zone 3"
- 6: Analog input 4, colloquially "Zone 4"
- 7: Analog input 5, colloquially "Zone 5"

The 400 Cellular also controls the following output zone:
- 8: Output relay; 0 for off/open; 1 for on/closed

NOTE: Zone numbering internal to the device is 0-based, but zone numbering in the Zone Update and Alarm POSTs is 1-based. For example, the internal input zone for Power is zone 0, but in the POSTs Power is zone 1. The dichotomy comes from the Sentinel devices, and since 400 Cellular at both the device and server ends is based on Sentinel, this dichotomy is carried over.

Numeric values in Zone Update and Alarm POSTs from the device to the server are generally represented as integers, with the least 3 digits representing decimal values. For example, the value of pi (3.1415926...) is represented as 3141 or possibly rounded to 3142; the value of 95.832% is represented as 95832.

## Diagnostic Description

<img src="400 Cellular Diagnostic block diagram.png" alt="Sensaphone 400 Cellular Diagnostic block diagram" />

### Summary

The **Diagnostic tool reads the debug serial output of the 400 Cellular**, which consists of ASCII strings ending with CRLF (carriage return/linefeed) (as if outputting to a serial terminal).
- Each line is scanned for relevant data to be extracted and displayed individually, such as the device MAC/SN or zone values.
- A status display shows important status of the device under test and of the Diagnostic tool, such as warning and error messages.
- Each debug printout line is shown in the Receive display.
- If enabled, each debug printout line is also saved to a log file (with CRLF line terminations).

The **Diagnostic tool can accept operator inputs** to pass along to the device under test.
- AT commands to be passed to the SARA-R5 cellular transceiver
- The simple MENU commands, which require only the MENU command to be selected and then sent to the device under test
- the MENU command to write a new MAC/SN to the device under test, which requires the operator to enter a valid MAC address in the entry box provided
- the MENU command to write a new Board revision to the device under test, which requires the operator to enter a board revision letter (A-Z) in the entry box provided

### Details

The **Diagnostic tool expects to connect to the Linux ttyUSB0 device**, the device name for a serial-to-USB converter. The connection status to ttyUSB0 will be given in the Status display.

**All debug printouts** received by the Diagnostic tool **are first stored in a software FIFO** by main_receive_msg_write(). The FIFO was added in anticipation of possible system slowdowns when writing the debug printouts to a log file. In practice system buffers and caches seem to mitigate any throughput bottlenecks with the log file, but the FIFO probably helps make the Diagnostic tool robust. At 200 entries deep, the FIFO doesn't seem to get much above 10% usage.

The **serial receive callback function** serial_read() is essentially the ***interrupt service routine (ISR) for received serial data***. It collects the received serial data; when CRLF is received it strips off the CRLF, terminates the string with a NULL and saves the string to the FIFO.
- As an ISR, this routine must spend as little time as possible executing. Setting variables, moving small amounts of data around are OK; time delays or waiting around for user inputs are bad; any processing that could be done at the task level or otherwise outside the ISR should be moved out of the ISR. "Get in, do what's needed, get out."

The **periodic function** main_periodic() of the Diagnostic tool **checks for fresh data in the FIFO**. If there are any, it reads each string from the FIFO, parses it for any relevant data to display, displays the string in the Receive display and optionally saves it to a log file (with CRLF line terminations). The periodic function also **checks the serial connection to the device under test**: if the connection to ttyUSB0 is good, it is assumed the device under test is connected.

**Processing of operator inputs** is performed by the callback routines triggered by the button_clicked events of the relevant Diagnostic tool buttons.
- Operator-entered **MAC address and board rev are validated** before passing these to the device under test. A MAC address must consist of 6 hex values separated by delimiters, of the form XX-XX-XX-XX-XX-XX. A board rev must be a single letter in range [A-Z], though case-insensitive.
- Operator-entered **AT command is passed** to the device under test, to the SARA-R5 cellular transceiver, **as is**.

Both the Status and Receive text windows adjust to display the most recent text string; that is, both move to the bottom of the respective text buffer. The operator may scroll either to view older text strings, but when a new text string is to be displayed, the display will move back to the bottom of the text buffer. Additionally, the periodic function moves the Status window to the bottom of the Status text buffer, so in practice manually scrolling the Status window tends to be futile.
- The code in both display_status_write() and display_receive_write() to move the text window to the bottom of the buffer is insufficient to move **immediately and reliably**. GTK seems to do a *lazy update* when it comes to moving text windows, updating (or not!) only when higher-priority display tasks have finished. To ensure the Status text window always shows the latest status, the periodic function also moves the Status text window to the bottom of the buffer. Thus, the Status display reliably shows the latest status.

To ensure the Receive text buffer doesn't get too large, the Receive text buffer is emptied every 5 minutes.

When enabled, the logfile filename uses the local date and time to prefix "400 Cellular.txt", so an example would be "20230327 0807 400 Cellular.txt" which will be in the same directory as the Diagnostic tool.

On startup or reboot, the 400 Cellular debug port outputs 3 instances of "*Sensaphone 400 Cellular starting...*" which the Diagnostic tool uses to detect device startup/reboot and reset its displays.
- There are 3 instances to ensure the device startup/reboot is detected even in the case of a garbled transmission or reception: assumes at least 1 complete message will be detected.


## Suggested changes
- Add an 'About' button to display version and date of Sensaphone 400 Cellular Diagnostic to Status
- Capture and display GNSS timestamp along with lat/long; if lat/long unavailable, do NOT clear lat/long/GNSS timestamp (This makes lat/long 'sticky': an advantage if the device is stationary; a disadvantage if the device is mobile)
- Option to dump Status contents to log file
- Reduce the initial height of the Receive window and the overall window, so Diagnostic can fit on a 1366x768 display. (Still manually resizable; maybe get fancy and automatically extend height to available display height??) This is what the Diagnostic tool looks like on a 1366x768 screen:

<img src="Diagnostic at 1366x768.png" alt="Diagnostic tool on 1366x768 screen" />

This is what the Diagnostic tool looks like on a 1366x768 screen, with the textboxes and associated labels and buttons with the "Visible" attribute turned off: at least more of the Receive window shows up; bottom still cut off...

<img src="Diagnostic at 1366x768 invisible textboxes.png" alt="Diagnostic tool with invisible textboxes on 1366x768 screen" />

