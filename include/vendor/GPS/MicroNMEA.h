/*
 * SPDX-FileCopyrightText: 2015-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include "driver/uart.h"
#include "esp_err.h"
#include "esp_event.h"
#include "esp_types.h"

#define GPS_MAX_SATELLITES_IN_USE (12)
#define GPS_MAX_SATELLITES_IN_VIEW (16)

#define GPS_EPOCH_YEAR 2000 // GPS dates are relative to year 2000
#define GPS_MIN_YEAR 0 // Minimum valid year offset (2000)
#define GPS_MAX_YEAR 99 // Maximum valid year offset (2099)
#define NMEA_MAX_STATEMENT_ITEM_LENGTH (16)

/**
 * @brief Declare of NMEA Parser Event base
 *
 */
ESP_EVENT_DECLARE_BASE(ESP_NMEA_EVENT);

/**
 * @brief GPS fix type
 *
 */
typedef enum {
  GPS_FIX_INVALID, /*!< Not fixed */
  GPS_FIX_GPS,     /*!< GPS */
  GPS_FIX_DGPS,    /*!< Differential GPS */
} gps_fix_t;

/**
 * @brief GPS fix mode
 *
 */
typedef enum {
  GPS_MODE_INVALID = 1, /*!< Not fixed */
  GPS_MODE_2D,          /*!< 2D GPS */
  GPS_MODE_3D           /*!< 3D GPS */
} gps_fix_mode_t;

/**
 * @brief GPS satellite information
 *
 */
typedef struct {
  uint8_t num;       /*!< Satellite number */
  uint8_t elevation; /*!< Satellite elevation */
  uint16_t azimuth;  /*!< Satellite azimuth */
  uint8_t snr;       /*!< Satellite signal noise ratio */
} gps_satellite_t;

/**
 * @brief GPS time
 *
 */
typedef struct {
  uint8_t hour;      /*!< Hour */
  uint8_t minute;    /*!< Minute */
  uint8_t second;    /*!< Second */
  uint16_t thousand; /*!< Thousand */
} gps_time_t;

/**
 * @brief GPS date
 *
 */
typedef struct {
  uint8_t day;   /*!< Day (start from 1) */
  uint8_t month; /*!< Month (start from 1) */
  uint16_t year; /*!< Year (start from 2000) */
} gps_date_t;

/**
 * @brief NMEA Statement
 *
 */
typedef enum {
  STATEMENT_UNKNOWN = 0, /*!< Unknown statement */
  STATEMENT_GGA,         /*!< GGA */
  STATEMENT_GSA,         /*!< GSA */
  STATEMENT_RMC,         /*!< RMC */
  STATEMENT_GSV,         /*!< GSV */
  STATEMENT_GLL,         /*!< GLL */
  STATEMENT_VTG          /*!< VTG */
} nmea_statement_t;

/**
 * @brief GPS object
 */
typedef struct {
  float latitude;          /*!< Latitude (degrees) */
  float longitude;         /*!< Longitude (degrees) */
  float altitude;          /*!< Altitude (meters) */
  gps_fix_t fix;           /*!< Fix status */
  uint8_t sats_in_use;     /*!< Number of satellites in use */
  gps_time_t tim;          /*!< time in UTC */
  gps_fix_mode_t fix_mode; /*!< Fix mode */
  uint8_t sats_id_in_use[GPS_MAX_SATELLITES_IN_USE]; /*!< ID list of satellite
                                                        in use */
  float dop_h;          /*!< Horizontal dilution of precision */
  float dop_p;          /*!< Position dilution of precision  */
  float dop_v;          /*!< Vertical dilution of precision  */
  uint8_t sats_in_view; /*!< Number of satellites in view */
  gps_satellite_t
      sats_desc_in_view[GPS_MAX_SATELLITES_IN_VIEW]; /*!< Information of
                                                        satellites in view */
  gps_date_t date;                                   /*!< Fix date */
  bool valid;                                        /*!< GPS validity */
  float speed;     /*!< Ground speed, unit: m/s */
  float cog;       /*!< Course over ground */
  float variation; /*!< Magnetic variation */
} gps_t;

/**
 * @brief GPS parser library runtime structure
 */
