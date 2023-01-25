/* 
 * File:   main.h
 * Author: Mark Bersalona
 *
 * Created on 2023.01.16
 */

#ifndef MAIN_H
#define MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Definitions
//
///////////////////////////////////////////////////////////////////////////////



// Limits for data age
#define DATA_TIMEOUT_MIN_MINUTES (60*2)
#define DATA_TIMEOUT_MAX_MINUTES (60*120)
#define DATA_TIMEOUT_MIN_HOURS   (DATA_TIMEOUT_MAX_MINUTES)
#define DATA_TIMEOUT_MAX_HOURS   (60*60*48)
#define DATA_TIMEOUT_MIN_DAYS    (DATA_TIMEOUT_MAX_HOURS)
#define DATA_TIMEOUT_MAX_DAYS    (60*60*24*64)
#define DATA_TIMEOUT_MIN_MONTHS  (DATA_TIMEOUT_MAX_DAYS)
#define DATA_TIMEOUT_MAX_MONTHS  (60*60*24*30*24)
#define DATA_TIMEOUT_MIN_YEARS   (DATA_TIMEOUT_MAX_MONTHS)




///////////////////////////////////////////////////////////////////////////////
//
// Public routines
//
///////////////////////////////////////////////////////////////////////////////
char * main_receive_msg_read(void);
void main_receive_msg_write(char *paucReceiveMsg);

void main_ATCommand_clicked(void);
void main_LOGENABLE_state_set(void);
void main_MENU_clicked(void);
void main_REBOOT_clicked(void);
void main_RTD_clicked(void);






///////////////////////////////////////////////////////////////////////////////
//
// Public variables
//
///////////////////////////////////////////////////////////////////////////////

// GTK builder
extern GtkBuilder *builder;

// Elapsed time since last data update
extern guint32 gulElapsedTimeSinceDataUpdate_sec;

// I/O channel for serial-to-USB port
extern GIOChannel *gIOChannelSerialUSB;


#ifdef __cplusplus
}
#endif

#endif /* MAIN_H */

