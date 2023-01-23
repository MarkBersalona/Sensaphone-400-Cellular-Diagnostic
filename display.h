/* 
 * File:   display.h
 * Author: Mark Bersalona
 *
 * Created on 2023.01.16
 */

#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Public variables
//
///////////////////////////////////////////////////////////////////////////////

// Main display
extern GtkCssProvider *cssProvider;
extern GtkWindow *window;
extern GtkWidget *lblMAC, *lblBoardRev, *lblHumiditySensor, *lbl400FW;
extern GtkWidget *lblTransceiver, *lblTransceiverFW, *lblICCID, *lblIMEI;
extern GtkWidget *cbtMenu;
extern GtkWidget *swLogfileEnable, *lblLogfile;

    
///////////////////////////////////////////////////////////////////////////////
//
// Public routines
//
///////////////////////////////////////////////////////////////////////////////

void display_clear_UUT_values(void);
void display_diagnostics_enter(GtkWindow *parent);
void display_diagnostics_exit(void);
void display_main_initialize(void);
void display_receive_write(char * paucWriteBuf);
void display_status_write(char * paucWriteBuf);
void display_update_zones(void);
void display_update_display_connection(void);
void display_update_data_age(void);



#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */

