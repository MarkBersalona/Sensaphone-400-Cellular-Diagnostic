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
//#include "json.h"
//#include "zone_db.h"
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





////////////////////////////////////////////////////////////////////////////
// Name:         main_periodic
// Description:  Main periodic code (1 second period)
// Parameters:   None
// Return:       TRUE
////////////////////////////////////////////////////////////////////////////
static gboolean
main_periodic(gpointer data)
{
    static guint32 lulElapsed_sec = 0;
    static guint32 lulAutoscrollElapsed_sec = 0;
    
    ++lulElapsed_sec;
    ++ulElapsedTimeSinceDataUpdate_sec;
    
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
        int fd;
        /* IO channel variable for file */
        GIOChannel *gIOPointer;

        g_print("\r\nlulElapsed_sec=%d   serial_open returns %d\r\n",
                lulElapsed_sec, 
                fd = serial_open("/dev/ttyUSB0",115200));
        //g_print("  fd = %d\r\n", fd);
        if (fd<0)
        {
            g_print("***ERROR*** problem opening ttyUSB0\r\n");
            isUSBConnectionOK = FALSE;
        }
        else
        {
            g_print("ttyUSB0 opened successfully!\r\n");
            isUSBConnectionOK = TRUE;
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
    // Display connection status
    //
    //////////////////////////////////////////////////////////
    // Display connection
    display_update_display_connection();
    
    // Display RMSP connection and Standby mode if active (update every 10 seconds)
    if (lulElapsed_sec%10 == 0)
    {
        // IF USB is connected (connection error code is updated)
        if (isUSBConnectionOK)
        {
            // Display RMSP connection (include Standby mode/state if active)
            display_update_RMSP_connection();
        }
        // ELSE (USB is disconnected, connection error code is really unknown)
        else
        {
            // Clear the RMSP connection display
            display_clear_RMSP_connection();
        }
        // ENDIF (is USB connected)
        
    } // end Display Standby mode every 10 seconds
    
    // Display data age if it's been a while since update (update every 5 seconds)
    if (lulElapsed_sec%5 == 0)
    {
        display_update_data_age();
    }
    
    
    
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
    
    g_print("\r\n");
    g_print("=================================<=>=================================\r\n");
    g_print("                 Sensaphone 400 Cellular Diagnostic                  \r\n");
    g_print("                               v%s.%s.%s \r\n", VERSION_A,VERSION_B,VERSION_C);
    g_print("                             2023.01.16                              \r\n");
    g_print("=================================<=>=================================\r\n");

    
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
   
/*     //
    // Disable window decoration (title bar, minimize/maximize/close, etc.)
    //
    gtk_window_set_decorated(GTK_WINDOW(window), FALSE);
    #if MARK_VERSION == 1
    gtk_window_set_decorated(GTK_WINDOW(dialog), FALSE);
    #endif
 */
 
    //
    // Initialize the Diagnostics dialog
    //
    //display_diagnostics_initialize();
    
    //
    // Start the timeout periodic function
    //
    g_timeout_add(MAIN_PERIODIC_INTERVAL_MSEC, main_periodic, NULL);

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


