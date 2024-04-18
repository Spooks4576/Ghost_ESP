#ifndef KEYBOARDDESCRIPTORS_H
#define KEYBOARDDESCRIPTORS_H

#include <HIDTypes.h>

#define KEYBOARD_REPORT_ID 0x40
#define MEDIA_KEYS_REPORT_ID 0x43

static const uint8_t _keyboardHIDReportDescriptor[] = {
  // Input
  USAGE_PAGE(1),      0x01,                     // USAGE_PAGE (Generic Desktop Ctrls)
  USAGE(1),           0x06,                     // USAGE (Keyboard)
  COLLECTION(1),      0x01,                     // COLLECTION (Application)
  REPORT_ID(1),       KEYBOARD_REPORT_ID,       //   REPORT_ID (1)
  USAGE_PAGE(1),      0x07,                     //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0xE0,                     //   USAGE_MINIMUM (0xE0)
  USAGE_MAXIMUM(1),   0xE7,                     //   USAGE_MAXIMUM (0xE7)
  LOGICAL_MINIMUM(1), 0x00,                     //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,                     //   Logical Maximum (1)
  REPORT_SIZE(1),     0x01,                     //   REPORT_SIZE (1)
  REPORT_COUNT(1),    0x08,                     //   REPORT_COUNT (8)
  HIDINPUT(1),        0x02,                     //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position) ; Modifier byte
  REPORT_COUNT(1),    0x01,                     //   REPORT_COUNT (1) ; 1 byte (Reserved)
  REPORT_SIZE(1),     0x08,                     //   REPORT_SIZE (8)
  HIDINPUT(1),        0x01,                     //   INPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position) ; Reserved byte
  REPORT_COUNT(1),    0x06,                     //   REPORT_COUNT (6) ; 6 bytes (Keys)
  REPORT_SIZE(1),     0x08,                     //   REPORT_SIZE(8)
  LOGICAL_MINIMUM(1), 0x00,                     //   LOGICAL_MINIMUM(0)
  LOGICAL_MAXIMUM(1), 0x65,                     //   LOGICAL_MAXIMUM(0x65) ; 101 keys
  USAGE_PAGE(1),      0x07,                     //   USAGE_PAGE (Kbrd/Keypad)
  USAGE_MINIMUM(1),   0x00,                     //   USAGE_MINIMUM (0)
  USAGE_MAXIMUM(1),   0x65,                     //   USAGE_MAXIMUM (0x65)
  HIDINPUT(1),        0x00,                     //   INPUT (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position) ; Key arrays (6 bytes)
  // Output
  REPORT_COUNT(1),    0x05,                     //   REPORT_COUNT (5) ; 5 bits (Num lock, Caps lock, Scroll lock, Compose, Kana)
  REPORT_SIZE(1),     0x01,                     //   REPORT_SIZE (1)
  USAGE_PAGE(1),      0x08,                     //   USAGE_PAGE (LEDs)
  USAGE_MINIMUM(1),   0x01,                     //   USAGE_MINIMUM (0x01) ; Num Lock
  USAGE_MAXIMUM(1),   0x05,                     //   USAGE_MAXIMUM (0x05) ; Kana
  HIDOUTPUT(1),       0x02,                     //   OUTPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  REPORT_COUNT(1),    0x01,                     //   REPORT_COUNT (1) ; 3 bits (Padding)
  REPORT_SIZE(1),     0x03,                     //   REPORT_SIZE (3)
  HIDOUTPUT(1),       0x01,                     //   OUTPUT (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
  END_COLLECTION(0),                            // END_COLLECTION
};

static const uint8_t _mediakeysHIDReportDescriptor[] = {
  USAGE_PAGE(1),      0x0C,                         // USAGE_PAGE (Consumer)
  USAGE(1),           0x01,                         // USAGE (Consumer Control)
  COLLECTION(1),      0x01,                         // COLLECTION (Application)
  REPORT_ID(1),       MEDIA_KEYS_REPORT_ID,         //   REPORT_ID (3)
  USAGE_PAGE(1),      0x0C,                         //   USAGE_PAGE (Consumer)
  LOGICAL_MINIMUM(1), 0x00,                         //   LOGICAL_MINIMUM (0)
  LOGICAL_MAXIMUM(1), 0x01,                         //   LOGICAL_MAXIMUM (1)
  REPORT_SIZE(1),     0x01,                         //   REPORT_SIZE (1)
  REPORT_COUNT(1),    0x18,                         //   REPORT_COUNT (24)
  USAGE(1),           0xB0,                         //   USAGE (Play)
  USAGE(1),           0xB1,                         //   USAGE (Pause)
  USAGE(1),           0xB2,                         //   USAGE (Record)
  USAGE(1),           0xB3,                         //   USAGE (Fast Forward)
  USAGE(1),           0xB4,                         //   USAGE (Rewind)
  USAGE(1),           0xB5,                         //   USAGE (Scan Next Track)      
  USAGE(1),           0xB6,                         //   USAGE (Scan Previous Track)  
  USAGE(1),           0xB7,                         //   USAGE (Stop)                
  USAGE(1),           0xB8,                         //   USAGE (Eject)    
  USAGE(1),           0xB9,                         //   USAGE (Random Play)         
  USAGE(1),           0xBC,                         //   USAGE (Repeat)
  USAGE(1),           0xCD,                         //   USAGE (Play/Pause)          
  USAGE(1),           0xE2,                         //   USAGE (Mute)                
  USAGE(1),           0xE9,                         //   USAGE (Volume Increment)    
  USAGE(1),           0xEA,                         //   USAGE (Volume Decrement)    
  // 2 byte usages  
  USAGE(2),           0x23, 0x02,                   //   Usage (WWW Home)
  USAGE(2),           0x94, 0x01,                   //   Usage (My Computer) 
  USAGE(2),           0x92, 0x01,                   //   Usage (Calculator)  
  USAGE(2),           0x2A, 0x02,                   //   Usage (WWW fav)     
  USAGE(2),           0x21, 0x02,                   //   Usage (WWW search)  
  USAGE(2),           0x26, 0x02,                   //   Usage (WWW stop)    
  USAGE(2),           0x24, 0x02,                   //   Usage (WWW back)    
  USAGE(2),           0x83, 0x01,                   //   Usage (Media sel)   
  USAGE(2),           0x8A, 0x01,                   //   Usage (Mail)        
  HIDINPUT(1),        0x02,                         //   INPUT (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
  END_COLLECTION(0)                                 // END_COLLECTION
};


#endif // KEYBOARDDESCRIPTORS_H
