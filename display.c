/* 
 * File:   display.c
 * Author: Mark Bersalona
 *
 * Created on 2023.01.16
 * Sensaphone 400 Cellular Diagnostic
 */


///////////////////////////////////////////////////////////////////////////////
//
// Includes
//
///////////////////////////////////////////////////////////////////////////////

#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "gconfig.h"
#include "main.h"
#include "serial.h"
#include "display.h"

///////////////////////////////////////////////////////////////////////////////
//
// Variables and tables
//
///////////////////////////////////////////////////////////////////////////////

// Main display
GtkCssProvider *cssProvider;
GtkWindow *window;

GtkWidget *lblMACTitle, *lblBoardRevTitle, *lblHumiditySensorTitle, *lbl400FWTitle;
GtkWidget *lblMAC, *lblBoardRev, *lblHumiditySensor, *lbl400FW;
GtkWidget *lblTransceiverTitle, *lblTransceiverFWTitle, *lblICCIDTitle, *lblIMEITitle;
GtkWidget *lblTransceiver, *lblTransceiverFW, *lblICCID, *lblIMEI;

GtkWidget *lblNewMACTitle, *lblNewBoardRevTitle, *lblATCommandTitle;
GtkWidget *txtentNewMAC, *txtentNewBoardRev, *txtentATCommand;
GtkWidget *btnNewMAC, *btnNewBoardRev, *btnATCommand;
GtkWidget *cbtMenu, *btnMenu;
GtkWidget *btnRTD, *btnReboot;

GtkWidget *lblZoneTitle, *lblTypeTitle, *lblValueTitle, *lblAlarmTitle;

GtkWidget *lblZonePower, *lblZoneBattery, *lblZoneLithium, *lblZoneIntTemp, *lblZoneHumidity;
GtkWidget *lblZone1, *lblZone2, *lblZone3, *lblZone4, *lblZoneRelay;
GtkWidget *lblTypePower, *lblTypeBattery, *lblTypeLithium, *lblTypeIntTemp, *lblTypeHumidity;
GtkWidget *lblTypeZone1, *lblTypeZone2, *lblTypeZone3, *lblTypeZone4, *lblTypeRelay;
GtkWidget *lblValuePower, *lblValueBattery, *lblValueLithium, *lblValueIntTemp, *lblValueHumidity;
GtkWidget *lblValueZone1, *lblValueZone2, *lblValueZone3, *lblValueZone4, *lblValueRelay;
GtkWidget *lblAlarmPower, *lblAlarmBattery, *lblAlarmLithium, *lblAlarmIntTemp, *lblAlarmHumidity;
GtkWidget *lblAlarmZone1, *lblAlarmZone2, *lblAlarmZone3, *lblAlarmZone4, *lblAlarmRelay;

GtkWidget *lblLatitudeTitle, *lblLongitudeTitle;
GtkWidget *lblLatitude, *lblLongitude;
GtkWidget *lblGPSTitle, *lblGLONASSTitle, *lblGalileoTitle, *lblBeiDouTitle, *lblQNSSTitle;
GtkWidget *lblGPS, *lblGLONASS, *lblGalileo, *lblBeiDou, *lblQNSS;

GtkWidget *lblStatusTitle, *textviewStatus;
GtkWidget *lblReceiveTitle, *lblLogfileTitle, *swLogfileEnable, *lblLogfile;
GtkWidget *textviewReceive;

// Receive view
GtkScrolledWindow *scrolledwindowReceive;
GtkAdjustment *adjReceive;
GtkTextBuffer *textbufReceive;
GtkTextIter textiterReceiveStart;
GtkTextIter textiterReceiveEnd;
// GtkTextMark *textmarkReceive;

// Status view
GtkScrolledWindow *scrolledwindowStatus;
GtkAdjustment *adjStatus;
GtkTextBuffer *textbufStatus;
GtkTextIter textiterStatusStart;
GtkTextIter textiterStatusEnd;
// GtkTextMark *textmarkStatus;

// Menu table
char* pucMenuItems[] =
{
    "Display menu",
    "Sensaphone cert",
    "Cellular info",
    "App CRC",
    "App to ext. flash",

    "App from ext. flash",
    "SARA-R5 defaults",
    "Airplane mode on/off",
    "Calib. zones 1-4",
    "Write zone configs",

    "Read  zone configs",
    "SPI flash test (big)",
    "SPI flash test (small)",
    "ACK all alarms",
    "Read MAC/SN",

    "Read Board rev.",
    "Toggle lamps",
    "Toggle relay",
    "Ping server",
    "Close sockets",

    "RESET to defaults",
    "REBOOT",

    "ENDOFMENU"
};
char* pucMenuCMD[] =
{
    "0", // Display menu
    "2", // Sensaphone cert
    "3", // Cellular info
    "4", // App CRC
    "5", // App to ext. flash

    "6", // App from ext. flash
    "8", // SARA-R5 defaults
    "A", // Airplane mode on/off
    "C", // Calib. zones 1-4
    "E", // Write zone configs
    
    "e", // Read  zone configs
    "F", // SPI flash test (big)
    "f", // SPI flash test (small)
    "K", // ACK all alarms
    "m", // Read MAC/SN
    
    "b", // Read Board rev.
    "L", // Toggle lamps
    "R", // Toggle relay
    "P", // Ping server
    "S", // Close sockets
    
    "X", // RESET to defaults
    "Z", // REBOOT

    "-", // end of menu
 
    
};

