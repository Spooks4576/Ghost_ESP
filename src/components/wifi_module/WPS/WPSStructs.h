#pragma once
#include <Arduino.h>
#include "lwip/sockets.h"

typedef unsigned long long int mac_t;
typedef unsigned int pin_t;
#define NO_MAC -1ull
#define NO_PIN -1u
#define EMPTY_PIN -2u
#define PIN_TYPES_COUNT 30
#define BRUTEFORCE_STATISTICS_PERIOD 5
#define BRUTEFORCE_STATISTICS_COUNT 15

typedef struct {
	const char *name;
	pin_t static_pin;
	pin_t (*gen)(mac_t mac);
} algo_t;

typedef struct {
	char pke[1024];
	char pkr[1024];
	char authkey[256];
	char e_hash1[256];
	char e_hash2[256];
	char e_nonce[128];
} pixiewps_data_t;

typedef struct {
	int status;
	int last_m_message;
	char essid[256];
	char wpa_psk[256];
} connection_status_t;

typedef struct {
	time_t start_time;
	pin_t mask;
	time_t last_attempt_time;
	double attempts_times[BRUTEFORCE_STATISTICS_COUNT];
	int counter;
} bruteforce_status_t;

typedef struct{
	int s;
	sockaddr_in local;
    sockaddr_in dest;
	char tempdir[256];
	char tempcfg[256];
}wpa_ctrl_t;

typedef struct{
	const char *interface;
	char sessions_dir[256];
	char pixiewps_dir[256];
	char reports_dir[256];
	FILE *wpas;
	wpa_ctrl_t ctrl;
	connection_status_t connection_status;
	pixiewps_data_t pixiewps_data;
	bruteforce_status_t bruteforce;

	int save_result;
	int print_debug;

	int status;
}data_t;

typedef struct {
	time_t time;
	mac_t bssid;
	char essid[256];
	pin_t pin;
	char psk[256];
} network_entry_t;

enum {
	SECURITY_OPEN,
	SECURITY_WEP,
	SECURITY_WPA,
	SECURITY_WPA2,
	SECURITY_WPA_WPA2,
};

enum {
	STATUS_NO,
	STATUS_WSC_NACK,
	STATUS_WPS_FAIL,
	STATUS_GOT_PSK,
	STATUS_SCANNING,
	STATUS_AUTHENTICATING,
	STATUS_ASSOCIATING,
	STATUS_EAPOL_START
};

typedef struct {
	mac_t bssid;
	char essid[256];
	int signal;
	int security;
	int wps_locked;
	char model[256];
	char model_number[256];
	char device_name[256];
} network_info_t;

typedef struct {
	const char *interface;
	mac_t bssid;
	unsigned int pin;
	int pixie_dust;
	int pixie_force;
	int bruteforce;
	int show_pixie_smd;
	unsigned int delay;
	int write;
	int iface_down;
	int verbose;
	int mtk_fix;
	const char *vuln_list_path;
	int loop;
	int reverse_scan;
} input_t;