#pragma once
#include "WPSUtils.h"

void InitWPSBruteForce(mac_t bssid, int delay)
{
    data_t* data = new data_t();
    init(data);
    smart_bruteforce(data, bssid, NO_PIN, delay);
}