char lcTempString[40];

////////////////////////////////////////////////////////////////////////////
// Name:         display_main_initialize
// Description:  Initialize Main window
// Parameters:   None
// Return:       None
////////////////////////////////////////////////////////////////////////////
void
display_main_initialize(void)
{
    //
    // Connect widgets
    //
    // Main display labels
    lblMACTitle            = GTK_WIDGET(gtk_builder_get_object(builder, "lblMACTitle"));
    lblMAC                 = GTK_WIDGET(gtk_builder_get_object(builder, "lblMAC"));
    lblBoardRevTitle       = GTK_WIDGET(gtk_builder_get_object(builder, "lblBoardRevTitle"));
    lblBoardRev            = GTK_WIDGET(gtk_builder_get_object(builder, "lblBoardRev"));
    lblHumiditySensorTitle = GTK_WIDGET(gtk_builder_get_object(builder, "lblHumiditySensorTitle"));
    lblHumiditySensor      = GTK_WIDGET(gtk_builder_get_object(builder, "lblHumiditySensor"));
    lbl400FWTitle          = GTK_WIDGET(gtk_builder_get_object(builder, "lbl400FWTitle"));
    lbl400FW               = GTK_WIDGET(gtk_builder_get_object(builder, "lbl400FW"));
		
    lblTransceiverTitle   = GTK_WIDGET(gtk_builder_get_object(builder, "lblTransceiverTitle"));
    lblTransceiver        = GTK_WIDGET(gtk_builder_get_object(builder, "lblTransceiver"));
    lblTransceiverFWTitle = GTK_WIDGET(gtk_builder_get_object(builder, "lblTransceiverFWTitle"));
    lblTransceiverFW      = GTK_WIDGET(gtk_builder_get_object(builder, "lblTransceiverFW"));
    lblICCIDTitle         = GTK_WIDGET(gtk_builder_get_object(builder, "lblICCIDTitle"));
    lblICCID              = GTK_WIDGET(gtk_builder_get_object(builder, "lblICCID"));
    lblIMEITitle          = GTK_WIDGET(gtk_builder_get_object(builder, "lblIMEITitle"));
    lblIMEI               = GTK_WIDGET(gtk_builder_get_object(builder, "lblIMEI"));
		
    lblNewMACTitle      = GTK_WIDGET(gtk_builder_get_object(builder, "lblNewMACTitle"));
    txtentNewMAC        = GTK_WIDGET(gtk_builder_get_object(builder, "txtentNewMAC"));
    btnNewMAC           = GTK_WIDGET(gtk_builder_get_object(builder, "btnNewMAC"));
    lblNewBoardRevTitle = GTK_WIDGET(gtk_builder_get_object(builder, "lblNewBoardRevTitle"));
    txtentNewBoardRev   = GTK_WIDGET(gtk_builder_get_object(builder, "txtentNewBoardRev"));
    btnNewBoardRev      = GTK_WIDGET(gtk_builder_get_object(builder, "btnNewBoardRev"));
    lblATCommandTitle   = GTK_WIDGET(gtk_builder_get_object(builder, "lblATCommandTitle"));
    txtentATCommand     = GTK_WIDGET(gtk_builder_get_object(builder, "txtentATCommand"));
    btnATCommand        = GTK_WIDGET(gtk_builder_get_object(builder, "btnATCommand"));
		
    cbtMenu             = GTK_WIDGET(gtk_builder_get_object(builder, "cbtMenu"));
    btnMenu             = GTK_WIDGET(gtk_builder_get_object(builder, "btnMenu"));
		
    btnRTD              = GTK_WIDGET(gtk_builder_get_object(builder, "btnRTD"));
    btnReboot           = GTK_WIDGET(gtk_builder_get_object(builder, "btnReboot"));
		
    lblZoneTitle  = GTK_WIDGET(gtk_builder_get_object(builder, "lblZoneTitle"));
    lblTypeTitle  = GTK_WIDGET(gtk_builder_get_object(builder, "lblTypeTitle"));
    lblValueTitle = GTK_WIDGET(gtk_builder_get_object(builder, "lblValueTitle"));
    lblAlarmTitle = GTK_WIDGET(gtk_builder_get_object(builder, "lblAlarmTitle"));
		
    lblZonePower    = GTK_WIDGET(gtk_builder_get_object(builder, "lblZonePower"));
    lblZoneBattery  = GTK_WIDGET(gtk_builder_get_object(builder, "lblZoneBattery"));
    lblZoneLithium  = GTK_WIDGET(gtk_builder_get_object(builder, "lblZoneLithium"));
    lblZoneIntTemp  = GTK_WIDGET(gtk_builder_get_object(builder, "lblZoneIntTemp"));
    lblZoneHumidity = GTK_WIDGET(gtk_builder_get_object(builder, "lblZoneHumidity"));
    lblZone1        = GTK_WIDGET(gtk_builder_get_object(builder, "lblZone1"));
    lblZone2        = GTK_WIDGET(gtk_builder_get_object(builder, "lblZone2"));
    lblZone3        = GTK_WIDGET(gtk_builder_get_object(builder, "lblZone3"));
    lblZone4        = GTK_WIDGET(gtk_builder_get_object(builder, "lblZone4"));
    lblZoneRelay    = GTK_WIDGET(gtk_builder_get_object(builder, "lblZoneRelay"));

    lblTypePower    = GTK_WIDGET(gtk_builder_get_object(builder, "lblTypePower"));
    lblTypeBattery  = GTK_WIDGET(gtk_builder_get_object(builder, "lblTypeBattery"));
    lblTypeLithium  = GTK_WIDGET(gtk_builder_get_object(builder, "lblTypeLithium"));
    lblTypeIntTemp  = GTK_WIDGET(gtk_builder_get_object(builder, "lblTypeIntTemp"));
    lblTypeHumidity = GTK_WIDGET(gtk_builder_get_object(builder, "lblTypeHumidity"));
    lblTypeZone1    = GTK_WIDGET(gtk_builder_get_object(builder, "lblTypeZone1"));
    lblTypeZone2    = GTK_WIDGET(gtk_builder_get_object(builder, "lblTypeZone2"));
    lblTypeZone3    = GTK_WIDGET(gtk_builder_get_object(builder, "lblTypeZone3"));
    lblTypeZone4    = GTK_WIDGET(gtk_builder_get_object(builder, "lblTypeZone4"));
    lblTypeRelay    = GTK_WIDGET(gtk_builder_get_object(builder, "lblTypeRelay"));

    lblValuePower    = GTK_WIDGET(gtk_builder_get_object(builder, "lblValuePower"));
    lblValueBattery  = GTK_WIDGET(gtk_builder_get_object(builder, "lblValueBattery"));
    lblValueLithium  = GTK_WIDGET(gtk_builder_get_object(builder, "lblValueLithium"));
    lblValueIntTemp  = GTK_WIDGET(gtk_builder_get_object(builder, "lblValueIntTemp"));
    lblValueHumidity = GTK_WIDGET(gtk_builder_get_object(builder, "lblValueHumidity"));
    lblValueZone1    = GTK_WIDGET(gtk_builder_get_object(builder, "lblValueZone1"));
    lblValueZone2    = GTK_WIDGET(gtk_builder_get_object(builder, "lblValueZone2"));
    lblValueZone3    = GTK_WIDGET(gtk_builder_get_object(builder, "lblValueZone3"));
    lblValueZone4    = GTK_WIDGET(gtk_builder_get_object(builder, "lblValueZone4"));
    lblValueRelay    = GTK_WIDGET(gtk_builder_get_object(builder, "lblValueRelay"));

    lblAlarmPower    = GTK_WIDGET(gtk_builder_get_object(builder, "lblAlarmPower"));
    lblAlarmBattery  = GTK_WIDGET(gtk_builder_get_object(builder, "lblAlarmBattery"));
    lblAlarmLithium  = GTK_WIDGET(gtk_builder_get_object(builder, "lblAlarmLithium"));
    lblAlarmIntTemp  = GTK_WIDGET(gtk_builder_get_object(builder, "lblAlarmIntTemp"));
    lblAlarmHumidity = GTK_WIDGET(gtk_builder_get_object(builder, "lblAlarmHumidity"));
    lblAlarmZone1    = GTK_WIDGET(gtk_builder_get_object(builder, "lblAlarmZone1"));
    lblAlarmZone2    = GTK_WIDGET(gtk_builder_get_object(builder, "lblAlarmZone2"));
    lblAlarmZone3    = GTK_WIDGET(gtk_builder_get_object(builder, "lblAlarmZone3"));
    lblAlarmZone4    = GTK_WIDGET(gtk_builder_get_object(builder, "lblAlarmZone4"));
    lblAlarmRelay    = GTK_WIDGET(gtk_builder_get_object(builder, "lblAlarmRelay"));

    lblLatitudeTitle   = GTK_WIDGET(gtk_builder_get_object(builder, "lblLatitudeTitle"));
    lblLongitudeTitle  = GTK_WIDGET(gtk_builder_get_object(builder, "lblLongitudeTitle"));
    lblLatitude        = GTK_WIDGET(gtk_builder_get_object(builder, "lblLatitude"));
    lblLongitude       = GTK_WIDGET(gtk_builder_get_object(builder, "lblLongitude"));
    lblGPSTitle        = GTK_WIDGET(gtk_builder_get_object(builder, "lblGPSTitle"));
    lblGLONASSTitle    = GTK_WIDGET(gtk_builder_get_object(builder, "lblGLONASSTitle"));
    lblGalileoTitle    = GTK_WIDGET(gtk_builder_get_object(builder, "lblGalileoTitle"));
    lblBeiDouTitle     = GTK_WIDGET(gtk_builder_get_object(builder, "lblBeiDouTitle"));
    lblQNSSTitle       = GTK_WIDGET(gtk_builder_get_object(builder, "lblQNSSTitle"));
    lblGPS             = GTK_WIDGET(gtk_builder_get_object(builder, "lblGPS"));
    lblGLONASS         = GTK_WIDGET(gtk_builder_get_object(builder, "lblGLONASS"));
    lblGalileo         = GTK_WIDGET(gtk_builder_get_object(builder, "lblGalileo"));
    lblBeiDou          = GTK_WIDGET(gtk_builder_get_object(builder, "lblBeiDou"));
    lblQNSS            = GTK_WIDGET(gtk_builder_get_object(builder, "lblQNSS"));

    lblStatusTitle  = GTK_WIDGET(gtk_builder_get_object(builder, "lblStatusTitle"));
    textviewStatus  = GTK_WIDGET(gtk_builder_get_object(builder, "textviewStatus"));
		
    lblReceiveTitle = GTK_WIDGET(gtk_builder_get_object(builder, "lblReceiveTitle"));
    lblLogfileTitle = GTK_WIDGET(gtk_builder_get_object(builder, "lblLogfileTitle"));
    swLogfileEnable = GTK_WIDGET(gtk_builder_get_object(builder, "swLogfileEnable"));
    lblLogfile      = GTK_WIDGET(gtk_builder_get_object(builder, "lblLogfile"));
    textviewReceive = GTK_WIDGET(gtk_builder_get_object(builder, "textviewReceive"));
		
    // Receive text buffer
    scrolledwindowReceive = GTK_SCROLLED_WINDOW(gtk_builder_get_object(builder, "scrolledwindow2"));
    textbufReceive        = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textviewReceive));
    gtk_text_buffer_get_start_iter(textbufReceive, &textiterReceiveStart);
    gtk_text_buffer_get_end_iter  (textbufReceive, &textiterReceiveEnd);
    // gtk_text_buffer_add_mark(textbufStatus, textmarkStatus, textiterStatusEnd);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(textviewReceive), TRUE);
    
    // Status text buffer
    scrolledwindowStatus = GTK_SCROLLED_WINDOW(gtk_builder_get_object(builder, "scrolledwindow1"));
    textbufStatus  = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textviewStatus));
    gtk_text_buffer_get_start_iter(textbufStatus, &textiterStatusStart);
    gtk_text_buffer_get_end_iter  (textbufStatus, &textiterStatusEnd);
    // gtk_text_buffer_add_mark(textbufStatus, textmarkStatus, textiterStatusEnd);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(textviewStatus), TRUE);
    
    // 
    // Initialize stylings
    //

    // Titles
    gtk_widget_set_name((lblMACTitle),            "DiagnosticsTitle");
    gtk_widget_set_name((lblBoardRevTitle),       "DiagnosticsTitle");
    gtk_widget_set_name((lblHumiditySensorTitle), "DiagnosticsTitle");
    gtk_widget_set_name((lbl400FWTitle),          "DiagnosticsTitle");
    gtk_widget_set_name((lblTransceiverTitle),    "DiagnosticsTitle");
    gtk_widget_set_name((lblTransceiverFWTitle),  "DiagnosticsTitle");
    gtk_widget_set_name((lblICCIDTitle),          "DiagnosticsTitle");
    gtk_widget_set_name((lblIMEITitle),           "DiagnosticsTitle");
    gtk_widget_set_name((lblNewMACTitle),         "DiagnosticsTitle");
    gtk_widget_set_name((lblNewBoardRevTitle),    "DiagnosticsTitle");
    gtk_widget_set_name((lblATCommandTitle),      "DiagnosticsTitle");
    gtk_widget_set_name((lblStatusTitle),         "DiagnosticsTitle");
    gtk_widget_set_name((lblReceiveTitle),        "DiagnosticsTitle");
    gtk_widget_set_name((lblLogfileTitle),        "DiagnosticsTitle");
    gtk_widget_set_name((lblZoneTitle),        "DiagnosticsTitle");
    gtk_widget_set_name((lblTypeTitle),        "DiagnosticsTitle");
    gtk_widget_set_name((lblValueTitle),        "DiagnosticsTitle");
    gtk_widget_set_name((lblAlarmTitle),        "DiagnosticsTitle");
    gtk_widget_set_name((lblLatitudeTitle),        "DiagnosticsTitle");
    gtk_widget_set_name((lblLongitudeTitle),        "DiagnosticsTitle");
    gtk_widget_set_name((lblGPSTitle),        "DiagnosticsTitle");
    gtk_widget_set_name((lblGLONASSTitle),        "DiagnosticsTitle");
    gtk_widget_set_name((lblGalileoTitle),        "DiagnosticsTitle");
    gtk_widget_set_name((lblBeiDouTitle),        "DiagnosticsTitle");
    gtk_widget_set_name((lblQNSSTitle),        "DiagnosticsTitle");

		
	// Values
    gtk_widget_set_name((lblMAC),            "DiagnosticValue");
    gtk_widget_set_name((lblBoardRev),       "DiagnosticValue");
    gtk_widget_set_name((lblHumiditySensor), "DiagnosticValue");
    gtk_widget_set_name((lbl400FW),          "DiagnosticValue");
    gtk_widget_set_name((lblTransceiver),    "DiagnosticValue");
    gtk_widget_set_name((lblTransceiverFW),  "DiagnosticValue");
    gtk_widget_set_name((lblICCID),          "DiagnosticValue");
    gtk_widget_set_name((lblIMEI),           "DiagnosticValue");
    gtk_widget_set_name((lblLogfile),        "DiagnosticValue");

    gtk_widget_set_name((lblTypeZone1),        "DiagnosticValue");
    gtk_widget_set_name((lblTypeZone2),        "DiagnosticValue");
    gtk_widget_set_name((lblTypeZone3),        "DiagnosticValue");
    gtk_widget_set_name((lblTypeZone4),        "DiagnosticValue");

    gtk_widget_set_name((lblValuePower),        "DiagnosticValue");
    gtk_widget_set_name((lblValueBattery),      "DiagnosticValue");
    gtk_widget_set_name((lblValueLithium),      "DiagnosticValue");
    gtk_widget_set_name((lblValueIntTemp),      "DiagnosticValue");
    gtk_widget_set_name((lblValueHumidity),     "DiagnosticValue");
    gtk_widget_set_name((lblValueZone1),        "DiagnosticValue");
    gtk_widget_set_name((lblValueZone2),        "DiagnosticValue");
    gtk_widget_set_name((lblValueZone3),        "DiagnosticValue");
    gtk_widget_set_name((lblValueZone4),        "DiagnosticValue");
    gtk_widget_set_name((lblValueRelay),        "DiagnosticValue");

    gtk_widget_set_name((lblAlarmPower),        "DiagnosticValue");
    gtk_widget_set_name((lblAlarmBattery),      "DiagnosticValue");
    gtk_widget_set_name((lblAlarmLithium),      "DiagnosticValue");
    gtk_widget_set_name((lblAlarmIntTemp),      "DiagnosticValue");
    gtk_widget_set_name((lblAlarmHumidity),     "DiagnosticValue");
    gtk_widget_set_name((lblAlarmZone1),        "DiagnosticValue");
    gtk_widget_set_name((lblAlarmZone2),        "DiagnosticValue");
    gtk_widget_set_name((lblAlarmZone3),        "DiagnosticValue");
    gtk_widget_set_name((lblAlarmZone4),        "DiagnosticValue");
    gtk_widget_set_name((lblAlarmRelay),        "DiagnosticValue");

    gtk_widget_set_name((lblLatitude),        "DiagnosticValue");
    gtk_widget_set_name((lblLongitude),        "DiagnosticValue");
    gtk_widget_set_name((lblGPS),        "DiagnosticValue");
    gtk_widget_set_name((lblGLONASS),        "DiagnosticValue");
    gtk_widget_set_name((lblGalileo),        "DiagnosticValue");
    gtk_widget_set_name((lblBeiDou),        "DiagnosticValue");
    gtk_widget_set_name((lblQNSS),        "DiagnosticValue");

    // Buttons
    gtk_widget_set_name((btnNewMAC),      "button");
    gtk_widget_set_name((btnNewBoardRev), "button");
    gtk_widget_set_name((btnATCommand),   "button");
    gtk_widget_set_name((btnMenu),        "button");
    gtk_widget_set_name((btnRTD),         "button");
    gtk_widget_set_name((btnReboot),      "button");
		
    //
    // Initialize values
	//
    display_clear_UUT_values();
    gtk_label_set_text(GTK_LABEL(lblLogfile), "20230116 1634 400 Cellular.txt");
    // Add items to the menu combo box
    int i = 1; // skipping the first menu item, because it's already in the menu combo box
    while (!strstr(pucMenuItems[i], "ENDOFMENU"))
    {
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(cbtMenu), pucMenuItems[i++]);
    }

    //
    // Link button presses to callback routines
    //
    g_signal_connect(btnATCommand,   "clicked", G_CALLBACK(main_ATCommand_clicked), NULL);
    g_signal_connect(btnNewBoardRev, "clicked", G_CALLBACK(main_BOARDREV_clicked), NULL);
    g_signal_connect(btnNewMAC,      "clicked", G_CALLBACK(main_MAC_clicked), NULL);
    g_signal_connect(btnReboot,      "clicked", G_CALLBACK(main_REBOOT_clicked), NULL);
    g_signal_connect(btnRTD,         "clicked", G_CALLBACK(main_RTD_clicked), NULL);
    g_signal_connect(btnMenu,        "clicked", G_CALLBACK(main_MENU_clicked), NULL);
    g_signal_connect(swLogfileEnable, "state-set", G_CALLBACK(main_LOGENABLE_state_set), NULL);

}
// end display_main_initialize


