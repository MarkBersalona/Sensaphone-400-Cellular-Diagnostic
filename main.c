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

char lcTempMainString[200];

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

// I/O channel for serial-to-USB port
GIOChannel *gIOChannelSerialUSB;


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
// Name:         main_LOGENABLE_state_set
// Description:  Callback routine - logfile enable switch clicked
// Parameters:   None
// Return:       None
////////////////////////////////////////////////////////////////////////////
void main_LOGENABLE_state_set(void)
{
    gboolean lfIsLogfileEnabled = gtk_switch_get_active(GTK_SWITCH(swLogfileEnable));
    if (lfIsLogfileEnabled)
    {
        // Logfile has just been enabled, build timestamp filename and open file
        sprintf(lcTempMainString, "Logfile %s 400 Cellular.txt opened\r\n", g_date_time_format(gDateTime, "%Y%m%d %H%M"));
        display_status_write(lcTempMainString);

        // Set the switch state to ON
        gtk_switch_set_state(GTK_SWITCH(swLogfileEnable), TRUE);
    }
    else
    {
        // Logfile has just been disabled, close the logfile and blank the displayed log filename
        display_status_write("Logfile is now closed\r\n");

        // Set the switch state to OFF
        gtk_switch_set_state(GTK_SWITCH(swLogfileEnable), FALSE);
    }
}
// end main_LOGENABLE_state_set

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
        sprintf(lcTempMainString, "MENU button pressed, menu item %d selected\r\n", liMenuItemSelected);
        display_status_write(lcTempMainString);
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
    display_status_write("REBOOT button pressed\r\n");
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
    display_status_write("RESET to Defaults button pressed\r\n");
}
// end main_RTD_clicked

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

    // Look for *** WARNING ***
    plcDetected = strstr((char*)paucReceiveMsg, "*** WARNING ***");
    if (plcDetected)
    {
        // Write the entire warning, including *** WARNING ***, to Status
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
    // Look for *** ERROR ***
    plcDetected = strstr((char*)paucReceiveMsg, "*** ERROR ***");
    if (plcDetected)
    {
        // Write the entire error, including *** ERROR ***, to Status
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected, sizeof(lcTempMainString));
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
    }
    
    // Look for "MAC address:"
    plcDetected = strstr((char*)paucReceiveMsg, "MAC address: ");
    if (plcDetected)
    {
        ///////////////////////////////////////////////////////////////////////////
        //// TEST MAB 2023.01.23
        //// Assume that if we're reading a new MAC address, this is a new UUT.
        display_clear_UUT_values();
        ///////////////////////////////////////////////////////////////////////////
        // Write the MAC address to Status and to the MAC label
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected+13, strlen(plcDetected+13));
        display_status_write("Detected device MAC address: ");
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
        gtk_label_set_text(GTK_LABEL(lblMAC), lcTempMainString);
    }
    
    // Look for "Model number is "
    plcDetected = strstr((char*)paucReceiveMsg, "Model number is ");
    if (plcDetected)
    {
        plcQuoteStart = strchr(plcDetected,   0x22); // search for double quote
        plcQuoteEnd   = strchr(plcQuoteStart+1, 0x22); // search for double quote
        if (plcQuoteStart && plcQuoteEnd) // both quotes found
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
        plcQuoteStart = strchr(plcDetected,   0x22); // search for double quote
        plcQuoteEnd   = strchr(plcQuoteStart+1, 0x22); // search for double quote
        if (plcQuoteStart && plcQuoteEnd) // both quotes found
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
    
    // Look for "Humidity sensor is the "
    plcDetected = strstr((char*)paucReceiveMsg, "Humidity sensor is the ");
    if (plcDetected)
    {
        // Write the humidity sensor to Status and to the humidity sensor label
        memset (lcTempMainString, 0, sizeof(lcTempMainString));
        memcpy (lcTempMainString, plcDetected+23, strlen(plcDetected+23));
        display_status_write("Detected humidity sensor: ");
        display_status_write(lcTempMainString);
        display_status_write("\r\n");
        gtk_label_set_text(GTK_LABEL(lblHumiditySensor), lcTempMainString);
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
        plcQuoteStart = strchr(plcDetected,   0x22); // search for double quote
        plcQuoteEnd   = strchr(plcQuoteStart+1, 0x22); // search for double quote
        if (plcQuoteStart && plcQuoteEnd) // both quotes found
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
    plcDetected = strstr((char*)paucReceiveMsg, "ICCID is ");
    if (plcDetected)
    {
        plcQuoteStart = strchr(plcDetected,   0x22); // search for double quote
        plcQuoteEnd   = strchr(plcQuoteStart+1, 0x22); // search for double quote
        if (plcQuoteStart && plcQuoteEnd) // both quotes found
        {
            // Write the ICCID to Status and to the ICCID label
            memset (lcTempMainString, 0, sizeof(lcTempMainString));
            memcpy (lcTempMainString, plcQuoteStart+1, plcQuoteEnd-plcQuoteStart-1);
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
// Name:         main_receive_msg_write
// Description:  Write a received string to the FIFO
// Parameters:   paucReceiveMsg - pointer to received NULL-terminated string
// Return:       None
////////////////////////////////////////////////////////////////////////////
void 
main_receive_msg_write(char *paucReceiveMsg)
{
    // Zeroize the FIFO entry about to get the received message string
    memset(&gucReceiveFIFO[guiReceiveFIFOWriteIndex][0], 0, RECEIVE_FIFO_MSG_LENGTH_MAX);

    // Copy the received message string into the FIFO entry,
    // then point to the next FIFO entry to receive the next received message string
    memcpy(&gucReceiveFIFO[guiReceiveFIFOWriteIndex][0], paucReceiveMsg, strlen(paucReceiveMsg));
    if (++guiReceiveFIFOWriteIndex >= RECEIVE_FIFO_MSG_COUNT) guiReceiveFIFOWriteIndex = 0;
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
    
    //////////////////////////////////////////////////////////
    //
    // Update timestamps
    //
    //////////////////////////////////////////////////////////
    if (gulUNIXTimestamp != g_get_real_time()/1000000)
    {
        // Updates every second
        gulUNIXTimestamp = g_get_real_time()/1000000;
        //gDateTime = g_date_time_new_now_utc();
        gDateTime = g_date_time_new_now_local();
        ++lulElapsed_sec;
        ++gulElapsedTimeSinceDataUpdate_sec;

        // Display data age if it's been a while since update (update every n seconds)
        if (lulElapsed_sec%300 == 0)
        {
            display_update_data_age();
        }

        if (lulElapsed_sec%60 == 0)
        {
            // Updates every minute
            sprintf(lcTempMainString, "%s: UNIX timestamp %d\t", __FUNCTION__, gulUNIXTimestamp);
            display_status_write(lcTempMainString);
            sprintf(lcTempMainString, "Local time %s\r\n", g_date_time_format(gDateTime, "%Y-%m-%d %H:%M"));
            display_status_write(lcTempMainString);
        }

        if (lulElapsed_sec%(60*60) == 0)
        {
            // Updates every hour
        }

        if (lulElapsed_sec%(60*60*24) == 0)
        {
            // Updates every 24-hour day
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

            // Display received message
            display_receive_write(plcReceivedMsgAvailable);
            display_receive_write("\r\n");

            // Parse received message
            main_parse_msg(plcReceivedMsgAvailable);

            // If log file is active, save received message
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
    
    // g_print("\r\n");
    // g_print("=================================<=>=================================\r\n");
    // g_print("                 Sensaphone 400 Cellular Diagnostic                  \r\n");
    // g_print("                               v%s.%s.%s \r\n", VERSION_A,VERSION_B,VERSION_C);
    // g_print("                             2023.01.17                              \r\n");
    // g_print("=================================<=>=================================\r\n");

    //
    // Initalize any globals needed
    //
    gulElapsedTimeSinceDataUpdate_sec = 0;
    //gDateTime = g_date_time_new_now_utc();
    gDateTime = g_date_time_new_now_local();
    
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
    
/*     // "UP" icon button, to scroll to higher zones
    evntUp = GTK_WIDGET(gtk_builder_get_object(builder, "evntUp"));
    g_signal_connect(evntUp, "button-press-event", G_CALLBACK(zone_db_click_next), NULL);
    // "STOP" icon button, to start/stop auto-scrolling of zones
    evntStop = GTK_WIDGET(gtk_builder_get_object(builder, "evntStop"));
    g_signal_connect(evntStop, "button-press-event", G_CALLBACK(zone_db_enable_scroll), NULL);
    // "DOWN" icon button, to scroll to lower zones
    evntDown = GTK_WIDGET(gtk_builder_get_object(builder, "evntDown"));
    g_signal_connect(evntDown, "button-press-event", G_CALLBACK(zone_db_click_prev), NULL);
    // "GEAR" icon button, to bring up the Diagnostics window
    evntDiagnostics = GTK_WIDGET(gtk_builder_get_object(builder, "evntDiagnostics"));
    g_signal_connect(evntDiagnostics, "button-press-event", G_CALLBACK(display_diagnostics_enter), GTK_WINDOW(window));
 */ 
/*     // Diagnostics window
    dialog = GTK_WIDGET(gtk_builder_get_object(builder, "dialog1"));
    g_signal_connect(dialog, "destroy", G_CALLBACK(gtk_widget_hide), GTK_WIDGET(dialog));
    gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
    // "EXIT" icon button, to leave the Diagnostics window to return to Main window
    evntExit = GTK_WIDGET(gtk_builder_get_object(builder, "evntExit"));
    g_signal_connect(evntExit, "button-press-event", G_CALLBACK(display_diagnostics_exit), NULL);
 */
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


