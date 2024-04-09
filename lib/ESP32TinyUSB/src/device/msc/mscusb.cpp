#include "mscusb.h"

#if CFG_TUD_MSC

#include "mscusb.h"
#define EPNUM_MSC 0x04
MSCusb* _MSCusb[4] = { };
static uint8_t luns = 0;

MSCusb::MSCusb()
{
    m_lun = luns++;
    enableMSC = true;
    _EPNUM_MSC = EPNUM_MSC;
    _MSCusb[m_lun] = this;
}

bool MSCusb::begin(char* str)
{
  if(m_lun >= 4) return false;
  if(m_lun == 0) { // init only first time, then just add luns
    // Interface number, string index, EP Out & EP In address, EP size
    uint8_t msc[] = {TUD_MSC_DESCRIPTOR(ifIdx++, 5, _EPNUM_MSC, (uint8_t)(0x80 | _EPNUM_MSC), 64)}; // highspeed 512
    memcpy(&desc_configuration[total], msc, sizeof(msc));
    total += sizeof(msc);
    count++;
  }
    if (!EspTinyUSB::begin(str, 5)) return false;
    return true;
}

void MSCusb::setBaseEP(uint8_t ep)
{
  _EPNUM_MSC = ep;
}

void MSCusb::setCallbacks(MSCCallbacks* cb)
{
  m_callbacks = cb;
}

TU_ATTR_WEAK void tud_msc_inquiry_cb(uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4])
{
  for (size_t i = 0; i < 4; i++)
  {
    if(_MSCusb[i] && _MSCusb[i]->m_callbacks && _MSCusb[i]->m_lun == lun) {
      _MSCusb[i]->m_callbacks->onInquiry(lun, vendor_id, product_id, product_rev);
    }
  }
}

TU_ATTR_WEAK bool tud_msc_test_unit_ready_cb(uint8_t lun)
{
  for (size_t i = 0; i < 4; i++)
  {
    if(_MSCusb[i] && _MSCusb[i]->m_callbacks && _MSCusb[i]->m_lun == lun) {
      return _MSCusb[i]->m_callbacks->onReady(lun);
    }
  }

  return false;
}

TU_ATTR_WEAK void tud_msc_capacity_cb(uint8_t lun, uint32_t* block_count, uint16_t* block_size)
{
  for (size_t i = 0; i < 4; i++)
  {
    if(_MSCusb[i] && _MSCusb[i]->m_callbacks && _MSCusb[i]->m_lun == lun) {
      _MSCusb[i]->m_callbacks->onCapacity(lun, block_count, block_size);
    }
  }
}

TU_ATTR_WEAK bool tud_msc_start_stop_cb(uint8_t lun, uint8_t power_condition, bool start, bool load_eject)
{
  for (size_t i = 0; i < 4; i++)
  {
    if(_MSCusb[i] && _MSCusb[i]->m_callbacks && _MSCusb[i]->m_lun == lun) {
      return _MSCusb[i]->m_callbacks->onStop(lun, power_condition, start, load_eject);
    }
  }

  return true;
}

TU_ATTR_WEAK int32_t tud_msc_read10_cb (uint8_t lun, uint32_t lba, uint32_t offset, void* buffer, uint32_t bufsize)
{
  for (size_t i = 0; i < 4; i++)
  {
    if(_MSCusb[i] && _MSCusb[i]->m_callbacks && _MSCusb[i]->m_lun == lun) {
      return _MSCusb[i]->m_callbacks->onRead(lun, lba, offset, buffer, bufsize);
    }
  }

  return -1;
}

TU_ATTR_WEAK int32_t tud_msc_write10_cb(uint8_t lun, uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t bufsize)
{
  for (size_t i = 0; i < 4; i++)
  {
    if(_MSCusb[i] && _MSCusb[i]->m_callbacks && _MSCusb[i]->m_lun == lun) {
      return _MSCusb[i]->m_callbacks->onWrite(lun, lba, offset, buffer, bufsize);
    }
  }

  return -1;
}

TU_ATTR_WEAK int32_t tud_msc_scsi_cb (uint8_t lun, uint8_t const scsi_cmd[16], void* buffer, uint16_t bufsize)
{

  void const* response = NULL;
  uint16_t resplen = 0;

  // most scsi handled is input
  bool in_xfer = true;

  switch (scsi_cmd[0])
  {
    case SCSI_CMD_PREVENT_ALLOW_MEDIUM_REMOVAL:
      // Host is about to read/write etc ... better not to disconnect disk
      resplen = 0;
    break;

    default:
      // Set Sense = Invalid Command Operation
      tud_msc_set_sense(lun, SCSI_SENSE_ILLEGAL_REQUEST, 0x20, 0x00);

      // negative means error -> tinyusb could stall and/or response with failed status
      resplen = -1;
    break;
  }

  // return resplen must not larger than bufsize
  if ( resplen > bufsize ) resplen = bufsize;

  if ( response && (resplen > 0) )
  {
    if(in_xfer)
    {
      memcpy(buffer, response, resplen);
    }else
    {
      // SCSI output
    }
  }

  return resplen;
}

// Support multi LUNs
uint8_t tud_msc_get_maxlun_cb(void)
{
  return luns;
}

#endif