////////////////////////////////////////////////////////////////////////////
// Name:         display_clear_UUT_values
// Description:  Clear the values of the unit under test
// Parameters:   None
// Return:       None
////////////////////////////////////////////////////////////////////////////
void display_clear_UUT_values(void)
{
    gtk_label_set_text(GTK_LABEL(lblMAC), "00-07-F9-xx-xx");
    gtk_label_set_text(GTK_LABEL(lblBoardRev), "-");
    gtk_label_set_text(GTK_LABEL(lblHumiditySensor), "---------------");
    gtk_label_set_text(GTK_LABEL(lbl400FW), "vX.X.X");
    gtk_label_set_text(GTK_LABEL(lblTransceiver), "-------------------");
    gtk_label_set_text(GTK_LABEL(lblTransceiverFW), "XX.XX");
    gtk_label_set_text(GTK_LABEL(lblICCID), "--------------------");
    gtk_label_set_text(GTK_LABEL(lblIMEI), "---------------");
    
    gtk_label_set_text(GTK_LABEL(lblTypeZone1), "------");
    gtk_label_set_text(GTK_LABEL(lblTypeZone2), "------");
    gtk_label_set_text(GTK_LABEL(lblTypeZone3), "------");
    gtk_label_set_text(GTK_LABEL(lblTypeZone4), "------");
    
    gtk_label_set_text(GTK_LABEL(lblValuePower), "------");
    gtk_label_set_text(GTK_LABEL(lblValueBattery), "------");
    gtk_label_set_text(GTK_LABEL(lblValueLithium), "------");
    gtk_label_set_text(GTK_LABEL(lblValueIntTemp), "------");
    gtk_label_set_text(GTK_LABEL(lblValueHumidity), "------");
    gtk_label_set_text(GTK_LABEL(lblValueZone1), "------");
    gtk_label_set_text(GTK_LABEL(lblValueZone2), "------");
    gtk_label_set_text(GTK_LABEL(lblValueZone3), "------");
    gtk_label_set_text(GTK_LABEL(lblValueZone4), "------");
    gtk_label_set_text(GTK_LABEL(lblValueRelay), "------");
    
    gtk_label_set_text(GTK_LABEL(lblAlarmPower), "------");
    gtk_label_set_text(GTK_LABEL(lblAlarmBattery), "------");
    gtk_label_set_text(GTK_LABEL(lblAlarmLithium), "------");
    gtk_label_set_text(GTK_LABEL(lblAlarmIntTemp), "------");
    gtk_label_set_text(GTK_LABEL(lblAlarmHumidity), "------");
    gtk_label_set_text(GTK_LABEL(lblAlarmZone1), "------");
    gtk_label_set_text(GTK_LABEL(lblAlarmZone2), "------");
    gtk_label_set_text(GTK_LABEL(lblAlarmZone3), "------");
    gtk_label_set_text(GTK_LABEL(lblAlarmZone4), "------");
    gtk_label_set_text(GTK_LABEL(lblAlarmRelay), "------");
    
    
    gtk_label_set_text(GTK_LABEL(lblLatitude),  "------");
    gtk_label_set_text(GTK_LABEL(lblLongitude), "------");
    gtk_label_set_text(GTK_LABEL(lblGPS),       "0");
    gtk_label_set_text(GTK_LABEL(lblGLONASS),   "0");
    gtk_label_set_text(GTK_LABEL(lblGalileo),   "0");
    gtk_label_set_text(GTK_LABEL(lblBeiDou),    "0");
    gtk_label_set_text(GTK_LABEL(lblQNSS),      "0");
}
// end display_clear_UUT_values


