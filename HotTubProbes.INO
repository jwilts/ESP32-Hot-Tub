// Created by Jeff Wilts
// May 2023
// Simple ESP32 program to monitor hot tub and email me when temperatures drop too low.
// Having trouble with wifi reaching the hot tub so added code to have it connect to multiple potential wifi networks.
// Need to put your actual IDs for digital probes in the setup section of the code. 

#include <OneWire.h>
#include <DallasTemperature.h>
#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <ESP_Mail_Client.h>
#include <map>
#include "SettingNFO.h"

SMTPSession smtp;
Session_Config config;

void smtpCallback(SMTP_Status status);

const int SensorDataPin = 2;
OneWire oneWire(SensorDataPin);
DallasTemperature sensors(&oneWire);

const int numProbes = 3;
float temperatures_Celsius[numProbes];
float temperatures_Fahrenheit[numProbes];

std::map<String, String> probeIdToLabel;

AsyncWebServer server(80);
AsyncEventSource events("/events");

unsigned long lastTime = 0;
const unsigned long timerDelay = 5000;

bool tempAlertTriggered = false;

void getDS18B20Readings() {
  sensors.requestTemperatures();
  for (int i = 0; i < numProbes; i++) {
    temperatures_Celsius[i] = sensors.getTempCByIndex(i);
    temperatures_Fahrenheit[i] = sensors.getTempFByIndex(i);
  }
}

void sendTemperatureEvents(AsyncEventSourceClient *client) {
  for (int i = 0; i < numProbes; i++) {
    client->send(String(temperatures_Celsius[i]).c_str(), ("temperature_Celsius_" + String(i)).c_str(), millis());
    client->send(String(temperatures_Fahrenheit[i]).c_str(), ("temperature_Fahrenheit_" + String(i)).c_str(), millis());
  }
}

void TempAlert(float temperature, int probeIndex) {
  uint8_t probeAddress[8];
  sensors.getAddress(probeAddress, probeIndex);

  String probeId = "";
  for (int i = 0; i < 8; i++) {
    probeId += String(probeAddress[i], HEX);
  }

  String commonName = probeIdToLabel[probeId];

  Serial.printf("Temperature on Probe %d (%s) dropped below 25 degrees! Alert triggered. Actual temperature: %.2f\n", probeIndex, commonName.c_str(), temperature);

  if (!tempAlertTriggered) {
    SMTP_Message message;
    message.sender.name = F("HOT TUB");
    message.sender.email = AUTHOR_EMAIL;
    message.subject = F("Temperature Alert");
    message.addRecipient(F("Recipient Name"), RECIPIENT_EMAIL);
    message.text.content = ("Temperature on Probe " + String(probeIndex) + " (" + commonName + ") dropped below 25 degrees! Actual temperature: " + String(temperature) + " ºC").c_str();
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    if (!smtp.connect(&config) || !MailClient.sendMail(&smtp, &message)) {
      Serial.println("Error sending email.");
    } else {
      Serial.println("Email sent successfully.");
      tempAlertTriggered = true;
    }
  }

  String alertMessage = "Temperature on Probe " + String(probeIndex) + " (" + commonName + ") dropped below 25 degrees! Alert triggered. Actual temperature: " + String(temperature) + " ºC";
  events.send(alertMessage.c_str(), "alert-message", millis());
}

