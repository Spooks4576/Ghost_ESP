#pragma once

#define SET_VALUE       0x21
#define GET_VALUE       0xA1

// DTR/RTS control in SET_CONTROL_LINE_STATE
#define ENABLE_DTR(val)      (val<<0)
#define ENABLE_RTS(val)      (val<<1)

#define SET_LINE_CODING 0x20
#define GET_LINE_CODING 0x21
#define SET_CONTROL_LINE_STATE 0x22
// #define SERIAL_STATE    0x20

#define MSC_SCSI_REQ_INIT_RESET(setup_pkt_ptr, intf_num) ({  \
    (setup_pkt_ptr)->bmRequestType = USB_BM_REQUEST_TYPE_DIR_OUT | USB_BM_REQUEST_TYPE_TYPE_CLASS | USB_BM_REQUEST_TYPE_RECIP_INTERFACE; \
    (setup_pkt_ptr)->bRequest = 0xFF;    \
    (setup_pkt_ptr)->wValue = 0; \
    (setup_pkt_ptr)->wIndex = (intf_num);    \
    (setup_pkt_ptr)->wLength = 0;    \
})

#define MSC_SCSI_REQ_MAX_LUN(setup_pkt_ptr, intf_num) ({  \
    (setup_pkt_ptr)->bmRequestType = USB_BM_REQUEST_TYPE_DIR_IN | USB_BM_REQUEST_TYPE_TYPE_CLASS | USB_BM_REQUEST_TYPE_RECIP_INTERFACE; \
    (setup_pkt_ptr)->bRequest = 0xFE;    \
    (setup_pkt_ptr)->wValue = 0; \
    (setup_pkt_ptr)->wIndex = (intf_num);    \
    (setup_pkt_ptr)->wLength = 1;    \
})

/**
 * @brief 
 *
 */
#define USB_CTRL_REQ_CDC_SET_LINE_CODING(ctrl_req_ptr, index, bitrate, cf, parity, bits) ({  \
    (ctrl_req_ptr)->bmRequestType = SET_VALUE;   \
    (ctrl_req_ptr)->bRequest = SET_LINE_CODING;  \
    (ctrl_req_ptr)->wValue = 0;   \
    (ctrl_req_ptr)->wIndex = (index);    \
    (ctrl_req_ptr)->wLength = (7);   \
})

#define USB_CTRL_REQ_CDC_GET_LINE_CODING(ctrl_req_ptr, index) ({  \
    (ctrl_req_ptr)->bmRequestType = GET_VALUE;   \
    (ctrl_req_ptr)->bRequest = GET_LINE_CODING;  \
    (ctrl_req_ptr)->wValue = 0;   \
    (ctrl_req_ptr)->wIndex = (index);    \
    (ctrl_req_ptr)->wLength = (7);   \
})

#define USB_CTRL_REQ_CDC_SET_CONTROL_LINE_STATE(ctrl_req_ptr, index, dtr, rts) ({  \
    (ctrl_req_ptr)->bmRequestType = SET_VALUE;   \
    (ctrl_req_ptr)->bRequest = SET_CONTROL_LINE_STATE;  \
    (ctrl_req_ptr)->wValue = ENABLE_DTR(dtr) | ENABLE_RTS(rts);   \
    (ctrl_req_ptr)->wIndex = (index);    \
    (ctrl_req_ptr)->wLength = (0);   \
})


/// SCSI Command Operation Code
typedef enum
{
  SCSI_CMD_TEST_UNIT_READY              = 0x00, ///< The SCSI Test Unit Ready command is used to determine if a device is ready to transfer data (read/write), i.e. if a disk has spun up, if a tape is loaded and ready etc. The device does not perform a self-test operation.
  SCSI_CMD_MAX_LUN                      = 0x01, ///< The SCSI Test Unit Ready command is used to determine if a device is ready to transfer data (read/write), i.e. if a disk has spun up, if a tape is loaded and ready etc. The device does not perform a self-test operation.
  SCSI_CMD_FORMAT_UNIT                  = 0x04, ///
  SCSI_CMD_INQUIRY                      = 0x12, ///< The SCSI Inquiry command is used to obtain basic information from a target device.
  SCSI_CMD_MODE_SELECT_6                = 0x15, ///<  provides a means for the application client to specify medium, logical unit, or peripheral device parameters to the device server. Device servers that implement the MODE SELECT(6) command shall also implement the MODE SENSE(6) command. Application clients should issue MODE SENSE(6) prior to each MODE SELECT(6) to determine supported mode pages, page lengths, and other parameters.
  SCSI_CMD_MODE_SENSE_6                 = 0x1A, ///< provides a means for a device server to report parameters to an application client. It is a complementary command to the MODE SELECT(6) command. Device servers that implement the MODE SENSE(6) command shall also implement the MODE SELECT(6) command.
  SCSI_CMD_START_STOP_UNIT              = 0x1B,
  SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL = 0x1E,
  SCSI_CMD_READ_CAPACITY_10             = 0x25, ///< The SCSI Read Capacity command is used to obtain data capacity information from a target device.
  SCSI_CMD_REQUEST_SENSE                = 0x03, ///< The SCSI Request Sense command is part of the SCSI computer protocol standard. This command is used to obtain sense data -- status/error information -- from a target device.
  SCSI_CMD_READ_FORMAT_CAPACITY         = 0x23, ///< The command allows the Host to request a list of the possible format capacities for an installed writable media. This command also has the capability to report the writable capacity for a media when it is installed
  SCSI_CMD_READ_10                      = 0x28, ///< The READ (10) command requests that the device server read the specified logical block(s) and transfer them to the data-in buffer.
  SCSI_CMD_WRITE_10                     = 0x2A, ///< The WRITE (10) command requests thatthe device server transfer the specified logical block(s) from the data-out buffer and write them.
}scsi_cmd_type_t;

typedef struct __attribute__((packed)) {
    uint8_t opcode; //0x28 = read(10), 0x2A=write(10)
    uint8_t flags;
    uint8_t lba_3;
    uint8_t lba_2;
    uint8_t lba_1;
    uint8_t lba_0;
    uint8_t group;
    uint8_t len_1;
    uint8_t len_0;
    uint8_t control;
} scsi_cmd10_t;

typedef struct __attribute__((packed)) {
    uint32_t dCBWSignature;
    uint32_t dCBWTag;
    uint32_t dCBWDataTransferLength;
    uint8_t bmCBWFlags;
    uint8_t bCBWLUN;
    uint8_t bCBWCBLength;
    scsi_cmd10_t CBWCB;
    uint8_t padding[6];
} msc_bulk_cbw_t;

// USB Bulk Transfer Command Status Wrapper data
typedef struct __attribute__((packed)) {
    uint32_t dCSWSignature;
    uint32_t dCSWTag;
    uint32_t dCSWDataResidue;
    uint8_t bCSWStatus;
} msc_bulk_csw_t;

typedef struct {
    usb_transfer_cb_t cbw_cb;
    usb_transfer_cb_t data_cb;
    usb_transfer_cb_t csw_cb;
    usb_transfer_cb_t capacity_cb;
    usb_transfer_cb_t inquiry_cb;
    usb_transfer_cb_t unit_ready_cb;
    usb_transfer_cb_t max_luns_cb;
    usb_transfer_cb_t sense_cb;
} msc_transfer_cb_t;

typedef struct{
    uint32_t dwDTERate;
    uint8_t bCharFormat;
    uint8_t bParityType;
    uint8_t bDataBits;
}line_coding_t;

