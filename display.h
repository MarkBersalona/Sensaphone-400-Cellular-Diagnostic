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


    
///////////////////////////////////////////////////////////////////////////////
//
// Public routines
//
///////////////////////////////////////////////////////////////////////////////

void display_main_initialize(void);
void display_status_write(char * paucWriteBuf);
void display_update_zones(void);
void display_update_display_connection(void);
void display_update_data_age(void);
void display_diagnostics_enter(GtkWindow *parent);
void display_diagnostics_exit(void);




#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */

