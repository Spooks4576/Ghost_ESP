#include "vendor/drivers/pcf8563.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "PCF8563";

static i2c_port_t pcf_i2c_port;
static uint8_t pcf_address;

static uint8_t _bcd_to_dec(uint8_t val) {
  return ((val / 16 * 10) + (val % 16));
}

static uint8_t _dec_to_bcd(uint8_t val) {
  return ((val / 10 * 16) + (val % 10));
}

esp_err_t pcf8563_init(i2c_port_t i2c_port, uint8_t addr) {
  pcf_i2c_port = i2c_port;
  pcf_address = addr;
  return ESP_OK;
}

static esp_err_t _read_register(uint8_t reg, uint8_t *data, size_t len) {
  return i2c_master_write_read_device(pcf_i2c_port, pcf_address, &reg, 1, data,
                                      len, pdMS_TO_TICKS(1000));
}

static esp_err_t _write_register(uint8_t reg, const uint8_t *data, size_t len) {
  uint8_t buffer[1 + len];
  buffer[0] = reg;
  memcpy(&buffer[1], data, len);
  return i2c_master_write_to_device(pcf_i2c_port, pcf_address, buffer,
                                    sizeof(buffer), pdMS_TO_TICKS(1000));
}

esp_err_t pcf8563_set_datetime(const RTC_Date *datetime) {
  uint8_t data[7];
  data[0] = _dec_to_bcd(datetime->second) & ~PCF8563_VOL_LOW_MASK;
  data[1] = _dec_to_bcd(datetime->minute);
  data[2] = _dec_to_bcd(datetime->hour);
  data[3] = _dec_to_bcd(datetime->day);
  data[4] = _dec_to_bcd(datetime->month) |
            ((datetime->year < 2000) ? PCF8563_CENTURY_MASK : 0);
  data[5] = _dec_to_bcd(datetime->year % 100);
  return _write_register(PCF8563_SEC_REG, data, 7);
}

esp_err_t pcf8563_get_datetime(RTC_Date *datetime) {
  uint8_t data[7];
  ESP_ERROR_CHECK(_read_register(PCF8563_SEC_REG, data, 7));

  bool century = data[5] & PCF8563_CENTURY_MASK;
  datetime->second = _bcd_to_dec(data[0] & ~PCF8563_VOL_LOW_MASK);
  datetime->minute = _bcd_to_dec(data[1] & PCF8563_MINUTES_MASK);
  datetime->hour = _bcd_to_dec(data[2] & PCF8563_HOUR_MASK);
  datetime->day = _bcd_to_dec(data[3] & PCF8563_DAY_MASK);
  datetime->month = _bcd_to_dec(data[4] & PCF8563_MONTH_MASK);
  datetime->year = (century ? 1900 : 2000) + _bcd_to_dec(data[5]);
  return ESP_OK;
}

esp_err_t pcf8563_set_alarm(const RTC_Alarm *alarm) {
  uint8_t data[4];
  data[0] = (alarm->minute != 0xFF)
                ? _dec_to_bcd(alarm->minute) & ~PCF8563_VOL_LOW_MASK
                : PCF8563_VOL_LOW_MASK;
  data[1] = (alarm->hour != 0xFF)
                ? _dec_to_bcd(alarm->hour) & ~PCF8563_VOL_LOW_MASK
                : PCF8563_VOL_LOW_MASK;
  data[2] = (alarm->day != 0xFF)
                ? _dec_to_bcd(alarm->day) & ~PCF8563_VOL_LOW_MASK
                : PCF8563_VOL_LOW_MASK;
  data[3] = (alarm->weekday != 0xFF)
                ? _dec_to_bcd(alarm->weekday) & ~PCF8563_VOL_LOW_MASK
                : PCF8563_VOL_LOW_MASK;
  return _write_register(PCF8563_ALRM_MIN_REG, data, 4);
}

esp_err_t pcf8563_get_alarm(RTC_Alarm *alarm) {
  uint8_t data[4];
  ESP_ERROR_CHECK(_read_register(PCF8563_ALRM_MIN_REG, data, 4));

  alarm->minute = (data[0] != PCF8563_VOL_LOW_MASK)
                      ? _bcd_to_dec(data[0] & PCF8563_MINUTES_MASK)
                      : 0xFF;
  alarm->hour = (data[1] != PCF8563_VOL_LOW_MASK)
                    ? _bcd_to_dec(data[1] & PCF8563_HOUR_MASK)
                    : 0xFF;
  alarm->day = (data[2] != PCF8563_VOL_LOW_MASK)
                   ? _bcd_to_dec(data[2] & PCF8563_DAY_MASK)
                   : 0xFF;
  alarm->weekday = (data[3] != PCF8563_VOL_LOW_MASK)
                       ? _bcd_to_dec(data[3] & PCF8563_WEEKDAY_MASK)
                       : 0xFF;
  return ESP_OK;
}

esp_err_t pcf8563_enable_alarm() {
  uint8_t data;
  ESP_ERROR_CHECK(_read_register(PCF8563_STAT2_REG, &data, 1));
  data |= (1 << 1);
  return _write_register(PCF8563_STAT2_REG, &data, 1);
}

esp_err_t pcf8563_disable_alarm() {
  uint8_t data;
  ESP_ERROR_CHECK(_read_register(PCF8563_STAT2_REG, &data, 1));
  data &= ~(1 << 1);
  return _write_register(PCF8563_STAT2_REG, &data, 1);
}

esp_err_t pcf8563_check_voltage_low(bool *voltage_low) {
  uint8_t data;
  ESP_ERROR_CHECK(_read_register(PCF8563_SEC_REG, &data, 1));
  *voltage_low = data & PCF8563_VOL_LOW_MASK;
  return ESP_OK;
}