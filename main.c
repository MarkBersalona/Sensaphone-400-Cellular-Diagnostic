/* 
 * File:   main.c
 * Author: Mark Bersalona
 *
 * Created on 2023.01.16
 * Main file for Sensaphone 400 Cellular Diagnostic
 */

///////////////////////////////////////////////////////////////////////////////
//
// Includes
//
///////////////////////////////////////////////////////////////////////////////

#include <gtk/gtk.h>
//#include <json-glib/json-glib.h>
//#include <glib-object.h>
//#include <gobject/gvaluecollector.h>
#include <stdio.h>
#include <stdlib.h>
//#include <fcntl.h>          // File Control Definitions
//#include <termios.h>        // POSIX Terminal Control definitions
#include <string.h>
#include <time.h>
#include <ctype.h>
#include "main.h"
#include "gconfig.h"
#include "serial.h"
#include "display.h"


///////////////////////////////////////////////////////////////////////////////
//
// Definitions
//
///////////////////////////////////////////////////////////////////////////////

#define GLADE_LAYOUT ("400CellularDiagnostic.glade")






///////////////////////////////////////////////////////////////////////////////
//
// Variables and tables
//
///////////////////////////////////////////////////////////////////////////////

// GTK builder
GtkBuilder *builder;

char lcTempMainString[250];

// Receive message FIFO
char gucReceiveFIFO[RECEIVE_FIFO_MSG_COUNT][RECEIVE_FIFO_MSG_LENGTH_MAX];
guint16 guiReceiveFIFOWriteIndex;
guint16 guiReceiveFIFOReadIndex;

// UNIX timestamp
guint32 gulUNIXTimestamp;
// Elapsed time since last data update
guint32 gulElapsedTimeSinceDataUpdate_sec;
// Glib date/time
GDateTime *gDateTime;
// Countdown minutes to Status timestamp
#define TIMESTAMP_DELAY_TABLE_COUNT (13)
guint16 guiStatusTimestampCountdown_minutes = 0;
guint8  guiStatusTimestampCountdownIndex    = 0;
guint8  uiStatusTimestampCountdownIndex_OLD = 0;
guint16 uiStatusTimestampCountdownTable[TIMESTAMP_DELAY_TABLE_COUNT] = 
{
    5, 6, 6, 7, 8, 10, 13, 18, 28, 39, 60, 120, 240
};

// Logfile
GIOChannel *gioChannelLogfile;
char lcLogfileName[100];
gboolean lfIsLogfileEnabled;

// I/O channel for serial-to-USB port
GIOChannel *gIOChannelSerialUSB;

// Sticky error status
char gucStickyErrorStatus[200];
char gucStickyErrorStatusOLD[200];

// Error codes
char* pucErrorCodes[] =
{
    "Secure connection to Sensaphone established",
    "Online IDLE state",
    "Online REGISTERING state",
    "Online WAITING state",
    "Online NEGOTIATING state",

    "Online CONNECTED state",
    "Network registration failed",
    "Network negotiation failed",
    "Receive timeout",
    "Server closed the socket",

    "Transmit failure",
    "ERROR response received",
    "uBlox SARA-R5 failed to initialize",
    "uBlox SARA-R5 updating",
    "uBlox SARA-R5 update failed",

    "uBlox SARA-R5 unresponsive",
    "WebSocket PONG frame expected but timed out",
    "Too many Alarm/Update POST ACK timeouts",
    "Unspecified ERROR",
};

// Sticky Status
#define STICKY_ERROR_COUNT_PERIOD_SECONDS 600
static guint16 guiStickyErrorCountdown_sec = STICKY_ERROR_COUNT_PERIOD_SECONDS;


///////////////////////////////////////////////////////////////////////////////
//
// Utility functions
//
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// Name:         is_valid_mac
// Description:  Validate a given string is a MAC address
//               Expects format xx-xx-xx-xx-xx-xx
//                           or xx:xx:xx:xx:xx:xx
//               where xx is a hexadecimal number in range [00:FF]
//               case-insensitive
// Parameters:   paucTestMAC - pointer to string to be tested
// Return:       TRUE if valid MAC; FALSE otherwise
////////////////////////////////////////////////////////////////////////////
gboolean is_valid_mac(char* paucTestMAC)
{
    // Test if the length is correct
    if (17 != strlen(paucTestMAC))
    {
        return FALSE;
    }

    // Test if delimiters are '-' or ':'
    for (int i = 2; i < 17; i += 3)
    {
        if (paucTestMAC[i] != '-' && paucTestMAC[i] != ':')
        {
            // invalid delimiter
            return FALSE;
        }
    }

    // Test if digits are hex
    for (int i = 0; i < 17; ++i)
    {
        if ( !isxdigit(paucTestMAC[i]) )
        {
            // Ignore the indices of the delimiters
            if ( i!=2 && i!=5 && i!=8 && i!=11 && i!=14)
            {
                return FALSE;
            }
        }
    }

    // Delimiters are OK
    // Digits are hex
    // Must be a properly-formatted MAC address
    return TRUE;
}
// end is_valid_mac

////////////////////////////////////////////////////////////////////////////
// Name:         trim
// Description:  Trims the given string of leading and trailing spaces
//               Uses isspace() to determine what's a "space"
//               By Johannes Schaub, 2008.12.09
//               stackoverflow.com/questions/352055/
//                 best-algorithm-to-strip-leading-and-trailing-spaces-in-c
// Parameters:   Pointer to start of string to trim
// Return:       Pointer to null-terminated trimmed string
////////////////////////////////////////////////////////////////////////////
char* trim(char* paucInputString)
{
    char* e = paucInputString + strlen(paucInputString) - 1;
    while (*paucInputString && isspace(*paucInputString)) paucInputString++;
    while (e > paucInputString && isspace(*e)) *e-- = '\0';
    return paucInputString;
}
// end trim







///////////////////////////////////////////////////////////////////////////////
//
// Callbacks
//
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// Name:         main_ATCommand_clicked
// Description:  Callback routine - ATCommand button clicked
//               Send the user-entered AT command to the SARA-R5 on the 400 Cellular
// Parameters:   the contents of the AT Command text entry
// Return:       None
////////////////////////////////////////////////////////////////////////////
void main_ATCommand_clicked(void)
{
    char lcATCommand[100];
    guint16 luiATCommandLength;

    display_status_write("AT Command button pressed\r\n");

    luiATCommandLength = gtk_entry_get_text_length(GTK_ENTRY(txtentATCommand));
    //sprintf(lcTempMainString, "AT Command length = %d chars\r\n", luiATCommandLength);
    //display_status_write(lcTempMainString);
    if (luiATCommandLength > 0)
    {
        // Get the contents of the AT Command text entry
        memset(lcATCommand, 0, sizeof(lcATCommand));
        memcpy(lcATCommand, gtk_entry_get_text(GTK_ENTRY(txtentATCommand)), luiATCommandLength);

        // Send the formatted AT Command to the 400 Cellular
        //sprintf(lcTempMainString, "AT Command = >>%s<<\r\n", lcATCommand);
        //display_status_write(lcTempMainString);
        sprintf(lcTempMainString, "+++COMMAND:%s", lcATCommand);
        serial_write(lcTempMainString);
    }
    else
    {
        display_status_write("WARNING - AT Command entry appears blank\r\n");
    }
}
// end main_ATCommand_clicked