String generateProbeCards() {
  String probeCards = "";
  for (int i = 0; i < numProbes; i++) {
    uint8_t probeAddress[8];
    sensors.getAddress(probeAddress, i);

    String probeId = "";
    for (int j = 0; j < 8; j++) {
      if (probeAddress[j] < 0x10) {
        probeId += "0";
      }
      probeId += String(probeAddress[j], HEX);
    }

    String commonName = probeIdToLabel[probeId];

    probeCards += "<div class='card temperature'>";
    probeCards += "<h4><i class='fas fa-thermometer-half'></i> " + commonName + " (Probe " + String(i) + ")</h4>";
    probeCards += "<p><span class='reading'><span id='temp_celcius_" + String(i) + "'>%TEMPERATURE_C_" + String(i) + "%</span> &deg;C</span></p>";
    probeCards += "<p><span class='reading'><span id='temp_fahrenheit_" + String(i) + "'>%TEMPERATURE_F_" + String(i) + "%</span> &deg;F</span></p>";
    probeCards += "</div>";
  }

  probeCards += "<div class='card'>";
  probeCards += "<h4><i class='fas fa-id-card'></i> Probe IDs</h4>";
  probeCards += "<p>";

  for (int i = 0; i < numProbes; i++) {
    uint8_t probeAddress[8];
    sensors.getAddress(probeAddress, i);

    String probeId = "";
    for (int j = 0; j < 8; j++) {
      if (probeAddress[j] < 0x10) {
        probeId += "0";
      }
      probeId += String(probeAddress[j], HEX);
    }

    probeCards += probeId + "<br>";
  }

  probeCards += "</p></div><div class='card'><h4><i class='fas fa-wifi'></i> Connected WiFi Network</h4><p>" + WiFi.SSID() + "</p></div>";

  return probeCards;
}

String processor(const String& var) {
  getDS18B20Readings();
  if (var == "NUM_PROBES") return String(numProbes);
  for (int i = 0; i < numProbes; i++) {
    if (var == "TEMPERATURE_C_" + String(i)) return String(temperatures_Celsius[i]);
    if (var == "TEMPERATURE_F_" + String(i)) return String(temperatures_Fahrenheit[i]);
  }
  return "";
}

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <title>DS18B20 Web Server</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css">
  <link rel="icon" href="data:,">
  <style>
    html {font-family: Arial; display: inline-block; text-align: center;}
    p {  font-size: 1.2rem;}
    body {  margin: 0;}
    .topnav { overflow: hidden; background-color: #4B1D3F; color: white; font-size: 1.7rem; }
    .content { padding: 20px; }
    .card { background-color: white; box-shadow: 2px 2px 12px 1px rgba(140,140,140,.5); }
    .cards { max-width: 700px; margin: 0 auto; display: grid; grid-gap: 2rem; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); }
    .reading { font-size: 2.8rem; }
    .card.temperature { color: #0e7c7b; }
    .alert-bar { display: none; }
  </style>
</head>
<body>
  <div class="topnav">
    <h3>DS18B20 WEB SERVER</h3>
  </div>
  <div class="content">
    <div class="cards">
      %PROBE_CARDS%
    </div>
    <div id="alert-section" class="card alert-bar">
      <h4><i class='fas fa-exclamation-triangle'></i> Alert Temperature</h4>
      <p id="alert-message"></p>
    </div>
  </div>
  <script>
    if (!!window.EventSource) {
      var source = new EventSource('/events');
      source.addEventListener('open', function(e) { console.log("Events Connected"); }, false);
      source.addEventListener('error', function(e) { if (e.target.readyState != EventSource.OPEN) { console.log("Events Disconnected"); } }, false);
      source.addEventListener('message', function(e) { console.log("message", e.data); }, false);
      %EVENT_LISTENERS%
    }
  </script>
</body>
</html>)rawliteral";

String generateEventListeners() {
  String eventListeners = "";
  for (int i = 0; i < numProbes; i++) {
    eventListeners += "source.addEventListener('temperature_Celsius_" + String(i) + "', function(e) { document.getElementById('temp_celcius_" + String(i) + "').innerHTML = e.data; }, false);\n";
    eventListeners += "source.addEventListener('temperature_Fahrenheit_" + String(i) + "', function(e) { document.getElementById('temp_fahrenheit_" + String(i) + "').innerHTML = e.data; }, false);\n";
  }
  eventListeners += "source.addEventListener('alert-message', function(e) { document.getElementById('alert-message').innerHTML = e.data; document.getElementById('alert-section').style.display = e.data ? 'block' : 'none'; }, false);\n";
  return eventListeners;
}

