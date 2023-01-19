/* 
 * File:   serial.c
 * Author: Mark Bersalona
 *
 * Created on 2023.01.16
 * Sensaphone 400 Cellular Diagnostic serial
 */


///////////////////////////////////////////////////////////////////////////////
//
// Includes
//
///////////////////////////////////////////////////////////////////////////////

#include <gtk/gtk.h>
//#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>          // File Control Definitions
#include <termios.h>        // POSIX Terminal Control definitions
#include <string.h>
#include "gconfig.h"
#include "main.h"
#include "display.h"

///////////////////////////////////////////////////////////////////////////////
//
// Variables and tables
//
///////////////////////////////////////////////////////////////////////////////

//
// serial-to-USB
//
gboolean isUSBConnectionOK = FALSE;
gboolean isFirstSerialFail = TRUE;
int fd; /* file descriptor of the port */

char lcSerialTempString[40];


////////////////////////////////////////////////////////////////////////////
// Name:         serial_open
// Description:  open a named IO device (file) with a variable baud rate
// Parameters:   char *name - name of port to open
//               int baud - baud rate
// Return:       
////////////////////////////////////////////////////////////////////////////
int 
serial_open(char *name, int baud)
{

    int b_baud;
    struct termios t; /* struct for port settings */

    switch (baud) 
    {
        case 4800:
            b_baud = B4800; break;
        case 9600:
            b_baud = B9600; break;
        case 19200:
            b_baud = B19200; break;
        case 38400:
            b_baud = B38400; break;
        case 57600:
            b_baud = B57600; break;
        case 115200:
            b_baud = B115200; break;
        default:
            b_baud = B9600; break;
    }

    fd = open (name, O_RDWR | O_NOCTTY );
    if (fd < 0) { return (-1); } /* open failed */

    if (tcgetattr (fd, &t) != 0) { return (-3); }
/* do settings */
    t.c_cc[VMIN] = 1;  // minimum characters received, should probably be smaller than 32
    t.c_cc[VTIME] = 1;

/* &= ~ disables bits, |= enables bits */
    t.c_iflag &= ~(BRKINT | IUTF8 | IGNBRK | IGNPAR | PARMRK | ISTRIP | INPCK | INLCR | IGNCR | ICRNL | IUCLC | IXON | IXANY | IXOFF | IMAXBEL);
    t.c_oflag &= ~(OPOST);
/* non-canonical */
    t.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL | ICANON | ISIG | NOFLSH | TOSTOP);
    t.c_cflag &= ~(CSIZE | CSTOPB | HUPCL | PARENB);

    t.c_cflag |= CLOCAL | CREAD | CS8;
/* copy data rates into termios struct */
    if (cfsetispeed (&t, b_baud) == -1) { return (-3); }
    if (cfsetospeed (&t, b_baud) == -1) { return (-3); }
/* throw away any input data / noise */
    if (tcflush (fd, TCIFLUSH) == -1) { return (-3); }
/* Now, set the terminal attributes */
    if (tcsetattr (fd, TCSANOW, &t) == -1) { return (-3); }
    return fd;
}
// end serial_open