////////////////////////////////////////////////////////////////////////////
// Name:         display_receive_write
// Description:  Write a new string buffer to Receive
// Parameters:   paucWriteBuf - pointer to buffer containing chars to
//                              write to Receive; assumes NULL-terminated
// Return:       None
////////////////////////////////////////////////////////////////////////////
void
display_receive_write(char * paucWriteBuf)
{
    // Move cursor to end of text buffer
    gtk_text_iter_forward_to_end(&textiterReceiveEnd);
    gtk_text_buffer_place_cursor(textbufReceive, &textiterReceiveEnd);

    // Write contents of paucWriteBuf string buffer to Receive text buffer
    gtk_text_buffer_insert(textbufReceive, &textiterReceiveEnd, paucWriteBuf, -1);

    // Move to bottom of Receive window
    adjReceive = gtk_scrolled_window_get_vadjustment(scrolledwindowReceive);
    gtk_adjustment_set_value( adjReceive, gtk_adjustment_get_upper(adjReceive) );
}
// end display_status_write


////////////////////////////////////////////////////////////////////////////
// Name:         display_status_write
// Description:  Write a new string buffer to Status
// Parameters:   paucWriteBuf - pointer to buffer containing chars to
//                              write to Status; assumes NULL-terminated
// Return:       None
////////////////////////////////////////////////////////////////////////////
void
display_status_write(char * paucWriteBuf)
{
    // Move cursor to end of text buffer
    gtk_text_iter_forward_to_end(&textiterStatusEnd);
    gtk_text_buffer_place_cursor(textbufStatus, &textiterStatusEnd);

    // Write contents of paucWriteBuf string buffer to Status text buffer
    gtk_text_buffer_insert(textbufStatus, &textiterStatusEnd, paucWriteBuf, -1);

    // Move to bottom of Status window
    adjStatus = gtk_scrolled_window_get_vadjustment(scrolledwindowStatus);
    gtk_adjustment_set_value( adjStatus, gtk_adjustment_get_upper(adjStatus) );
}
// end display_status_write