////////////////////////////////////////////////////////////////////////////
// Name:         main_BOARDREV_clicked
// Description:  Callback routine - BoardRev button clicked
//               Send the user-entered Board rev to the 400 Cellular
// Parameters:   the contents of the Board rev text entry
// Return:       None
////////////////////////////////////////////////////////////////////////////
void main_BOARDREV_clicked(void)
{
    char lcBoardRev[100];
    guint16 luiBoardRevLength;

    display_status_write("Board rev button pressed\r\n");

    luiBoardRevLength = gtk_entry_get_text_length(GTK_ENTRY(txtentNewBoardRev));
    //sprintf(lcTempMainString, "luiBoardRevLength length = %d chars\r\n", luiBoardRevLength);
    //display_status_write(lcTempMainString);
    if (luiBoardRevLength > 0)
    {
        // Get the contents of the Board rev text entry
        memset(lcBoardRev, 0, sizeof(lcBoardRev));
        memcpy(lcBoardRev, trim((char*)gtk_entry_get_text(GTK_ENTRY(txtentNewBoardRev))), 1);

        // Send the formatted Board rev Menu to the 400 Cellular
        if (isalpha(lcBoardRev[0]))
        {
            //sprintf(lcTempMainString, "New Board rev = >>%s<<\r\n", lcBoardRev);
            //display_status_write(lcTempMainString);
            sprintf(lcTempMainString, "+++MENU:B %s", lcBoardRev);
            serial_write(lcTempMainString);
        }
        else
        {
            sprintf(lcTempMainString, "WARNING - '%s' is an invalid Board rev\r\n", lcBoardRev);
            display_status_write(lcTempMainString);
        }
    }
    else
    {
        display_status_write("WARNING - Board rev appears blank\r\n");
    }
}
// end main_BOARDREV_clicked

////////////////////////////////////////////////////////////////////////////
// Name:         main_LOGENABLE_state_set
// Description:  Callback routine - logfile enable switch clicked
// Parameters:   None
// Return:       None
////////////////////////////////////////////////////////////////////////////
void main_LOGENABLE_state_set(void)
{
    lfIsLogfileEnabled = gtk_switch_get_active(GTK_SWITCH(swLogfileEnable));
    if (lfIsLogfileEnabled)
    {
        // Logfile has just been enabled, build timestamp filename and open file
        memset(lcLogfileName, 0, sizeof(lcLogfileName));
        sprintf(lcLogfileName, "%s 400 Cellular.txt", g_date_time_format(gDateTime, "%Y%m%d %H%M"));
        sprintf(lcTempMainString, "Logfile %s opened\r\n", lcLogfileName);
        display_status_write(lcTempMainString);
        gtk_label_set_text(GTK_LABEL(lblLogfile), lcLogfileName);
        // (Open file for append, which creates new file if file doesn't exist yet)
        gioChannelLogfile = g_io_channel_new_file(lcLogfileName, "a", NULL);
        //g_io_channel_set_encoding(gioChannelLogfile, NULL, NULL);

        // Write intro text to logfile
        memset(lcTempMainString, 0, sizeof(lcTempMainString));
        sprintf(lcTempMainString, "---------- Sensaphone 400 Cellular logfile, opened %s local time -----------", 
                                                          g_date_time_format(gDateTime, "%Y.%m.%d %H:%M") );
        main_logfile_write(lcTempMainString);

        // Set the switch state to ON
        gtk_switch_set_state(GTK_SWITCH(swLogfileEnable), TRUE);
    }
    else
    {
        // Logfile has just been disabled, close the logfile and blank the displayed log filename
        g_io_channel_shutdown(gioChannelLogfile, TRUE, NULL);
        sprintf(lcTempMainString, "Logfile %s is now closed\r\n", lcLogfileName);
        display_status_write(lcTempMainString);
        gtk_label_set_text(GTK_LABEL(lblLogfile), "------------------------------------------");

        // Set the switch state to OFF
        gtk_switch_set_state(GTK_SWITCH(swLogfileEnable), FALSE);
    }
}
// end main_LOGENABLE_state_set

////////////////////////////////////////////////////////////////////////////
// Name:         main_MAC_clicked
// Description:  Callback routine - MAC button clicked
//               Send the user-entered MAC to the 400 Cellular
// Parameters:   the contents of the MAC text entry
// Return:       None
////////////////////////////////////////////////////////////////////////////
void main_MAC_clicked(void)
{
    char lcMACAddress[100];
    guint16 luiMACAddressLength;

    display_status_write("MAC button pressed\r\n");

    luiMACAddressLength = gtk_entry_get_text_length(GTK_ENTRY(txtentNewMAC));
    //sprintf(lcTempMainString, "luiMACAddressLength length = %d chars\r\n", luiMACAddressLength);
    //display_status_write(lcTempMainString);
    if (luiMACAddressLength > 0)
    {
        // Get the contents of the MAC text entry
        memset(lcMACAddress, 0, sizeof(lcMACAddress));
        memcpy(lcMACAddress, trim((char*)gtk_entry_get_text(GTK_ENTRY(txtentNewMAC))), luiMACAddressLength);

        // Send the formatted MAC Menu to the 400 Cellular
        if (is_valid_mac(lcMACAddress))
        {
            //sprintf(lcTempMainString, "New MAC address = >>%s<<\r\n", lcMACAddress);
            //display_status_write(lcTempMainString);
            sprintf(lcTempMainString, "+++MENU:M %s", lcMACAddress);
            serial_write(lcTempMainString);
        }
        else
        {
            sprintf(lcTempMainString, "WARNING - '%s' is an invalid MAC address\r\n", lcMACAddress);
            display_status_write(lcTempMainString);
        }
    }
    else
    {
        display_status_write("WARNING - MAC address appears blank\r\n");
    }
}
// end main_MAC_clicked

////////////////////////////////////////////////////////////////////////////
// Name:         main_MENU_clicked
// Description:  Callback routine - MENU button clicked
// Parameters:   None
// Return:       None
////////////////////////////////////////////////////////////////////////////
void main_MENU_clicked(void)
{
    gint liMenuItemSelected = gtk_combo_box_get_active(GTK_COMBO_BOX(cbtMenu));
    if (-1 ==liMenuItemSelected)
    {
        // No menu item selected
        display_status_write("MENU button pressed, but no menu item selected\r\n");
    }
    else
    {
        // A menu item has been selected
        sprintf(lcTempMainString, "MENU button pressed, menu item %d (%s, %s) selected\r\n", 
                                                    liMenuItemSelected, 
                                                    pucMenuCMD[liMenuItemSelected], 
                                                    pucMenuItems[liMenuItemSelected]);
        display_status_write(lcTempMainString);

        // Send the formatted menu command to the 400 Cellular
        sprintf(lcTempMainString, "+++MENU:%s", pucMenuCMD[liMenuItemSelected]);
        serial_write(lcTempMainString);
    }
}
// end main_MENU_clicked

////////////////////////////////////////////////////////////////////////////
// Name:         main_REBOOT_clicked
// Description:  Callback routine - REBOOT button clicked
// Parameters:   None
// Return:       None
////////////////////////////////////////////////////////////////////////////
void main_REBOOT_clicked(void)
{
    //display_status_write("REBOOT button pressed\r\n");
    // Send the formatted menu command to the 400 Cellular
    sprintf(lcTempMainString, "+++MENU:Z");
    serial_write(lcTempMainString);

    // Reset display
    //display_clear_UUT_values();
}
// end main_REBOOT_clicked

////////////////////////////////////////////////////////////////////////////
// Name:         main_RTD_clicked
// Description:  Callback routine - RTD button clicked
// Parameters:   None
// Return:       None
////////////////////////////////////////////////////////////////////////////
void main_RTD_clicked(void)
{
    //display_status_write("RESET to Defaults button pressed\r\n");
    // Send the formatted menu command to the 400 Cellular
    sprintf(lcTempMainString, "+++MENU:X");
    serial_write(lcTempMainString);

    // Reset display
    //display_clear_UUT_values();
}
// end main_RTD_clicked







