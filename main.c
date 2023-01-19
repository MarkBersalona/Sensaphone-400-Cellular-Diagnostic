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

char lcTempMainString[100];

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
        GIOChannel *gIOPointer;

        g_print("\r\nlulElapsed_sec=%d   serial_open returns %d\r\n",
                lulElapsed_sec, 
                fd = serial_open("/dev/ttyUSB0",115200));
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
            gIOPointer = g_io_channel_unix_new(fd);  // creates the correct reference for callback
            
            // Set encoding
            g_io_channel_set_encoding(gIOPointer, NULL, NULL);
            g_print("\r\n%s() g_io_channel_get_encoding() returned %s\r\n", __FUNCTION__,
                    g_io_channel_get_encoding(gIOPointer));

            // Specify callback routines for serial read and error
            g_io_add_watch(gIOPointer, 
                           G_IO_IN, 
                           serial_read, 
                           NULL);
            g_io_add_watch(gIOPointer, 
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
            // Display received message
            display_receive_write(plcReceivedMsgAvailable);
            display_receive_write("\r\n");

            // Parse received message

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
    display_status_write("                             2023.01.19                              \r\n");
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