typedef struct {
  uint8_t item_pos;         /*!< Current position in item */
  uint8_t item_num;         /*!< Current item number */
  uint8_t asterisk;         /*!< Asterisk detected flag */
  uint8_t crc;              /*!< Calculated CRC value */
  uint8_t parsed_statement; /*!< OR'd of statements that have been parsed */
  uint8_t sat_num;          /*!< Satellite number */
  uint8_t sat_count;        /*!< Satellite count */
  uint8_t cur_statement;    /*!< Current statement ID */
  uint32_t all_statements;  /*!< All statements mask */
  char item_str[NMEA_MAX_STATEMENT_ITEM_LENGTH]; /*!< Current item */
  gps_t parent;                                  /*!< Parent class */
  uart_port_t uart_port;                         /*!< Uart port number */
  uint8_t *buffer;                               /*!< Runtime buffer */
  esp_event_loop_handle_t event_loop_hdl;        /*!< Event loop handle */
  TaskHandle_t tsk_hdl;                          /*!< NMEA Parser task handle */
  QueueHandle_t event_queue;                     /*!< UART event queue handle */
} esp_gps_t;

/**
 * @brief Configuration of NMEA Parser
 *
 */
typedef struct {
  struct {
    uart_port_t uart_port;        /*!< UART port number */
    uint32_t rx_pin;              /*!< UART Rx Pin number */
    uint32_t baud_rate;           /*!< UART baud rate */
    uart_word_length_t data_bits; /*!< UART data bits length */
    uart_parity_t parity;         /*!< UART parity */
    uart_stop_bits_t stop_bits;   /*!< UART stop bits length */
    uint32_t event_queue_size;    /*!< UART event queue size */
  } uart;                         /*!< UART specific configuration */
} nmea_parser_config_t;

/**
 * @brief NMEA Parser Handle
 *
 */
typedef void *nmea_parser_handle_t;

/**
 * @brief Default configuration for NMEA Parser
 *
 */

#ifdef CONFIG_GPS_UART_RX_PIN

#define NMEA_PARSER_CONFIG_DEFAULT()                                           \
  {                                                                            \
    .uart = {                                                                  \
      .uart_port = UART_NUM_1,                                                 \
      .rx_pin = CONFIG_GPS_UART_RX_PIN,                                        \
      .baud_rate = 9600,                                                       \
      .data_bits = UART_DATA_8_BITS,                                           \
      .parity = UART_PARITY_DISABLE,                                           \
      .stop_bits = UART_STOP_BITS_1,                                           \
      .event_queue_size = 16                                                   \
    }                                                                          \
  }
#else
#define NMEA_PARSER_CONFIG_DEFAULT()                                           \
  {                                                                            \
    .uart = {                                                                  \
      .uart_port = UART_NUM_1,                                                 \
      .rx_pin = 1,                                                             \
      .baud_rate = 9600,                                                       \
      .data_bits = UART_DATA_8_BITS,                                           \
      .parity = UART_PARITY_DISABLE,                                           \
      .stop_bits = UART_STOP_BITS_1,                                           \
      .event_queue_size = 16                                                   \
    }                                                                          \
  }
#endif
/**
 * @brief NMEA Parser Event ID
 *
 */
typedef enum {
  GPS_UPDATE, /*!< GPS information has been updated */
  GPS_UNKNOWN /*!< Unknown statements detected */
} nmea_event_id_t;

/**
 * @brief Init NMEA Parser
 *
 * @param config Configuration of NMEA Parser
 * @return nmea_parser_handle_t handle of NMEA parser
 */
nmea_parser_handle_t nmea_parser_init(const nmea_parser_config_t *config);

/**
 * @brief Deinit NMEA Parser
 *
 * @param nmea_hdl handle of NMEA parser
 * @return esp_err_t ESP_OK on success, ESP_FAIL on error
 */
esp_err_t nmea_parser_deinit(nmea_parser_handle_t nmea_hdl);

/**
 * @brief Add user defined handler for NMEA parser
 *
 * @param nmea_hdl handle of NMEA parser
 * @param event_handler user defined event handler
 * @param handler_args handler specific arguments
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_ERR_NO_MEM: Cannot allocate memory for the handler
 *  - ESP_ERR_INVALIG_ARG: Invalid combination of event base and event id
 *  - Others: Fail
 */
esp_err_t nmea_parser_add_handler(nmea_parser_handle_t nmea_hdl,
                                  esp_event_handler_t event_handler,
                                  void *handler_args);

/**
 * @brief Remove user defined handler for NMEA parser
 *
 * @param nmea_hdl handle of NMEA parser
 * @param event_handler user defined event handler
 * @return esp_err_t
 *  - ESP_OK: Success
 *  - ESP_ERR_INVALIG_ARG: Invalid combination of event base and event id
 *  - Others: Fail
 */
esp_err_t nmea_parser_remove_handler(nmea_parser_handle_t nmea_hdl,
                                     esp_event_handler_t event_handler);

// Helper functions
static inline uint16_t gps_get_absolute_year(uint16_t year_offset) {
  return GPS_EPOCH_YEAR + year_offset;
}

static inline bool gps_is_valid_year(uint16_t year_offset) {
  return (year_offset <= GPS_MAX_YEAR);
}

#ifdef __cplusplus
}
#endif