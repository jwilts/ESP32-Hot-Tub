// SettingNFO.h
// For ESP32-Hot-Tub-Probes

#ifndef SETTINGNFO_H
#define SETTINGNFO_H

// Define multiple WiFi networks
const char* wifiNetworks[][2] = {
  {"SSID1", "WififPassword"},
  {"SSID2", "WifiPassword"},
  {"SSID3", "WifiPassword"},
  // Add more networks as needed
};

const int numWiFiNetworks = sizeof(wifiNetworks) / sizeof(wifiNetworks[0]);

// PUT YOUR PROBE NAMES HERE AS WELL AS IN MAIN PROGRAM
const char* probeId_air = "28077F05141901CC";
const char* probeId_water = "28FF9A03b2170584";
const char* probeId_enclosure = "28FFDFDFB21704F2";

// Email settings
const char* AUTHOR_EMAIL = "YourApplicationEmail@gmail.com";
const char* AUTHOR_PASSWORD = "Your Application Key";
const char* RECIPIENT_EMAIL = "WhoGetsNotifed@hotmail.com";

/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* Recipient's email*/
#define RECIPIENT_EMAIL "WhoGetsNotifed@hotmail.com"


#endif // SETTINGNFO_H
