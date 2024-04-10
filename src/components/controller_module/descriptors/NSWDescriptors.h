#ifndef NSW_DESCRIPTORS_H
#define NSW_DESCRIPTORS_H

#include <stdint.h>

#define NSW_HID_REPORT_DESCRIPTOR \
  0x05, 0x01,                    /* Usage Page (Generic Desktop) */ \
  0x09, 0x05,                    /* Usage (Game Pad) */ \
  0xA1, 0x01,                    /* Collection (Application) */ \
  0x15, 0x00,                    /* Logical Minimum (0) */ \
  0x25, 0x01,                    /* Logical Maximum (1) */ \
  0x35, 0x00,                    /* Physical Minimum (0) */ \
  0x45, 0x01,                    /* Physical Maximum (1) */ \
  0x75, 0x01,                    /* Report Size (1) */ \
  0x95, 0x0E,                    /* Report Count (14) */ \
  0x05, 0x09,                    /* Usage Page (Button) */ \
  0x19, 0x01,                    /* Usage Minimum (1) */ \
  0x29, 0x0E,                    /* Usage Maximum (14) */ \
  0x81, 0x02,                    /* Input (Data, Var, Abs) */ \
  0x95, 0x02,                    /* Report Count (2) */ \
  0x81, 0x01,                    /* Input (Const, Var, Abs) */ \
  0x05, 0x01,                    /* Usage Page (Generic Desktop) */ \
  0x25, 0x07,                    /* Logical Maximum (7) */ \
  0x46, 0x3B, 0x01,              /* Physical Maximum (315) */ \
  0x75, 0x04,                    /* Report Size (4) */ \
  0x95, 0x01,                    /* Report Count (1) */ \
  0x65, 0x14,                    /* Unit (Eng Rot:Angular Pos) */ \
  0x09, 0x39,                    /* Usage (Hat switch) */ \
  0x81, 0x42,                    /* Input (Data,Var,Abs,Null) */ \
  0x65, 0x00,                    /* Unit (None) */ \
  0x95, 0x01,                    /* Report Count (1) */ \
  0x81, 0x01,                    /* Input (Const,Var,Abs) */ \
  0x26, 0xFF, 0x00,              /* Logical Maximum (255) */ \
  0x46, 0xFF, 0x00,              /* Physical Maximum (255) */ \
  0x09, 0x30,                    /* Usage (X) */ \
  0x09, 0x31,                    /* Usage (Y) */ \
  0x09, 0x32,                    /* Usage (Z) */ \
  0x09, 0x35,                    /* Usage (Rz) */ \
  0x75, 0x08,                    /* Report Size (8) */ \
  0x95, 0x04,                    /* Report Count (4) */ \
  0x81, 0x02,                    /* Input (Data, Var, Abs) */ \
  0x75, 0x08,                    /* Report Size (8) */ \
  0x95, 0x01,                    /* Report Count (1) */ \
  0x81, 0x01,                    /* Input (Const, Var, Abs) */ \
  0x05, 0x0C,                    /* Usage Page (Consumer) */ \
  0x09, 0x00,                    /* Usage (Undefined) */ \
  0x15, 0x80,                    /* Logical Minimum (-128) */ \
  0x25, 0x7F,                    /* Logical Maximum (127) */ \
  0x75, 0x08,                    /* Report Size (8) */ \
  0x95, 0x40,                    /* Report Count (64) */ \
  0xB1, 0x02,                    /* Feature (Data, Var, Abs) */ \
  0xC0                           /* End Collection */

#endif