////////////////////////////////////////////////////////////////////////////
// Name:         serial_read
// Description:  Read serial port
// Parameters:   
// Return:       
////////////////////////////////////////////////////////////////////////////
gboolean 
serial_read(GIOChannel *gio, GIOCondition condition, gpointer data) // GdkInputCondition condition )
{
    static gsize n = 1;
    static char msg[10000] = ""; // application dependent - convert back to static when processing works
    static int  count = 0;
    gchar buf;
    GIOStatus readStatus;
    gboolean lfReturnValue = TRUE;
    GError *lgerror;
   
    //
    // Running in Linux Mint on my laptop, instead of Raspbian on a
    // Raspberry Pi or Libre AML-S906X-CC, if the device under test
    // shuts off, app seems to hang here. In that case, much better
    // to get rid of the while loop, even if it means calling serial_read()
    // once for each received character!
    // Probably want to do any parsing or display stuff outside
    // this routine, which is effectively an interrupt-service routine.
    // MAB 2023.01.18
    //
    //n = 1;
    //while (n > 0 && n < sizeof(msg))
    {
        readStatus = g_io_channel_read_chars(gio, &buf, 1, &n, NULL); // force to read 1 char at a time?
        switch (readStatus)
        {
        case G_IO_STATUS_NORMAL:
            break;
        case G_IO_STATUS_ERROR:
            isUSBConnectionOK = FALSE;
            g_print("\r\n *** G_IO_STATUS_ERROR ***\r\n");
            lfReturnValue = FALSE;
            break;
        case G_IO_STATUS_EOF:
            isUSBConnectionOK = FALSE;
            g_print("\r\n *** G_IO_STATUS_EOF *** \r\n");
            lfReturnValue = FALSE;
            break;
        case G_IO_STATUS_AGAIN:
            isUSBConnectionOK = FALSE;
            g_print("\r\n *** G_IO_STATUS_AGAIN *** \r\n");
            lfReturnValue = FALSE;
            break;
        default:
            g_print("\r\n readStatus = %d\r\n", readStatus);
            lfReturnValue = FALSE;
            break;
        }
        if (n > 0)
        {
            msg[count++] = buf;
        }
        if (buf == '\n')
        {
            if (count<2) count=2; // just in case \n received too early in msg
            msg[count-2] = '\0';  // overwrite \r\n
            //g_print("%s\r\n",msg);

            // Save received string to receive FIFO
            main_receive_msg_write(msg);

            //////////////////////////////////////////////////////////
            // At this point msg[] may contain a Display JSON string.
            // Try to detect these; if found, strip the preamble and
            // save the JSON to the appropriate JSON buffer.
            // char * lpCheckForJSON;
            // lpCheckForJSON = strstr(msg, "DISPLAY ZONE VALUES >>> ");
            // if (lpCheckForJSON != NULL)
            // {
            //     //g_print("DISPLAY ZONE VALUES detected\r\n");
            //     memset (strJSONZoneValues, 0, sizeof(strJSONZoneValues));
            //     strncpy(strJSONZoneValues, &msg[24], strlen(msg));
            //     ucJSONParseCountdown = DELAY_JSON_PARSING_SEC;
            // }
            // lpCheckForJSON = strstr(msg, "DISPLAY ALARM VALUES >>> ");
            // if (lpCheckForJSON != NULL)
            // {
            //     //g_print("DISPLAY ALARM VALUES detected\r\n");
            //     memset (strJSONAlarmValues, 0, sizeof(strJSONAlarmValues));
            //     strncpy(strJSONAlarmValues, &msg[25], strlen(msg));
            //     //g_print("%s\r\n",strJSONAlarmValues);
            //     ucJSONParseCountdown = DELAY_JSON_PARSING_SEC;
            // }
            // lpCheckForJSON = strstr(msg, "DISPLAY ZONE NAMES >>> ");
            // if (lpCheckForJSON != NULL)
            // {
            //     //g_print("DISPLAY ZONE NAMES detected\r\n");
            //     memset (strJSONZoneNames, 0, sizeof(strJSONZoneNames));
            //     strncpy(strJSONZoneNames, &msg[23], strlen(msg));
            //     //g_print("%s\r\n",strJSONZoneNames);
            //     ucJSONParseCountdown = DELAY_JSON_PARSING_SEC;
            // }
            // lpCheckForJSON = strstr(msg, "DISPLAY ZONE TYPES >>> ");
            // if (lpCheckForJSON != NULL)
            // {
            //     //g_print("DISPLAY ZONE TYPES detected\r\n");
            //     memset (strJSONZoneTypes, 0, sizeof(strJSONZoneTypes));
            //     strncpy(strJSONZoneTypes, &msg[23], strlen(msg));
            //     //g_print("%s\r\n",strJSONZoneTypes);
            //     ucJSONParseCountdown = DELAY_JSON_PARSING_SEC;
            // }
            // lpCheckForJSON = strstr(msg, "DISPLAY DIAGNOSTICS >>> ");
            // if (lpCheckForJSON != NULL)
            // {
            //     //g_print("DISPLAY DIAGNOSTICS detected\r\n");
            //     memset (strJSONDiagnostics, 0, sizeof(strJSONDiagnostics));
            //     strncpy(strJSONDiagnostics, &msg[24], strlen(msg));
            //     //g_print("%s\r\n",strJSONDiagnostics);
            //     ucJSONParseCountdown = DELAY_JSON_PARSING_SEC;
            // }
            
            // // Also check for the online connection error code
            // // Although it isn't JSON, the prefix is similar
            // lpCheckForJSON = strstr(msg, "DISPLAY ERROR CODE >>> ");
            // if (lpCheckForJSON != NULL)
            // {
            //     //g_print("DISPLAY ERROR CODE detected\r\n");
            //     memset (lcSerialTempString, 0, sizeof(lcSerialTempString));
            //     strncpy(lcSerialTempString, &msg[23], strlen(msg));
            //     ucConnectionErrorCode = atoi(lcSerialTempString);
            //     //g_print("ucConnectionErrorCode = %d\r\n",ucConnectionErrorCode);
            // }
            //////////////////////////////////////////////////////////
            
            count = 0;
            //msg[0] = '\0';  // finish up and restart
            memset(msg, 0, sizeof(msg));
            n = 0; // drop out after every complete message since ...read_chars seems to always block
        }
    } // while (n>0)
    return lfReturnValue;
}
// end serial_read


////////////////////////////////////////////////////////////////////////////
// Name:         serial_error
// Description:  Handle serial port errors
// Parameters:   
// Return:       
////////////////////////////////////////////////////////////////////////////
gboolean 
serial_error(GIOChannel *gio, GIOCondition condition, gpointer data)
{
    if (condition&G_IO_ERR)
    {
        g_print("\r\n ***** G_IO_ERR *****\r\n");
    }
    if (condition&G_IO_HUP)
    {
        g_print("\r\n ***** G_IO_HUP *****\r\n");
    }
    if (condition&G_IO_NVAL)
    {
        g_print("\r\n ***** G_IO_NVAL *****\r\n");
    }
    g_io_channel_shutdown(gio, FALSE, NULL);
    //isUSBConnectionOK = FALSE;
    return FALSE;
}
// end serial_error