////////////////////////////////////////////////////////////////////////////
// Name:         display_update_zones
// Description:  Update the main display zones
// Parameters:   None
// Return:       None
////////////////////////////////////////////////////////////////////////////
void
display_update_zones(void)
{
    char lstrBuf[40];
    int zone;
    
/*     // First, clear the existing zone info
    #if MARK_VERSION == 1
    for (int i = 0; i < 8; ++i)
    #else
    for (int i = 0; i < 81; ++i)
    #endif        
    {
        gtk_label_set_text(GTK_LABEL(lblZoneNumber[i]), "");
        gtk_label_set_text(GTK_LABEL(lblZoneName[i]),   "");
        gtk_label_set_text(GTK_LABEL(lblZoneValue[i]),  "");
        gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), "");
    }
    
    // FOR each of the zones to display
    #if MARK_VERSION == 1
    for (int i = 0; i < 8; ++i)
    #else
    for (int i = 0; i < 81; ++i)
    #endif        
    {
        // Point to the zone to be displayed
        zone = firstDisplayedZoneIndex + i;
        if (zone >= IO_NUMBER_OF_ZONES)
        {
            firstDisplayedZoneIndex = IO_NUMBER_OF_ZONES - 1;
            break;
        }

        // Fill in the zone number
        if (SortedZoneIndex[zone] >= IO_POWER_OFFSET   && 
            SortedZoneIndex[zone] <= IO_LITHIUM_OFFSET    )
        {
            strcpy(lstrBuf, "--");
        }
        else if (SortedZoneIndex[zone] >= IO_STANDARD_OFFSET && 
                 SortedZoneIndex[zone] <  IO_MODBUS_OFFSET     )
        {
            sprintf(lstrBuf, 
                    "%d", 
                    SortedZoneIndex[zone] + 1 - IO_STANDARD_OFFSET);
        }
        else if (SortedZoneIndex[zone] >= IO_MODBUS_OFFSET && 
                 SortedZoneIndex[zone] <  IO_RELAY_OFFSET     )
        {
            sprintf(lstrBuf, 
                    "MB%02d", 
                    SortedZoneIndex[zone] + 1 - IO_MODBUS_OFFSET);
        }
        else
        {
            sprintf(lstrBuf, 
                    "O%d", 
                    SortedZoneIndex[zone] + 1 - IO_RELAY_OFFSET);
        }
        gtk_label_set_text(GTK_LABEL(lblZoneNumber[i]), lstrBuf);
        #if MARK_VERSION == 1
        // (if disabled, blank the zone number)
        if (ZoneDB[SortedZoneIndex[zone]].enabled == 0)
        {
            gtk_label_set_text(GTK_LABEL(lblZoneNumber[i]), "");
        }
        #endif
        
        // Fill in the zone name
        gtk_label_set_text(GTK_LABEL(lblZoneName[i]), 
                           ZoneDB[SortedZoneIndex[zone]].name);
        // (if disabled, blank the zone name)
        if (ZoneDB[SortedZoneIndex[zone]].enabled == 0)
        {
            #if MARK_VERSION == 1
            gtk_label_set_text(GTK_LABEL(lblZoneName[i]), "");
            #else
            gtk_widget_set_name((lblZoneName[i]), "lblZoneNameDisabled");
            gtk_label_set_text(GTK_LABEL(lblZoneName[i]), "---< disabled >---");
            #endif
        }
        else
        {
            gtk_widget_set_name((lblZoneName[i]), "lblZoneNameOK");
        }
        
        // Combine value and units, fill in value labels
        if (SortedZoneIndex[zone] == IO_POWER_OFFSET)
        {
            if (ZoneDB[SortedZoneIndex[zone]].value > 0)
                strcpy(lstrBuf, "ON");
            else
                strcpy(lstrBuf, "OFF");
        }
        else if (SortedZoneIndex[zone] == IO_BATTERY_OFFSET ||
                 SortedZoneIndex[zone] == IO_LITHIUM_OFFSET   )
        {
            sprintf(lstrBuf, 
                    "%.00f %s", 
                    (float)ZoneDB[SortedZoneIndex[zone]].value/1000,
                    ZoneDB[SortedZoneIndex[zone]].units);
        }
        else if (SortedZoneIndex[zone] >= IO_RELAY_OFFSET)
        {
            if (ZoneDB[SortedZoneIndex[zone]].value > 0)
                strcpy(lstrBuf, "ON");
            else
                strcpy(lstrBuf, "OFF");
        }
        else
        {
            switch (ZoneDB[SortedZoneIndex[zone]].type)
            {
            case RUNTIME_NO:
            case RUNTIME_NC:
                sprintf(lstrBuf, 
                        "%d %s", 
                        ZoneDB[SortedZoneIndex[zone]].value,
                        ZoneDB[SortedZoneIndex[zone]].units);
                break;
            case NORMALLY_OPEN:
            case NORMALLY_CLOSED:
                if (ZoneDB[SortedZoneIndex[zone]].value == 0)
                    strcpy(lstrBuf, "OPEN");
                else
                    strcpy(lstrBuf, "CLOSED");
                break;
            default:
                sprintf(lstrBuf, 
                        "%.02f %s", 
                        (float)ZoneDB[SortedZoneIndex[zone]].value/1000,
                        ZoneDB[SortedZoneIndex[zone]].units);
                break;
            }
        }
        gtk_label_set_text(GTK_LABEL(lblZoneValue[i]), 
                           lstrBuf);
        // (if disabled, clear value)
        if (ZoneDB[SortedZoneIndex[zone]].enabled == 0)
        {
            gtk_label_set_text(GTK_LABEL(lblZoneValue[i]), "");
        }
        
        // Read alarm values, fill in alarm labels appropriately
        // (for convenience, read alarm values to local copies
        guint8 alarm_status, alarm_range, alarm_unack;
        alarm_status = ZoneDB[SortedZoneIndex[zone]].alarm_status;
        alarm_range  = ZoneDB[SortedZoneIndex[zone]].alarm_range;
        alarm_unack  = ZoneDB[SortedZoneIndex[zone]].alarm_unack;
        if (alarm_status == VAL_ALARM_NONE && 
            alarm_range  == VAL_RANGE_OK   && 
            alarm_unack  == 0                 )
        {
            // No alarm
            // (set zone status to OK style)
            gtk_widget_set_name((lblZoneStatus[i]), 
                                "ZoneStatusOK");
            // (set zone status to "OK")
            gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), 
                           "OK");
        }
        else
        {
            // Alarm exists
            // (set zone status to ALARM style)
            gtk_widget_set_name((lblZoneStatus[i]), 
                                "ZoneStatusAlarm");
            // (set zone status to an acknowledged or unacknowledged indicator)
            if (alarm_unack==0)
            {
                if (alarm_status==MSK_ALARM_LOW)
                    gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), "LO");
                else if (alarm_status==MSK_ALARM_HIGH)
                    gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), "HI");
                else if (alarm_status==MSK_ALARM_RESPONSE)
                    gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), "NR");
                else if (alarm_status==VAL_ALARM_NONE)
                    gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), "??");
                else
                {
                    g_print("%s() Zone %d   alarm_status=%d   alarm_unack=%d\r\n",
                            __FUNCTION__,
                            SortedZoneIndex[zone]+1,
                            alarm_status,
                            alarm_unack);
                    gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), "??");
                }
            }
            else // unack != 0
            {
                if (alarm_status==MSK_ALARM_LOW)
                    gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), "U-LO");
                else if (alarm_status==MSK_ALARM_HIGH)
                    gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), "U-HI");
                else if (alarm_status==MSK_ALARM_RESPONSE)
                    gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), "U-NR");
                else if (alarm_status==VAL_ALARM_NONE)
                    gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), "U-??");
                else
                {
                    g_print("%s() Zone %d   alarm_status=%d   alarm_unack=%d\r\n",
                            __FUNCTION__,
                            SortedZoneIndex[zone]+1,
                            alarm_status,
                            alarm_unack);
                    gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), "U-??");
                }
            }
        }
        // If an output type or disabled, blank the alarm status
        if (ZoneDB[SortedZoneIndex[zone]].type == RELAY_OUTPUT ||
            ZoneDB[SortedZoneIndex[zone]].enabled == 0            )
        {
            gtk_label_set_text(GTK_LABEL(lblZoneStatus[i]), "");
        }
        
    } // for
 */
}
// end display_update_zones