///////////////////////////////////////////////////////////////////////////////
//
// Main application
//
///////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
// Name:         main_parse_msg
// Description:  Parse a string for detectable data
// Parameters:   paucReceiveMsg - pointer to received NULL-terminated string
// Return:       None
////////////////////////////////////////////////////////////////////////////
void 
main_parse_msg(char *paucReceiveMsg)
{
    char *plcDetected;
    char *plcQuoteStart;
    char *plcQuoteEnd;
    char *plcPercentage;
    char *plcHumidity;
    char *plcDetectedParam;
    char *plcSpace;

    // For zone number, type and type description
    char lcZoneNumberBuf[4];
    uint8_t lucZoneNumber;
    char lcZoneTypeBuf[4];
    uint8_t lucZoneType;
    int liZoneValue;
    char lcZoneTypeName[20];

    // For zone alarm statuses
    char lcZoneAlarmBuf[4];
    uint8_t lucZoneAlarm;
    char lcZoneRangeBuf[4];
    uint8_t lucZoneRange;
    char lcZoneUnackBuf[4];
    uint8_t lucZoneUnack;
    char lcZoneAlarmColor[30];

    // Look for *** WARNING ***
    plcDetected = strstr((char*)paucReceiveMsg, "*** WARNING ***");
    if (plcDetected)
    {
        // Write the entire warning, including *** WARNING ***, to Status
        // (but only if there's a significant string following)
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        if (strlen(lcTempMainString) > 20)
        {
            display_status_write(lcTempMainString);
            display_status_write("\r\n");
        }
    }
    
    // Look for *** ERROR ***
    plcDetected = strstr((char*)paucReceiveMsg, "*** ERROR ***");
    if (plcDetected)
    {
        // Write the entire error, including *** ERROR ***, to Status
        // (but only if there's a significant string following)
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        if (strlen(lcTempMainString) > 20)
        {
            display_status_write(lcTempMainString);
            display_status_write("\r\n");

            // Save error message as a sticky one
            strcpy(gucStickyErrorStatus, lcTempMainString);
        }
    }
    
    // Look for "Sensaphone 400 starting..."
    plcDetected = strstr((char*)paucReceiveMsg, "Sensaphone 400 starting...");
    if (plcDetected)
    {
        // This is a new UUT or the old UUT restarting
        // Either way, reset the Diagnostic tool display
        display_clear_UUT_values();
    }
    
    // Look for "*** ERROR CODE *** geOnlineConnectionError = "
    plcDetected = strstr((char*)paucReceiveMsg, "*** ERROR CODE *** geOnlineConnectionError = ");
    if (plcDetected)
    {
        // Get the error code
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected+45, strlen(plcDetected+45));
        int liErrorCode = atoi(lcTempMainString);
        sprintf(lcTempMainString, "*** ERROR CODE *** %s", pucErrorCodes[liErrorCode]);
        display_status_write(lcTempMainString);
        display_status_write("\r\n");

        // Save error code as a sticky one
        strcpy(gucStickyErrorStatus, lcTempMainString);
    }
    
    // Look for "MAC address:"
    plcDetected = strstr((char*)paucReceiveMsg, "MAC address: ");
    if (plcDetected)
    {
        // Write the MAC address to Status and to the MAC label
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected+13, strlen(plcDetected+13));
        if ( strlen(plcDetected+13) > 12)
        {
            display_status_write("Detected device MAC address: ");
            display_status_write(lcTempMainString);
            display_status_write("\r\n");
            gtk_label_set_text(GTK_LABEL(lblMAC), lcTempMainString);
        }
    }
    
    // Look for "Model number is "
    plcDetected = strstr((char*)paucReceiveMsg, "Model number is ");
    if (plcDetected)
    {
        plcQuoteStart = strchr(plcDetected+16, 0x22); // search for double quote
        plcQuoteEnd   = strchr(plcDetected+21, 0x22); // search for double quote
        if (plcQuoteStart && plcQuoteEnd && (plcQuoteEnd-plcQuoteStart > 5) ) // both quotes found
        {
            // Write the transceiver model to Status and to the transceiver label
            memset (lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy (lcTempMainString, plcQuoteStart+1, plcQuoteEnd-plcQuoteStart-1);
            display_status_write("Detected transceiver model: ");
            display_status_write(lcTempMainString);
            display_status_write("\r\n");
            gtk_label_set_text(GTK_LABEL(lblTransceiver), lcTempMainString);
        }
    }
    
    // Look for "SARA-R5 FW version number is "
    plcDetected = strstr((char*)paucReceiveMsg, "SARA-R5 FW version number is ");
    if (plcDetected)
    {
        plcQuoteStart = strchr(plcDetected+29, 0x22); // search for double quote
        plcQuoteEnd   = strchr(plcDetected+32, 0x22); // search for double quote
        if (plcQuoteStart && plcQuoteEnd && (plcQuoteEnd-plcQuoteStart > 2) ) // both quotes found
        {
            // Write the transceiver firmware version to Status and to the transceiver FW label
            memset (lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy (lcTempMainString, plcQuoteStart+1, plcQuoteEnd-plcQuoteStart-1);
            display_status_write("Detected transceiver FW version: ");
            display_status_write(lcTempMainString);
            display_status_write("\r\n");
            gtk_label_set_text(GTK_LABEL(lblTransceiverFW), lcTempMainString);
        }
    }
        
    // Look for "Board revision = "
    plcDetected = strstr((char*)paucReceiveMsg, "Board revision = ");
    if (plcDetected)
    {
        // Write the board rev to Status and to the board rev label
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected+17, strlen(plcDetected+17));
        display_status_write("Detected Board revision: ");
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
        gtk_label_set_text(GTK_LABEL(lblBoardRev), lcTempMainString);
    }
    
    // Look for "IMEI is "
    plcDetected = strstr((char*)paucReceiveMsg, "IMEI is ");
    if (plcDetected)
    {
        plcQuoteStart = strchr(plcDetected+8,  0x22); // search for double quote
        plcQuoteEnd   = strchr(plcDetected+12, 0x22); // search for double quote
        if (plcQuoteStart && plcQuoteEnd && (plcQuoteEnd-plcQuoteStart > 5) ) // both quotes found
        {
            // Write the IMEI to Status and to the IMEI label
            memset (lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy (lcTempMainString, plcQuoteStart+1, plcQuoteEnd-plcQuoteStart-1);
            display_status_write("Detected IMEI: ");
            display_status_write(lcTempMainString);
            display_status_write("\r\n");
            gtk_label_set_text(GTK_LABEL(lblIMEI), lcTempMainString);
        }
    }
    
    // Look for "ICCID is "
    // (but only if there's a significant string following)
    plcDetected = strstr((char*)paucReceiveMsg, "ICCID is ");
    if (plcDetected)
    {
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected+9, strlen(plcDetected+9));
        if (strlen(lcTempMainString) > 20)
        {
            display_status_write("Detected ICCID: ");
            display_status_write(lcTempMainString);
            display_status_write("\r\n");
            gtk_label_set_text(GTK_LABEL(lblICCID), lcTempMainString);
        }
    }
    
    // Look for "400 Cellular firmware version is "
    plcDetected = strstr((char*)paucReceiveMsg, "400 Cellular firmware version is ");
    if (plcDetected)
    {
        // Write the 400 Cellular FW version to Status and to the 400 FW label
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected+33, strlen(plcDetected+33));
        display_status_write("Detected 400 Cellular firmware version: ");
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
        gtk_label_set_text(GTK_LABEL(lbl400FW), lcTempMainString);
    }
    
    // Look for "Network_Online_StateMachine: Transitioning from "
    plcDetected = strstr((char*)paucReceiveMsg, "Network_Online_StateMachine: Transitioning from ");
    if (plcDetected)
    {
        // Write the 400 Cellular's network online state transition to Status
        display_status_write(paucReceiveMsg);
        display_status_write("\r\n");

        // Get the new connection state
        plcDetectedParam = strstr((char*)paucReceiveMsg, " to ");
        if (plcDetectedParam)
        {
            memset (lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy (lcTempMainString, plcDetectedParam+4, strlen(trim(plcDetectedParam+4)));
            gtk_label_set_text(GTK_LABEL(lblConnection), lcTempMainString);
            if ( strstr((char*)lcTempMainString, "CONNECTED") )
            {
                gtk_widget_set_name((lblConnection),     "ConnectionOK");     // green

                // Clear sticky error status, prep for the next one
                memset(gucStickyErrorStatus, 0x00, sizeof(gucStickyErrorStatus));
                guiStickyErrorCountdown_sec = STICKY_ERROR_COUNT_PERIOD_SECONDS;
                gtk_label_set_text(GTK_LABEL(lblStatusTitle),  "Status");
            }
            else if ( strstr((char*)lcTempMainString, "ERROR") || 
                      strstr((char*)lcTempMainString, "Error")    )
            {
                gtk_widget_set_name((lblConnection),     "ConnectionError");     // red
            }
            else if ( strstr((char*)lcTempMainString, "UPDATING") )
            {
                gtk_widget_set_name((lblConnection),     "ConnectionWarning");   // yellow
            }
            else
            {
                gtk_widget_set_name((lblConnection),     "DiagnosticValue");  // white
            }
        }
    }
    
    // Look for "+++ Start DIAGNOSTIC MODE +++"
    plcDetected = strstr((char*)paucReceiveMsg, "+++ Start DIAGNOSTIC MODE +++");
    if (plcDetected)
    {
        // Diagnostic mode is enabled on the 400 Cellular
        // Send a sacrificial dummy string to "initialize" serial_write()
        // and 400 Cellular Diagnostic receive
        serial_write("+++");
    }
    
    // Look for "Periodic battery status: Main power "
    plcDetected = strstr((char*)paucReceiveMsg, "Periodic battery status: Main power ");
    if (plcDetected)
    {
        // Write the 400 Cellular Power status to the Power Value label
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected+36, strlen(trim(plcDetected+36)));
        //display_status_write("Detected 400 Cellular power status: ");
        //display_status_write(lcTempMainString);
        //display_status_write("\r\n");
        gtk_label_set_text(GTK_LABEL(lblValuePower), lcTempMainString);
    }
    
    // Look for "Periodic battery status: Time: " then "Percentage = "
    plcDetected = strstr((char*)paucReceiveMsg, "Periodic battery status: Time: ");
    if (plcDetected)
    {
        plcPercentage = strstr((char*)plcDetected, "Percentage = ");
        if (plcPercentage)
        {
            // Write the 400 Cellular Battery percentage to the Battery Value label
            memset (lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy (lcTempMainString, plcPercentage+13, strlen(trim(plcPercentage+13)));
            gtk_label_set_text(GTK_LABEL(lblValueBattery), lcTempMainString);
        }
    }
    
    // Look for "Lithium average A/D count" then "Percentage = "
    plcDetected = strstr((char*)paucReceiveMsg, "Lithium average A/D count");
    if (plcDetected)
    {
        plcPercentage = strstr((char*)plcDetected, "Percentage = ");
        if (plcPercentage)
        {
            // Write the 400 Cellular Lithium percentage to the Lithium Value label
            memset (lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy (lcTempMainString, plcPercentage+13, strlen(trim(plcPercentage+13)));
            gtk_label_set_text(GTK_LABEL(lblValueLithium), lcTempMainString);
        }
    }
    
    // Look for the periodic input zone readouts
    plcDetected = strstr((char*)paucReceiveMsg, "InputTask: Zone ");
    if (plcDetected)
    {
        // Get the zone number
        memset(lcZoneNumberBuf, 0, sizeof(lcZoneNumberBuf));
        memcpy(lcZoneNumberBuf, plcDetected+16, 2);
        lucZoneNumber = atoi(trim(lcZoneNumberBuf));

        // Get the type number; look up the type name and save it
        char *plcTypeDetected = strstr((char*)paucReceiveMsg, "type=");
        if (plcTypeDetected)
        {
            memset(lcZoneTypeBuf, 0, sizeof(lcZoneTypeBuf));
            memcpy(lcZoneTypeBuf, plcTypeDetected+5, 3);
            lucZoneType = atoi(trim(lcZoneTypeBuf));
            memset(lcZoneTypeName, 0, sizeof(lcZoneTypeName));
            switch (lucZoneType)
            {
                case  0: sprintf(lcZoneTypeName, "Normally Open");                   break;
                case  1: sprintf(lcZoneTypeName, "Normally Closed");                 break;
                case  2: sprintf(lcZoneTypeName, "Temp 2.8K F");                     break;
                case  3: sprintf(lcZoneTypeName, "Temp 2.8K C");                     break;
                case 29: sprintf(lcZoneTypeName, "Runtime NO");                      break;
                case 30: sprintf(lcZoneTypeName, "Runtime NC");                      break;
                default: sprintf(lcZoneTypeName, "Unexpected type=%d", lucZoneType); break;
            }
        }

        // Get the value
        char *plcValueDetected = strstr((char*)paucReceiveMsg, "Value=");
        if (plcValueDetected)
        {
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcValueDetected+6, strlen(plcValueDetected+6));
        }

        // Write the zone type name and zone value to the appropriate zone labels
        switch (lucZoneNumber)
        {
            case 1:
                gtk_label_set_text(GTK_LABEL(lblTypeZone1),  trim(lcZoneTypeName));
                gtk_label_set_text(GTK_LABEL(lblValueZone1), trim(lcTempMainString));
                break;
            case 2:
                gtk_label_set_text(GTK_LABEL(lblTypeZone2),  trim(lcZoneTypeName));
                gtk_label_set_text(GTK_LABEL(lblValueZone2), trim(lcTempMainString));
                break;
            case 3:
                gtk_label_set_text(GTK_LABEL(lblTypeZone3),  trim(lcZoneTypeName));
                gtk_label_set_text(GTK_LABEL(lblValueZone3), trim(lcTempMainString));
                break;
            case 4:
                gtk_label_set_text(GTK_LABEL(lblTypeZone4),  trim(lcZoneTypeName));
                gtk_label_set_text(GTK_LABEL(lblValueZone4), trim(lcTempMainString));
                break;
            case 5:
                gtk_label_set_text(GTK_LABEL(lblTypeZone5),  trim(lcZoneTypeName));
                gtk_label_set_text(GTK_LABEL(lblValueZone5), trim(lcTempMainString));
                break;
            default:
                sprintf(lcTempMainString, "ERROR - bad zone number %d\r\n", lucZoneNumber);
                display_status_write(lcTempMainString);
                break;
        }
    }

    // Look for "Relay has been turned ON"
    plcDetected = strstr((char*)paucReceiveMsg, "Relay has been turned ON");
    if (plcDetected)
    {
        // Write "CLOSED" to Relay value label
        gtk_label_set_text(GTK_LABEL(lblValueRelay), "CLOSED");
    }
    
    // Look for "Relay has been turned OFF"
    plcDetected = strstr((char*)paucReceiveMsg, "Relay has been turned OFF");
    if (plcDetected)
    {
        // Write "OPEN" to Relay value label
        gtk_label_set_text(GTK_LABEL(lblValueRelay), "OPEN");
    }
    
    // Look for "UGRMC string is INVALID" - meaning failed to get lat/long from satellites
    plcDetected = strstr((char*)paucReceiveMsg, "UGRMC string is INVALID");
    if (plcDetected)
    {
        // Write "-----" to lat/long labels
        gtk_label_set_text(GTK_LABEL(lblLatitude),  "-----");
        gtk_label_set_text(GTK_LABEL(lblLongitude), "-----");
    }
    
    // Look for "UGRMC string is VALID" - we got lat/long from satellites
    plcDetected = strstr((char*)paucReceiveMsg, "UGRMC string is VALID");
    if (plcDetected)
    {
        char *plcLatitudeDetected  = strstr((char*)paucReceiveMsg, "Latitude ");
        char *plcLongitudeDetected = strstr((char*)paucReceiveMsg, "Longitude ");

        if (plcLatitudeDetected)
        {
            // Get the latitude, write it to the latitude label
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcLatitudeDetected+9, plcLongitudeDetected-(plcLatitudeDetected+9)-1);
            gtk_label_set_text(GTK_LABEL(lblLatitude), trim(lcTempMainString));
        }

        if (plcLongitudeDetected)
        {
            // Get the longitude, write it to the longitude label
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcLongitudeDetected+10, strlen(plcLongitudeDetected+10));
            gtk_label_set_text(GTK_LABEL(lblLongitude), trim(lcTempMainString));
        }
    }
    
    // Look for "UGGSV satellite counts: "
    plcDetected = strstr((char*)paucReceiveMsg, "UGGSV satellite counts: ");
    if (plcDetected)
    {
        //char *plcNONEDetected  = strstr((char*)paucReceiveMsg, "NONE");
        char *plcGPSDetected = strstr((char*)paucReceiveMsg, "GPS=");
        char *plcGLONASSDetected = strstr((char*)paucReceiveMsg, "GLONASS=");
        char *plcGalileoDetected = strstr((char*)paucReceiveMsg, "Galileo=");
        char *plcBeiDouDetected = strstr((char*)paucReceiveMsg, "BeiDou=");
        char *plcQZSSDetected = strstr((char*)paucReceiveMsg, "QZSS=");

        // Zeroize all the satellite counts
        //if (plcNONEDetected)
        {
            gtk_label_set_text(GTK_LABEL(lblGPS), "0");
            gtk_label_set_text(GTK_LABEL(lblGLONASS), "0");
            gtk_label_set_text(GTK_LABEL(lblGalileo), "0");
            gtk_label_set_text(GTK_LABEL(lblBeiDou), "0");
            gtk_label_set_text(GTK_LABEL(lblQZSS), "0");
        }

        // If GPS detected, write count to its label
        if (plcGPSDetected)
        {
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcGPSDetected+4, 4);
            gtk_label_set_text(GTK_LABEL(lblGPS), trim(lcTempMainString));
        }

        // If GLONASS detected, write count to its label
        if (plcGLONASSDetected)
        {
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcGLONASSDetected+8, 4);
            gtk_label_set_text(GTK_LABEL(lblGLONASS), trim(lcTempMainString));
        }

        // If Galileo detected, write count to its label
        if (plcGalileoDetected)
        {
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcGalileoDetected+8, 4);
            gtk_label_set_text(GTK_LABEL(lblGalileo), trim(lcTempMainString));
        }

        // If BeiDou detected, write count to its label
        if (plcBeiDouDetected)
        {
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcBeiDouDetected+7, 4);
            gtk_label_set_text(GTK_LABEL(lblBeiDou), trim(lcTempMainString));
        }

        // If QZSS detected, write count to its label
        if (plcQZSSDetected)
        {
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcQZSSDetected+5, 4);
            gtk_label_set_text(GTK_LABEL(lblQZSS), trim(lcTempMainString));
        }
    }
    
    // Look for "Alarm change:", then "Zone=", "Alarm=", "Range=" and "Unack="
    plcDetected = strstr((char*)paucReceiveMsg, "Alarm change:");
    if (plcDetected)
    {
        // Display the alarm change in Status
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, strlen(trim(plcDetected)));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");

        // Get the zone number
        plcDetectedParam = strstr((char*)paucReceiveMsg, "Zone=");
        if (plcDetectedParam)
        {
            memset (lcZoneNumberBuf, 0, sizeof(lcZoneNumberBuf));
            memcpy (lcZoneNumberBuf, plcDetectedParam+5, 3);
            lucZoneNumber = (char)atoi(trim(lcZoneNumberBuf));
        }

        // Get the zone alarm
        plcDetectedParam = strstr((char*)paucReceiveMsg, "Alarm=");
        if (plcDetectedParam)
        {
            memset (lcZoneAlarmBuf, 0, sizeof(lcZoneAlarmBuf));
            memcpy (lcZoneAlarmBuf, plcDetectedParam+6, 3);
            lucZoneAlarm = (char)atoi(trim(lcZoneAlarmBuf));
        }

        // Get the zone range
        plcDetectedParam = strstr((char*)paucReceiveMsg, "Range=");
        if (plcDetectedParam)
        {
            memset (lcZoneRangeBuf, 0, sizeof(lcZoneRangeBuf));
            memcpy (lcZoneRangeBuf, plcDetectedParam+6, 3);
            lucZoneRange = (char)atoi(trim(lcZoneRangeBuf));
        }

        // Get the zone unack
        plcDetectedParam = strstr((char*)paucReceiveMsg, "Unack=");
        if (plcDetectedParam)
        {
            memset (lcZoneUnackBuf, 0, sizeof(lcZoneUnackBuf));
            memcpy (lcZoneUnackBuf, plcDetectedParam+6, 3);
            lucZoneUnack = (char)atoi(trim(lcZoneUnackBuf));
        }

        // Prepare the alarm status to display
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        if (VAL_ALARM_NONE==lucZoneAlarm && VAL_RANGE_OK==lucZoneRange && 0==lucZoneUnack)
        {
            sprintf(lcTempMainString, "OK");
            sprintf(lcZoneAlarmColor, "ZoneStatusOK");
        }
        else if (0==lucZoneUnack)
        {
            sprintf(lcZoneAlarmColor, "ZoneStatusAlarm");
            if (MSK_ALARM_LOW==lucZoneAlarm)
                sprintf(lcTempMainString, "LO");
            else if (MSK_ALARM_HIGH==lucZoneAlarm)
                sprintf(lcTempMainString, "HI");
            else if (MSK_ALARM_RESPONSE==lucZoneAlarm)
                sprintf(lcTempMainString, "NR");
            else
                sprintf(lcTempMainString, "??");
        }
        else // (0!=lucZoneUnack)
        {
            sprintf(lcZoneAlarmColor, "ZoneStatusAlarm");
            if (MSK_ALARM_LOW==lucZoneAlarm)
                sprintf(lcTempMainString, "U-LO");
            else if (MSK_ALARM_HIGH==lucZoneAlarm)
                sprintf(lcTempMainString, "U-HI");
            else if (MSK_ALARM_RESPONSE==lucZoneAlarm)
                sprintf(lcTempMainString, "U-NR");
            else
                sprintf(lcTempMainString, "U-??");
        }

        // Select the alarm label to update
        switch (lucZoneNumber)
        {
            case 0: 
                gtk_label_set_text(GTK_LABEL(lblAlarmPower), trim(lcTempMainString)); 
                gtk_widget_set_name((lblAlarmPower), lcZoneAlarmColor); 
                break;
            case 1: 
                gtk_label_set_text(GTK_LABEL(lblAlarmBattery), trim(lcTempMainString)); 
                gtk_widget_set_name((lblAlarmBattery), lcZoneAlarmColor); 
                break;
            case 2: 
                gtk_label_set_text(GTK_LABEL(lblAlarmLithium), trim(lcTempMainString)); 
                gtk_widget_set_name((lblAlarmLithium), lcZoneAlarmColor); 
                break;
            // case 3: 
            //     gtk_label_set_text(GTK_LABEL(lblAlarmIntTemp), trim(lcTempMainString)); 
            //     gtk_widget_set_name((lblAlarmIntTemp), lcZoneAlarmColor); 
            //     break;
            // case 4: 
            //     gtk_label_set_text(GTK_LABEL(lblAlarmHumidity), trim(lcTempMainString)); 
            //     gtk_widget_set_name((lblAlarmHumidity), lcZoneAlarmColor); 
            //     break;
            case 3: 
                gtk_label_set_text(GTK_LABEL(lblAlarmZone1), trim(lcTempMainString)); 
                gtk_widget_set_name((lblAlarmZone1), lcZoneAlarmColor); 
                break;
            case 4: 
                gtk_label_set_text(GTK_LABEL(lblAlarmZone2), trim(lcTempMainString)); 
                gtk_widget_set_name((lblAlarmZone2), lcZoneAlarmColor); 
                break;
            case 5: 
                gtk_label_set_text(GTK_LABEL(lblAlarmZone3), trim(lcTempMainString)); 
                gtk_widget_set_name((lblAlarmZone3), lcZoneAlarmColor); 
                break;
            case 6: 
                gtk_label_set_text(GTK_LABEL(lblAlarmZone4), trim(lcTempMainString)); 
                gtk_widget_set_name((lblAlarmZone4), lcZoneAlarmColor); 
                break;
            case 7: 
                gtk_label_set_text(GTK_LABEL(lblAlarmZone5), trim(lcTempMainString)); 
                gtk_widget_set_name((lblAlarmZone5), lcZoneAlarmColor); 
                break;
        }
    }

    // Look for "RTC is " followed by "UTC"
    plcDetected = strstr((char*)paucReceiveMsg, "RTC is ");
    if (plcDetected)
    {
        // Look for "UTC"
        plcDetectedParam = strstr((char*)paucReceiveMsg, "UTC");

        if (plcDetectedParam)
        {
            // Get date and time, excluding the seconds and UTC
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcDetected+7, (plcDetectedParam-4)-(plcDetected+7));

            // Write device RTC to RTC label
            gtk_label_set_text(GTK_LABEL(lblRTC),  lcTempMainString);
        }
    }
    
    // Look for "Cellular signal quality: " followed by "RSSI=", "RSRQ=", "RSRP=" and "QUALITY="
    if ( NULL!=strstr((char*)paucReceiveMsg, "Cellular signal quality: ") &&
         NULL!=strstr((char*)paucReceiveMsg, "RSSI=")                     &&
         NULL!=strstr((char*)paucReceiveMsg, "RSRQ=")                     &&
         NULL!=strstr((char*)paucReceiveMsg, "RSRP=")                     &&
         NULL!=strstr((char*)paucReceiveMsg, "QUALITY=")                     )
    {
        // Look for "RSSI=" and get RSSI, display it to RSSI label
        plcDetectedParam = strstr((char*)paucReceiveMsg, "RSSI=");
        plcSpace = strchr(plcDetectedParam+1, 0x20); // search for 1st space character
        if (plcDetectedParam && plcSpace)
        {
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcDetectedParam+5, plcSpace-(plcDetectedParam+5));
            gtk_label_set_text(GTK_LABEL(lblRSSI),  lcTempMainString);
        }
        
        // Look for "RSRQ=" and get RSRQ, display it to RSRQ label
        plcDetectedParam = strstr((char*)paucReceiveMsg, "RSRQ=");
        plcSpace = strchr(plcDetectedParam+1, 0x20); // search for 1st space character
        if (plcDetectedParam && plcSpace)
        {
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcDetectedParam+5, plcSpace-(plcDetectedParam+5));
            gtk_label_set_text(GTK_LABEL(lblRSRQ),  lcTempMainString);
        }
        
        // Look for "RSRP=" and get RSRP, display it to RSRP label
        plcDetectedParam = strstr((char*)paucReceiveMsg, "RSRP=");
        plcSpace = strchr(plcDetectedParam+1, 0x20); // search for 1st space character
        if (plcDetectedParam && plcSpace)
        {
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcDetectedParam+5, plcSpace-(plcDetectedParam+5));
            gtk_label_set_text(GTK_LABEL(lblRSRP),  lcTempMainString);
        }
        
        // Look for "QUALITY=" and get QUALITY, display it to QUALITY label
        plcDetectedParam = strstr((char*)paucReceiveMsg, "QUALITY=");
        if (plcDetectedParam)
        {
            memset(lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy(lcTempMainString, plcDetectedParam+8, strlen(plcDetectedParam+8));
            gtk_label_set_text(GTK_LABEL(lblSignalQuality),  trim(lcTempMainString));
            if ( strstr((char*)lcTempMainString, "EXCELLENT") ||
                 strstr((char*)lcTempMainString, "GOOD")        )
            {
                gtk_widget_set_name((lblSignalQuality),  "ConnectionOK");       // green
            }
            else if ( strstr((char*)lcTempMainString, "FAIR") )
            {
                gtk_widget_set_name((lblSignalQuality),  "ConnectionWarning");  // yellow
            }
            else if ( strstr((char*)lcTempMainString, "POOR") )
            {
                gtk_widget_set_name((lblSignalQuality),  "ConnectionError");    // red
            }
            else
            {
                gtk_widget_set_name((lblSignalQuality),  "DiagnosticValue");    // white
            }
        }
    }

    // Look for Standby State
    plcDetected = strstr((char*)paucReceiveMsg, "Standby State:");
    if (plcDetected)
    {
        // Write the entire Standby state to Status
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");

        // Save error message as a sticky one
        strcpy(gucStickyErrorStatus, lcTempMainString);
    }
    
    // Look for "Zone Update Pending in "
    plcDetected = strstr((char*)paucReceiveMsg, "Zone Update Pending in ");
    if (plcDetected)
    {
        // Write the entire "Zone Update Pending in ..." to Status
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
    // Look for "Keep-alive WebSocket P-i-n-g pending in "
    plcDetected = strstr((char*)paucReceiveMsg, "Keep-alive WebSocket P-i-n-g pending in ");
    if (plcDetected)
    {
        // Write the entire "Keep-alive WebSocket P-i-n-g pending in " to Status
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
    // Look for "Server has ACKed the "
    plcDetected = strstr((char*)paucReceiveMsg, "Server has ACKed the ");
    if (plcDetected)
    {
        // Write the entire "Server has ACKed the " to Status
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
    // Look for "PING frame received"
    plcDetected = strstr((char*)paucReceiveMsg, "PING frame received");
    if (plcDetected)
    {
        // Write the entire "PING frame received" to Status
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
    // Look for "Firmware Download progress:"
    plcDetected = strstr((char*)paucReceiveMsg, "Firmware Download progress:");
    if (plcDetected)
    {
        // Write the entire "Firmware Download progress:..." to Status
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
    // Look for "Firmware Download elapsed time:"
    plcDetected = strstr((char*)paucReceiveMsg, "Firmware Download elapsed time:");
    if (plcDetected)
    {
        // Write the entire "Firmware Download elapsed time:..." to Status
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
    // Look for "CRC of processor flash Pages 16-253 = "
    plcDetected = strstr((char*)paucReceiveMsg, "CRC of processor flash Pages 16-253 = ");
    if (plcDetected)
    {
        // Write the entire "CRC of processor flash Pages 16-253 = ..." to Status
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
    // Look for "CRC after external flash Blocks 8-126 = "
    plcDetected = strstr((char*)paucReceiveMsg, "CRC after external flash Blocks 8-126 = ");
    if (plcDetected)
    {
        // Write the entire "CRC after external flash Blocks 8-126 = ..." to Status
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
    // Look for "Network_Detect_FTP_Command_Response: "
    plcDetected = strstr((char*)paucReceiveMsg, "Network_Detect_FTP_Command_Response: ");
    if (plcDetected)
    {
        // Get the FTP command response and result
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected+37, strlen(plcDetected+37));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
    // Look for "Network_Detect_FTP_Error: "
    plcDetected = strstr((char*)paucReceiveMsg, "Network_Detect_FTP_Error: ");
    if (plcDetected)
    {
        // Get the FTP command response and result
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected+26, strlen(plcDetected+26));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
    // Look for "Network_Detect_FW_Install: "
    plcDetected = strstr((char*)paucReceiveMsg, "Network_Detect_FW_Install: ");
    if (plcDetected)
    {
        // Get the FW install status/result
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected+27, strlen(plcDetected+27));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
}
// end main_parse_msg

////////////////////////////////////////////////////////////////////////////
// Name:         main_receive_msg_read
// Description:  Read a received string from the receive FIFO
// Parameters:   None
// Return:       Pointer to received string, or NULL if no more strings
//               are available from the FIFO
////////////////////////////////////////////////////////////////////////////
char * 
main_receive_msg_read(void)
{
    char *plcReturnPointer;

    if (guiReceiveFIFOReadIndex == guiReceiveFIFOWriteIndex)
    {
        // Read and write FIFO pointers are the same, no more receive strings available
        plcReturnPointer = NULL;
    }
    else
    {
        // Return the next available received string off the FIFO,
        // then point to the next received string
        plcReturnPointer = &gucReceiveFIFO[guiReceiveFIFOReadIndex][0];
        if (++guiReceiveFIFOReadIndex >= RECEIVE_FIFO_MSG_COUNT) guiReceiveFIFOReadIndex = 0;
    }
    return plcReturnPointer;
}
// end main_receive_msg_read


////////////////////////////////////////////////////////////////////////////
// Name:         main_logfile_write
// Description:  Write NULL-terminated string to logfile
//               Assumes logfile already exists
// Parameters:   paucMessage - pointer to NULL-terminated string
// Return:       Size of message written
////////////////////////////////////////////////////////////////////////////
int main_logfile_write(char * paucMessage)
{
    gssize lsizeByteWritten = 0;

    if (lfIsLogfileEnabled)
    {
        // Write message to logfile
        g_io_channel_write_chars(gioChannelLogfile, paucMessage, -1, &lsizeByteWritten, NULL);
        g_io_channel_write_chars(gioChannelLogfile, "\r\n",      -1, NULL, NULL);
        // Send it out NOW!!
        //g_io_channel_flush(gioChannelLogfile, NULL);
    }

    return (int)lsizeByteWritten;
}
// end main_logfile_write

////////////////////////////////////////////////////////////////////////////
// Name:         main_receive_msg_write
// Description:  Write a received string to the FIFO
// Parameters:   paucReceiveMsg - pointer to received NULL-terminated string
// Return:       None
////////////////////////////////////////////////////////////////////////////
void 
main_receive_msg_write(char *paucReceiveMsg)
{
    static char lcFIFOWarning[200];

    // Zeroize the FIFO entry about to get the received message string
    memset(&gucReceiveFIFO[guiReceiveFIFOWriteIndex][0], 0, RECEIVE_FIFO_MSG_LENGTH_MAX);

    // Copy the received message string into the FIFO entry,
    // then point to the next FIFO entry to receive the next received message string
    memcpy(&gucReceiveFIFO[guiReceiveFIFOWriteIndex][0], paucReceiveMsg, strlen(paucReceiveMsg));
    if (++guiReceiveFIFOWriteIndex >= RECEIVE_FIFO_MSG_COUNT) guiReceiveFIFOWriteIndex = 0;

    // Check if the FIFO is almost full
    int liFIFOCount = guiReceiveFIFOWriteIndex - guiReceiveFIFOReadIndex;
    if (liFIFOCount < 0) liFIFOCount += RECEIVE_FIFO_MSG_COUNT;
    if (liFIFOCount > RECEIVE_FIFO_MSG_COUNT - 25)
    {
        sprintf(lcFIFOWarning, "\r\nWARNING - receive FIFO is almost full\r\n");
        display_status_write(lcFIFOWarning);
        //main_logfile_write(lcFIFOWarning);
    }
    else if (liFIFOCount == RECEIVE_FIFO_MSG_COUNT/2)
    {
        sprintf(lcFIFOWarning, "\r\nWARNING - receive FIFO is half-full\r\n");
        display_status_write(lcFIFOWarning);
        //main_logfile_write(lcFIFOWarning);
    }
}
// end main_receive_msg_write


////////////////////////////////////////////////////////////////////////////
// Name:         main_periodic
// Description:  Main periodic code
// Parameters:   None
// Return:       TRUE
////////////////////////////////////////////////////////////////////////////
static gboolean
main_periodic(gpointer data)
{
    static guint32 lulElapsed_sec = 0;
    char* plcReceivedMsgAvailable;
    static int fd;
    static gint liRTCSecond;
    static gint liRTCMinute;
    static gint liRTCHour;
    static gint liOLDRTCSecond = 99;
    static gint liOLDRTCMinute = 99;
    static gint liOLDRTCHour   = 99;
    
    //////////////////////////////////////////////////////////
    //
    // Update timestamps
    //
    //////////////////////////////////////////////////////////
    if (gulUNIXTimestamp != g_get_real_time()/1000000)
    {
        //
        // Updates every second
        //
        gulUNIXTimestamp = g_get_real_time()/1000000;
        //gDateTime = g_date_time_new_now_utc();
        gDateTime = g_date_time_new_now_local();
        ++lulElapsed_sec;
        ++gulElapsedTimeSinceDataUpdate_sec;
        liRTCSecond = g_date_time_get_second(gDateTime);
        liRTCMinute = g_date_time_get_minute(gDateTime);
        liRTCHour   = g_date_time_get_hour(gDateTime);

        // Force Status window to bottom
        adjStatus = gtk_scrolled_window_get_vadjustment(scrolledwindowStatus);
        gtk_adjustment_set_value( adjStatus, gtk_adjustment_get_upper(adjStatus) );

        // Display sticky error status (if any) to Status for N minutes
        // (but first check if stick error status has changed)
        if (strcmp(gucStickyErrorStatus, gucStickyErrorStatusOLD))
        {
            // Want to display new sticky status immediately,
            // so reset sticky error count and make the new
            // sticky status the current one
            guiStickyErrorCountdown_sec = STICKY_ERROR_COUNT_PERIOD_SECONDS;
            memset(gucStickyErrorStatusOLD, 0x00, sizeof(gucStickyErrorStatusOLD));
            strcpy(gucStickyErrorStatusOLD, gucStickyErrorStatus);
        }
        if (strlen(gucStickyErrorStatus) > 0)
        {
            if (guiStickyErrorCountdown_sec > 0)
            {
                // Display sticky error status
                --guiStickyErrorCountdown_sec;
                if (lulElapsed_sec%60 == 0)
                {
                    display_status_write(gucStickyErrorStatus);
                    display_status_write("\r\n");
                }
                sprintf(lcTempMainString, "Status: %s", gucStickyErrorStatus);
                gtk_label_set_text(GTK_LABEL(lblStatusTitle),  lcTempMainString);
            }
            else
            {
                // Clear sticky error status, prep for the next one
                memset(gucStickyErrorStatus, 0x00, sizeof(gucStickyErrorStatus));
                guiStickyErrorCountdown_sec = STICKY_ERROR_COUNT_PERIOD_SECONDS;
                gtk_label_set_text(GTK_LABEL(lblStatusTitle),  "Status");
            }
        }
        else
        {
            guiStickyErrorCountdown_sec = STICKY_ERROR_COUNT_PERIOD_SECONDS;
        }

        if (lulElapsed_sec%60 == 0)
        {
            //
            // Updates every ELAPSED minute
            //
        }

        if (lulElapsed_sec%(5*60) == 0)
        {
            //
            // Updates every ELAPSED 5 minutes
            //
            // Periodically clear the Receive text buffer
            gtk_text_buffer_get_start_iter(textbufReceive, &textiterReceiveStart);
            gtk_text_buffer_get_end_iter  (textbufReceive, &textiterReceiveEnd);
            gtk_text_buffer_delete(textbufReceive, &textiterReceiveStart, &textiterReceiveEnd);
        }


        if (lulElapsed_sec%(60*60) == 0)
        {
            //
            // Updates every ELAPSED hour
            //
        }

        if (lulElapsed_sec%(60*60*24) == 0)
        {
            //
            // Updates every ELAPSED 24-hour day
            //
            // Periodically clear the Status text buffer
            gtk_text_buffer_get_start_iter(textbufStatus, &textiterStatusStart);
            gtk_text_buffer_get_end_iter  (textbufStatus, &textiterStatusEnd);
            gtk_text_buffer_delete(textbufStatus, &textiterStatusStart, &textiterStatusEnd);
        }

        //
        // Updates every minute
        //
        if (liOLDRTCMinute != liRTCMinute)
        {
            liOLDRTCMinute = liRTCMinute;

            if (--guiStatusTimestampCountdown_minutes)
            {
                // Do nothing, not yet time to display Status timestamp
            }
            else
            {
                // Save old table index
                uiStatusTimestampCountdownIndex_OLD = guiStatusTimestampCountdownIndex;

                // Print timestamp in Status
                sprintf(lcTempMainString, "%s: UNIX timestamp %d\t", __FUNCTION__, gulUNIXTimestamp);
                display_status_write(lcTempMainString);
                sprintf(lcTempMainString, "Local time %s\r\n", g_date_time_format(gDateTime, "%Y-%m-%d %H:%M:%S"));
                display_status_write(lcTempMainString);

                // Display data age
                display_update_data_age();

                // Restore table index; update if needed
                guiStatusTimestampCountdownIndex = uiStatusTimestampCountdownIndex_OLD;
                if (guiStatusTimestampCountdownIndex<(TIMESTAMP_DELAY_TABLE_COUNT-1)) ++guiStatusTimestampCountdownIndex;

                // Reset minute countdown for Status timestamp
                guiStatusTimestampCountdown_minutes = uiStatusTimestampCountdownTable[guiStatusTimestampCountdownIndex];
            }
        }

    }
    
    //////////////////////////////////////////////////////////
    //
    // USB reconnect
    //
    //////////////////////////////////////////////////////////
    if (isUSBConnectionOK)
    {
        // Code when USB connection OK
    }
    else // USB connection not OK
    {
        // Code when USB connection failed
        // Try to open the serial-to-USB port
        /* IO channel variable for file */

        // g_print("\r\nlulElapsed_sec=%d   serial_open returns %d\r\n",
        //         lulElapsed_sec, 
        //         fd = serial_open("/dev/ttyUSB0",115200));
        fd = serial_open("/dev/ttyUSB0",115200);
        //g_print("  fd = %d\r\n", fd);
        if (fd<0)
        {
            if (isFirstSerialFail)
            {
                isFirstSerialFail = FALSE;
                display_status_write("***ERROR*** problem opening ttyUSB0 - connect serial-to-USB cable to USB port\r\n");
            }
            isUSBConnectionOK = FALSE;
        }
        else
        {
            display_status_write("ttyUSB0 opened successfully!\r\n");
            isUSBConnectionOK = TRUE;
            isFirstSerialFail = TRUE;
            gIOChannelSerialUSB = g_io_channel_unix_new(fd);  // creates the correct reference for callback
            
            // Set encoding
            g_io_channel_set_encoding(gIOChannelSerialUSB, NULL, NULL);
            // g_print("\r\n%s() g_io_channel_get_encoding() returned %s\r\n", __FUNCTION__,
            //         g_io_channel_get_encoding(gIOChannelSerialUSB));

            // Specify callback routines for serial read and error
            g_io_add_watch(gIOChannelSerialUSB, 
                           G_IO_IN, 
                           serial_read, 
                           NULL);
            g_io_add_watch(gIOChannelSerialUSB, 
                           G_IO_ERR|G_IO_HUP|G_IO_NVAL, 
                           serial_error, 
                           NULL);
        }
    }
    
    
    //////////////////////////////////////////////////////////
    //
    // Display and parse received messages
    // If a log file is active, save received messages
    //
    //////////////////////////////////////////////////////////
    do
    {
        plcReceivedMsgAvailable = main_receive_msg_read();
        if (plcReceivedMsgAvailable)
        {
            // Reinitialize data age
            gulElapsedTimeSinceDataUpdate_sec = 0;

            // If log file is active, save received message
            // (save it to the logfile NOW; if something unexpected
            //  is triggering the app to crash, perhaps it'll be saved)
            main_logfile_write(plcReceivedMsgAvailable);

            // Display received message
            display_receive_write(plcReceivedMsgAvailable);
            display_receive_write("\r\n");

            // Parse received message
            main_parse_msg(plcReceivedMsgAvailable);
        }
    } while (plcReceivedMsgAvailable);
    
    
    return TRUE;
}
// end main_periodic



////////////////////////////////////////////////////////////////////////////
// Name:         main
// Description:  Main routine for 400 Cellular Diagnostic
// Parameters:   Standard main arguments, unused
// Return:       0 on conventional exit; error otherwise
////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv)
{
    GError *error = NULL;

    gtk_init(&argc, &argv);
    
    //
    // Initalize any globals needed
    //
    gulElapsedTimeSinceDataUpdate_sec = 0;
    //gDateTime = g_date_time_new_now_utc();
    gDateTime = g_date_time_new_now_local();
    lfIsLogfileEnabled = FALSE;
    guiStatusTimestampCountdown_minutes = 1;
    memset (gucStickyErrorStatus, 0x00, sizeof(gucStickyErrorStatus));

    //
    // Enable CSS styling (colors, fonts, text sizes)
    //
    cssProvider = gtk_css_provider_new();
    gtk_css_provider_load_from_path(cssProvider, "theme.css", NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                 GTK_STYLE_PROVIDER(cssProvider),
                                 GTK_STYLE_PROVIDER_PRIORITY_USER);

    //
    // Construct a GtkBuilder instance and load our UI description
    //
    builder = gtk_builder_new();
    if (gtk_builder_add_from_file(builder, GLADE_LAYOUT, &error) == 0)
    {
        g_printerr ("Error loading file: %s\r\n", error->message);
        g_clear_error(&error);
        return 1;
    }
    
    //
    // Connect signal handlers to the constructed widgets
    //
    // Main window
    window = GTK_WINDOW(gtk_builder_get_object(builder, "window1"));
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    

    //
    // Initialize the Main window
    //
    display_main_initialize();
   
    //
    // Enable window decoration (title bar, minimize/maximize/close, etc.)
    //
    gtk_window_set_decorated(GTK_WINDOW(window), TRUE);

    
    //
    // Start the timeout periodic function
    //
    g_timeout_add(MAIN_PERIODIC_INTERVAL_MSEC, main_periodic, NULL);

    display_status_write("=================================<=>=================================\r\n");
    display_status_write("                 Sensaphone 400 Cellular Diagnostic                  \r\n");
    sprintf(lcTempMainString, "                               v%s.%s.%s \r\n", VERSION_A,VERSION_B,VERSION_C);
    display_status_write(lcTempMainString);
    sprintf(lcTempMainString, "                             %s     \r\n", VERSION_DATE);
    display_status_write(lcTempMainString);
    display_status_write("=================================<=>=================================\r\n");


    ///////////////////////////////////////////////////////////////////////////////////////////////////
    //// TEST MAB 2023.01.26
    //// To simulate parsing of satellite info, push test strings into the Receive FIFO
    //main_receive_msg_write("Network_Detect_GNSS_LatLong: UGRMC string is VALID:     Latitude 39.8701455     Longitude -75.4413740");
    //main_receive_msg_write("Network_Detect_GNSS_Satellites: UGGSV satellite counts: GPS=11    GLONASS=7");
    
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    //
    // Kick off GTK main loop
    //
    gtk_main();

    
    
    
    ////////////////////////////////////////////////////////////////////
    //
    // Should only get here when exiting/quitting the GTK application
    //
    ////////////////////////////////////////////////////////////////////
    return (0);
}
// end main


