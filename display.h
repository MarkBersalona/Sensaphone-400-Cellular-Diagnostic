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
extern GtkWidget *txtentNewMAC, *txtentNewBoardRev, *txtentATCommand;
extern GtkWidget *cbtMenu;
extern GtkWidget *swLogfileEnable, *lblLogfile;
extern GtkWidget *lblRTC, *lblConnection;
extern GtkWidget *lblRSSI, *lblRSRP, *lblRSRQ, *lblSignalQuality;

extern GtkWidget *lblTypePower, *lblTypeBattery, *lblTypeLithium, *lblTypeIntTemp, *lblTypeHumidity;
extern GtkWidget *lblTypeZone1, *lblTypeZone2, *lblTypeZone3, *lblTypeZone4, *lblTypeRelay;
extern GtkWidget *lblValuePower, *lblValueBattery, *lblValueLithium, *lblValueIntTemp, *lblValueHumidity;
extern GtkWidget *lblValueZone1, *lblValueZone2, *lblValueZone3, *lblValueZone4, *lblValueRelay;
extern GtkWidget *lblAlarmPower, *lblAlarmBattery, *lblAlarmLithium, *lblAlarmIntTemp, *lblAlarmHumidity;
extern GtkWidget *lblAlarmZone1, *lblAlarmZone2, *lblAlarmZone3, *lblAlarmZone4, *lblAlarmRelay;

extern GtkWidget *lblLatitude, *lblLongitude;
extern GtkWidget *lblGPS, *lblGLONASS, *lblGalileo, *lblBeiDou, *lblQNSS;

extern GtkWidget *lblLogfile;

extern GtkWidget *lblStatusTitle;
extern GtkTextBuffer *textbufStatus;
extern GtkTextBuffer *textbufReceive;
extern GtkTextIter textiterReceiveStart;
extern GtkTextIter textiterReceiveEnd;
extern GtkTextIter textiterStatusEnd;
extern GtkScrolledWindow *scrolledwindowStatus;
extern GtkAdjustment *adjStatus;

    
// Menu tables
extern char* pucMenuItems[];
extern char* pucMenuCMD[];

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