////////////////////////////////////////////////////////////////////////////
// Name:         display_update_data_age
// Description:  Update data age if data hasn't updated "in a while"
// Parameters:   None
// Return:       None
////////////////////////////////////////////////////////////////////////////
void
display_update_data_age(void)
{
    if (gulElapsedTimeSinceDataUpdate_sec < DATA_TIMEOUT_MIN_MINUTES)
    {
        // Data has been updated recently, so connection is OK
    }
    else if (gulElapsedTimeSinceDataUpdate_sec < DATA_TIMEOUT_MIN_DAYS)
    {
        // Data was at least updated within the minimum number of days,
        // so issue a connection warning
    }
    else
    {
        // Data has NOT been updated within the minimum number of days,
        // so issue a connection error
    }

    if (gulElapsedTimeSinceDataUpdate_sec >= DATA_TIMEOUT_MIN_MINUTES &&
        gulElapsedTimeSinceDataUpdate_sec <  DATA_TIMEOUT_MAX_MINUTES    )
    {
        // Data is less than an hour old, report data age in minutes
        sprintf(lcTempString, "Data %d minutes old\r\n", 
                gulElapsedTimeSinceDataUpdate_sec/60);
        display_status_write(lcTempString);
    }
    else if (gulElapsedTimeSinceDataUpdate_sec >= DATA_TIMEOUT_MIN_HOURS &&
             gulElapsedTimeSinceDataUpdate_sec <  DATA_TIMEOUT_MAX_HOURS    )
    {
        // Data is less than a day old, report data age in hours
        sprintf(lcTempString, "Data %d hours old\r\n", 
                gulElapsedTimeSinceDataUpdate_sec/(60*60));
        display_status_write(lcTempString);
    }
    else if (gulElapsedTimeSinceDataUpdate_sec >= DATA_TIMEOUT_MIN_DAYS &&
             gulElapsedTimeSinceDataUpdate_sec <  DATA_TIMEOUT_MAX_DAYS    )
    {
        // Data is less than a month old, report data age in days
        sprintf(lcTempString, "Data %d days old\r\n", 
                gulElapsedTimeSinceDataUpdate_sec/(60*60*24));
        display_status_write(lcTempString);
    }
    else if (gulElapsedTimeSinceDataUpdate_sec >= DATA_TIMEOUT_MIN_MONTHS &&
             gulElapsedTimeSinceDataUpdate_sec <  DATA_TIMEOUT_MAX_MONTHS    )
    {
        // Data is less than a year old, report data age in months
        sprintf(lcTempString, "Data %.01f months old\r\n", 
                (float)gulElapsedTimeSinceDataUpdate_sec/(60*60*24*30));
        display_status_write(lcTempString);
    }
    else if (gulElapsedTimeSinceDataUpdate_sec >= DATA_TIMEOUT_MIN_YEARS)
    {
        // Report data age in years
        sprintf(lcTempString, "Data %.01f years old\r\n", 
                (float)gulElapsedTimeSinceDataUpdate_sec/(60*60*24*365));
        display_status_write(lcTempString);
    }
}
// end display_update_data_age

////////////////////////////////////////////////////////////////////////////
// Name:         display_diagnostics_enter
// Description:  Entry callback for Diagnostics: display Diagnostics dialog
// Parameters:   None
// Return:       None
////////////////////////////////////////////////////////////////////////////
void
display_diagnostics_enter(GtkWindow *parent)
{
    //g_print("%s() Showing the Diagnostics dialog\r\n", __FUNCTION__);
    //gtk_widget_show_all(dialog);
}
// end display_diagnostics_enter

////////////////////////////////////////////////////////////////////////////
// Name:         display_diagnostics_exit
// Description:  Exit callback for Diagnostics: hide Diagnostics dialog
// Parameters:   None
// Return:       None
////////////////////////////////////////////////////////////////////////////
void
display_diagnostics_exit(void)
{
    //g_print("%s() Hiding the Diagnostics dialog\r\n", __FUNCTION__);
    //gtk_widget_hide(GTK_WIDGET(dialog));
}
// end display_diagnostics_exit