void sendWelcomeEmail() {
  SMTP_Message message;
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  message.sender.name = F("HOT TUB");
  message.sender.email = AUTHOR_EMAIL;
  message.subject = F("Hot Tub Monitor Started");
  message.addRecipient(F("Jeff"), RECIPIENT_EMAIL);
  message.text.content = "Hello World! - Sent from ESP board";
  message.text.charSet = "us-ascii";
  message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  if (!smtp.connect(&config) || !MailClient.sendMail(&smtp, &message)) {
    Serial.printf("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
  }
}

void smtpCallback(SMTP_Status status) {
  Serial.println(status.info());
  if (status.success()) {
    Serial.printf("Message sent success: %d\n", status.completedCount());
    Serial.printf("Message sent failed: %d\n", status.failedCount());
    for (size_t i = 0; i < smtp.sendingResult.size(); i++) {
      SMTP_Result result = smtp.sendingResult.getItem(i);
      Serial.printf("Message No: %d\nStatus: %s\nDate/Time: %s\nRecipient: %s\nSubject: %s\n", i + 1, result.completed ? "success" : "failed", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str(), result.recipients.c_str(), result.subject.c_str());
    }
    smtp.sendingResult.clear();
  }
}

void setup() {
  Serial.begin(115200);
  probeIdToLabel["28077F05141901CC"] = "Air";
  probeIdToLabel["28FF9A03b2170584"] = "Water";
  probeIdToLabel["28FFDFDFB21704F2"] = "Enclosure";

  for (int i = 0; i < numWiFiNetworks; i++) {
    Serial.printf("Connecting to WiFi network: %s\n", wifiNetworks[i][0]);
    WiFi.setTxPower(WIFI_POWER_19_5dBm);
    WiFi.begin(wifiNetworks[i][0], wifiNetworks[i][1]);
    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED && millis() - startTime < 10000) {
      delay(500);
      Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("\nConnected to WiFi!");
      Serial.print("Station IP Address: ");
      Serial.println(WiFi.localIP());
      MailClient.networkReconnect(true);
      smtp.debug(1);
      smtp.callback(smtpCallback);
      sendWelcomeEmail();
      break;
    } else {
      Serial.println("\nConnection failed. Trying next WiFi network...");
      WiFi.disconnect();
      delay(5000);
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to any WiFi network. Please check your credentials.");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    String indexWithProbeCards = String(index_html);
    indexWithProbeCards.replace("%PROBE_CARDS%", generateProbeCards());
    indexWithProbeCards.replace("%EVENT_LISTENERS%", generateEventListeners());
    request->send(200, "text/html", indexWithProbeCards);
  });

  events.onConnect([](AsyncEventSourceClient *client){
    if(client->lastId()){
      Serial.printf("Client reconnected! Last message ID that it got is: %u\n", client->lastId());
    }
    client->send("hello!", NULL, millis(), 10000);
    sendTemperatureEvents(client);
    client->send(tempAlertTriggered ? "Temperature alert triggered!" : "", "alert-message", millis());
  });
  server.addHandler(&events);
  server.begin();
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    getDS18B20Readings();
    for (int i = 0; i < numProbes; i++) {
      Serial.printf("Probe %d - Temperature = %.2f ºC\nProbe %d - Temperature = %.2f ºF\n", i, temperatures_Celsius[i], i, temperatures_Fahrenheit[i]);
      events.send(String(temperatures_Celsius[i]).c_str(), ("temperature_Celsius_" + String(i)).c_str(), millis());
      events.send(String(temperatures_Fahrenheit[i]).c_str(), ("temperature_Fahrenheit_" + String(i)).c_str(), millis());
      if (i == 1 && temperatures_Celsius[i] < 25 && !tempAlertTriggered) {
        TempAlert(temperatures_Celsius[i], i);
        tempAlertTriggered = true;
      }
    }
    lastTime = millis();
  }
}
