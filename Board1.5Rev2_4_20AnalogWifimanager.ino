#include "BluetoothSerial.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//BluetoothSerial SerialBT;
#define _TASK_SLEEP_ON_IDLE_RUN
#define _TASK_PRIORITY
#define _TASK_WDT_IDS
#define _TASK_TIMECRITICAL

// Include libraries this sketch will use
#include  <Wire.h>
#include  <MCP342X.h>

// Instantiate objects used in this project
MCP342X AI14(0x68);

#define WIFI_AP ""
#define WIFI_PASSWORD ""


#include <FS.h>
#include "SPIFFS.h"
#include <WiFi.h>

#include <DNSServer.h>
#include <WiFiManager.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#include <TaskScheduler.h>
#include <PubSubClient.h>


#include "RTClib.h"
#include "EEPROM.h"

#include <ArduinoOTA.h>
#include <WebServer.h>


boolean pro_int;



#define HOSTNAME "ENT"
#define PASSWORD "7650"

static int32_t  adc0;
static int32_t  adc1;
static int32_t  adc2;
static int32_t  adc3;
char char_chipID[20];
 

TaskHandle_t Task1;
String deviceToken = "preparesetting";
uint64_t chipId = 0;
String weight = "";
String rawWeight = "";
String wifiName = "";
float DP = 0.0;
unsigned long ms;
unsigned long diffMillis = 0;
TaskHandle_t Task2;
WebServer server(80);


WiFiClientSecure wifiClient;
PubSubClient client(wifiClient);

struct Settings
{
  char TOKEN[40] = "";
  char SERVER[40] = "thingcontrol.io";
  int PORT = 8883;
  int sampling = 1000;
  char MODE[60] = "v1/devices/me/telemetry";
  uint32_t ip;
} sett;




char sampling[8];

char program[10];
char mode_freq[3];

unsigned long previousMillis = 0;
const long interval = 60000;  //millisecond
unsigned long currentMillis;


unsigned long previousRollOver = 0;
unsigned long intervalRollOver = 1000000000;  //roleover();
unsigned long currentRollOver;



Scheduler runner;

float value = 0.0;
String json = "";
int countInSec = 0;

//WiFiClient wifiClient;
//PubSubClient client(wifiClient);

int status = WL_IDLE_STATUS;


int rssi = 0; ;

// # Add On
#include <TimeLib.h>
#include <ArduinoJson.h>
#include "time.h"

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600 * 7;

const int   daylightOffset_sec = 3600;

bool connectWifi = false;
StaticJsonDocument<400> doc;
bool validEpoc = false;
String dataJson = "";
unsigned long _epoch = 0;
unsigned long _messageId = 0;
WiFiManager wifiManager;
unsigned long time_s = 0;

struct tm timeinfo;




int reading = 0; // Value to be displayed
int d = 0; // Variable used for the sinewave test waveform
boolean range_error = 0;
int8_t ramp = 1;




// Pause in milliseconds between screens, change to 0 to time font rendering






//flag for saving data
bool shouldSaveConfig = false;






class IPAddressParameter : public WiFiManagerParameter
{
  public:
    IPAddressParameter(const char *id, const char *placeholder, IPAddress address)
      : WiFiManagerParameter("")
    {
      init(id, placeholder, address.toString().c_str(), 16, "", WFM_LABEL_BEFORE);
    }

    bool getValue(IPAddress &ip)
    {
      return ip.fromString(WiFiManagerParameter::getValue());
    }
};

class IntParameter : public WiFiManagerParameter
{
  public:
    IntParameter(const char *id, const char *placeholder, long value, const uint8_t length = 10)
      : WiFiManagerParameter("")
    {
      init(id, placeholder, String(value).c_str(), length, "", WFM_LABEL_BEFORE);
    }

    long getValue()
    {
      return String(WiFiManagerParameter::getValue()).toInt();
    }
};



char  char_to_byte(char c)
{
  if ((c >= '0') && (c <= '9'))
  {
    return (c - 0x30);
  }
  if ((c >= 'A') && (c <= 'F'))
  {
    return (c - 55);
  }
}

String read_String(char add)
{
  int i;
  char data[100]; //Max 100 Bytes
  int len = 0;
  unsigned char k;
  k = EEPROM.read(add);
  while (k != '\0' && len < 500) //Read until null character
  {
    k = EEPROM.read(add + len);
    data[len] = k;
    len++;
  }
  data[len] = '\0';

  return String(data);
}


void getChipID() {
  chipId = ESP.getEfuseMac(); //The chip ID is essentially its MAC address(length: 6 bytes).
  Serial.printf("ESP32ChipID=%04X", (uint16_t)(chipId >> 32)); //print High 2 bytes
  Serial.printf("%08X\n", (uint32_t)chipId); //print Low 4bytes.

}

void setupOTA()
{
  //Port defaults to 8266
  //ArduinoOTA.setPort(8266);

  //Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(uint64ToString(chipId).c_str());

  //No authentication by default
  ArduinoOTA.setPassword(PASSWORD);

  //Password can be set with it's md5 value as well
  //MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  //ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]()
  {
    Serial.println("Start Updating....");

    Serial.printf("Start Updating....Type:%s\n", (ArduinoOTA.getCommand() == U_FLASH) ? "sketch" : "filesystem");
  });

  ArduinoOTA.onEnd([]()
  {

    Serial.println("Update Complete!");

    ESP.restart();
  });

  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total)
  {
    String pro = String(progress / (total / 100)) + "%";
    int progressbar = (progress / (total / 100));
    //int progressbar = (progress / 5) % 100;
    //int pro = progress / (total / 100);



    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });

  ArduinoOTA.onError([](ota_error_t error)
  {
    Serial.printf("Error[%u]: ", error);
    String info = "Error Info:";
    switch (error)
    {
      case OTA_AUTH_ERROR:
        info += "Auth Failed";
        Serial.println("Auth Failed");
        break;

      case OTA_BEGIN_ERROR:
        info += "Begin Failed";
        Serial.println("Begin Failed");
        break;

      case OTA_CONNECT_ERROR:
        info += "Connect Failed";
        Serial.println("Connect Failed");
        break;

      case OTA_RECEIVE_ERROR:
        info += "Receive Failed";
        Serial.println("Receive Failed");
        break;

      case OTA_END_ERROR:
        info += "End Failed";
        Serial.println("End Failed");
        break;
    }


    Serial.println(info);
    ESP.restart();
  });

  ArduinoOTA.begin();
}

String uint64ToString(uint64_t input) {
  String result = "";
  uint8_t base = 10;

  do {
    char c = input % base;
    input /= base;

    if (c < 10)
      c += '0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);
  return result;
}


void setupWIFI()
{
  WiFi.setHostname(uint64ToString(chipId).c_str());

  byte count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 10)
  {
    count ++;
    delay(500);
    Serial.print(".");
  }


  if (WiFi.status() == WL_CONNECTED)
    Serial.println("Connecting...OK.");
  else
    Serial.println("Connecting...Failed");

  reconnectMqtt();
}

void reconnectMqtt()
{

  Serial.print("Token:");
  Serial.println(deviceToken);
  if (client.connect(char_chipID, deviceToken.c_str(), NULL)) {


    Serial.println( F("Connect MQTT Success."));

  } else {
    Serial.println("Not connected");
  }
}
void writeString(char add, String data)
{
  int _size = data.length();
  int i;
  for (i = 0; i < _size; i++)
  {
    EEPROM.write(add + i, data[i]);
  }
  EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
  EEPROM.commit();
}

void _writeEEPROM(String data) {
  //Serial.print("Writing Data:");
  //Serial.println(data);
  writeString(10, data);  //Address 10 and String type data
  delay(10);
}





void configModeCallback (WiFiManager * myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
  Serial.print("saveConfigCallback:");
  Serial.println(sett.TOKEN);
}

void handleRoot() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent("<!DOCTYPE html><html lang=\"en\"><head> <meta charset=\"UTF-8\"> <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Setting</title></head><body> <div style=\"text-align: center;\"> <h1>Setting</h1><a style=\"background-color: red;border: 0;padding: 10px 20px;color: white;font-weight: 600;border-radius: 5px;\" href=\"/setting\">Setting</a> </div></body></html>");
  server.client().stop(); // Stop is needed because we sent no content length
}
void handleSetting() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.sendContent("<!DOCTYPE html><html lang=\"en\"><head> <meta charset=\"UTF-8\"> <meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>Setting</title></head><body> <h1>Setting</h1> <form method=\"POST\" action=\"save_setting\"> <label for=\"token\">Device Token</label> <input type=\"text\" name=\"token\" ");
  if (strcmp(sett.TOKEN, "") != 0) {
    server.sendContent("value=\"" + String(sett.TOKEN) + "\"");
  }
  server.sendContent("><br><label for=\"server\">Server</label><input type=\"text\" name=\"server\" ");
  if (strcmp(sett.SERVER, "") != 0) {
    server.sendContent("value=\"" + String(sett.SERVER) + "\"");
  }
  server.sendContent("><br><label for=\"port\">Port</label><input type=\"text\" name=\"port\" ");
  if (sett.PORT != 0) {
    server.sendContent("value=\"" + String(sett.PORT) + "\"");
  }
  server.sendContent("><br><label for=\"sampling\">Sampling Rate (ms)</label><input type=\"text\" name=\"sampling\" ");
  if (sett.sampling != 0) {
    server.sendContent("value=\"" + String(sett.sampling) + "\"");
  }
  //  server.sendContent("><br><label for=\"dpsbits\">Sampling Rate(ms)</label><input type=\"text\" name=\"dpsbits\" ");
  //  if (strcmp(dpsbits, "") != 0) {
  //    server.sendContent("value=\"" + String(dpsbits) + "\"");
  //  }
  server.sendContent("><br><label for=\"program\">Topic</label><input type=\"text\" name=\"program\" ");
  if (strcmp(program, "") != 0) {
    server.sendContent("value=\"" + String(program) + "\"");
  }
  //  server.sendContent("><br><label for=\"mode_freq\">Freq (ms)</label><input type=\"text\" name=\"mode_freq\" ");
  //  if (strcmp(mode_freq, "") != 0) {
  //    server.sendContent("value=\"" + String(mode_freq) + "\"");
  //  }
  server.sendContent("><br><input type=\"submit\" value=\"Save\"></form></body></html>");
  server.client().stop();
}
void handleSettingSave() {
  Serial.println("setting save");
  server.arg("token").toCharArray(sett.TOKEN, sizeof(sett.TOKEN));
  server.arg("server").toCharArray(sett.SERVER, sizeof(sett.SERVER));
  sett.PORT = server.arg("port").toInt();
  sett.sampling = server.arg("sampling").toInt();

  //  server.arg("dpsbits").toCharArray(dpsbits, sizeof(dpsbits));
  server.arg("program").toCharArray(program, sizeof(program));
  //  server.arg("mode_freq").toCharArray(mode_freq, sizeof(mode_freq));
  server.sendHeader("Location", "setting", true);
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 302, "text/plain", "");  // Empty content inhibits Content-length header so we have to close the socket ourselves.
  server.client().stop(); // Stop is needed because we sent no content length
  Serial.println("saving config");
  DynamicJsonDocument jsonBuffer(1024);
  jsonBuffer["token"] = sett.TOKEN;
  jsonBuffer["server"] = sett.SERVER;
  jsonBuffer["port"] = sett.PORT;
  jsonBuffer["sampling"] = sett.sampling;
  //  jsonBuffer["dpsbits"] = dpsbits;
  //  jsonBuffer["program"] = program;
  //  jsonBuffer["mode_freq"] = mode_freq;
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  serializeJson(jsonBuffer, Serial);
  serializeJson(jsonBuffer, configFile);
  configFile.close();
  String topic = String(mode_freq);
  topic.toCharArray(sett.MODE, sizeof(sett.MODE));
}
boolean isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}
/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}
boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String("Thingcontrol") + ".local")) {
    Serial.print("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

void handle_NotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  server.send(404, "text/plain", "Not found");
}

void setup(void) {
  Serial.begin(115200);
  getChipID();
  wifiName.concat("@ENT");
  wifiName.concat(uint64ToString(chipId));
  deviceToken = wifiName.c_str();

  uint64ToString(chipId).toCharArray(char_chipID, 20);
  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonDocument jsonBuffer(1024);
        deserializeJson(jsonBuffer, buf.get());
        serializeJson(jsonBuffer, Serial);
        if (!jsonBuffer.isNull()) {
          Serial.println("\nparsed json");
          //strcpy(output, json["output"]);
          if (jsonBuffer.containsKey("sampling")) sett.sampling = jsonBuffer["sampling"];
          //          if (jsonBuffer.containsKey("dpsbits")) strcpy(dpsbits, jsonBuffer["dpsbits"]);
          if (jsonBuffer.containsKey("program")) strcpy(program, jsonBuffer["program"]);
          //          if (jsonBuffer.containsKey("mode_freq")) strcpy(mode_freq, jsonBuffer["mode_freq"]);
          if (jsonBuffer.containsKey("token")) strcpy(sett.TOKEN, jsonBuffer["token"]);
          if (jsonBuffer.containsKey("server")) strcpy(sett.SERVER, jsonBuffer["server"]);
          if (jsonBuffer.containsKey("port")) sett.PORT = jsonBuffer["port"];
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }


  delay(1000);
  Serial.println("Start");
  //  SerialBT.println("Start");
  EEPROM.begin(512);




  WiFiManagerParameter sampling_param("sampling", "Sampling : (ms)", sampling, 8);
  //  WiFiManagerParameter dpsbits_param("databits", "Serial Port : Data Bits, Parity Bits, Stop Bits", dpsbits, 5);
  WiFiManagerParameter program_param("program", "Topic", program, 100);
  //  WiFiManagerParameter mode_param("mode", "Freq", mode_freq, 6);
  //  wifiManager.resetSettings();  // for clearing the setting
  wifiManager.setTimeout(120);

  wifiManager.setAPCallback(configModeCallback);
  std::vector<const char *> menu = {"wifi", "info", "sep", "restart", "exit"};
  wifiManager.setMenu(menu);
  wifiManager.setClass("invert");
  wifiManager.setConfigPortalTimeout(120); // auto close configportal after n seconds
  wifiManager.setAPClientCheck(true); // avoid timeout if client connected to softap
  wifiManager.setBreakAfterConfig(true);   // always exit configportal even if wifi save fails

  WiFiManagerParameter blnk_Text("<b>Device setup.</b> <br>");
  sett.TOKEN[39] = '\0';   //add null terminator at the end cause overflow
  sett.SERVER[39] = '\0';   //add null terminator at the end cause overflow
  WiFiManagerParameter blynk_Token( "blynktoken", "device Token",  deviceToken.c_str(), 40);
  WiFiManagerParameter blynk_Server( "blynkserver", "Server",  sett.SERVER, 40);

  IntParameter blynk_Port( "blynkport", "Port",  sett.PORT);
  IntParameter blynk_Sampling( "blynksampling", "Sampling",  sett.sampling);

  wifiManager.addParameter( &blnk_Text );
  wifiManager.addParameter( &blynk_Token );
  wifiManager.addParameter( &blynk_Server );
  wifiManager.addParameter( &blynk_Port );
  wifiManager.addParameter(&blynk_Sampling);
  //  wifiManager.addParameter(&dpsbits_param);
  wifiManager.addParameter(&program_param);
  //  wifiManager.addParameter(&mode_param);


  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);




  if (!wifiManager.autoConnect(wifiName.c_str())) {
    Serial.println("failed to connect and hit timeout");
    //    SerialBT.println("failed to connect and hit timeout");

  }

  if (sampling_param.getValue() != 0) sett.sampling =  blynk_Sampling.getValue();
  //  if (dpsbits_param.getValue() != "") strcpy(dpsbits, dpsbits_param.getValue());
  if (program_param.getValue() != "") strcpy(program, program_param.getValue());

  if (blynk_Token.getValue() != "") strcpy(sett.TOKEN, blynk_Token.getValue());
  if (blynk_Server.getValue() != "") strcpy(sett.SERVER, blynk_Server.getValue());
  if (blynk_Port.getValue() != 0) sett.PORT =  blynk_Port.getValue();
  if (blynk_Sampling.getValue() != 0) sett.sampling =  blynk_Sampling.getValue();

  Serial.println("saving config");
  DynamicJsonDocument jsonBuffer(1024);
  jsonBuffer["sampling"] = sett.sampling;
  jsonBuffer["program"] = program;

  jsonBuffer["token"] = sett.TOKEN;
  jsonBuffer["server"] = sett.SERVER;
  jsonBuffer["port"] = sett.PORT;
  File configFile = SPIFFS.open("/config.json", "w");
  if (!configFile) {
    Serial.println("failed to open config file for writing");
  }

  serializeJson(jsonBuffer, Serial);
  serializeJson(jsonBuffer, configFile);
  configFile.close();

  String topic = "v1/devices/me/telemetry";
  //  topic.toCharArray(sett.MODE, sizeof(sett.MODE));
  configTime(gmtOffset_sec, 0, ntpServer);
  client.setServer( sett.SERVER, sett.PORT );


  setupWIFI();
  setupOTA();

  if (WiFi.status() != WL_CONNECTED) {
    /* Put IP Address details */
    IPAddress local_ip(192, 168, 1, 1);
    IPAddress gateway(192, 168, 1, 1);
    IPAddress subnet(255, 255, 255, 0);

    WiFi.softAP("TB", "76507650");
    WiFi.softAPConfig(local_ip, gateway, subnet);
  }

  server.on("/", handleRoot);
  server.on("/setting", handleSetting);
  server.on("/save_setting", handleSettingSave);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  server.onNotFound ( handle_NotFound );
  server.begin();
  Serial.println("HTTP server started");
  Serial.println(WiFi.localIP());
  //  SerialBT.println("HTTP server started");
  //  SerialBT.println(WiFi.localIP());

  Serial.println("Initialized scheduler");


  if (program == "Program2") pro_int = 1;
  else pro_int = 0;
  unsigned char u;

  Wire.begin();  // join I2C bus
  //TWBR = 12;  // 400 kHz (maximum)


  Serial.println("Starting up");
  Serial.println("Testing device connection...");
  Serial.println(AI14.testConnection() ? "MCP342X connection successful 1-4" : "MCP342X connection failed 1-4");


  previousMillis = millis();
  Serial.println("Start..");
}

//====mapfloat====

float mapfloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  if ((in_max - in_min) + out_min != 0) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  } else {
    return 0;
  }
}

void loop() {

  String str = "";
  int byteCount = 0;

 

  currentMillis = millis();
  json = "";
  if (currentMillis - previousMillis  >= sett.sampling)
  {


    AI14.configure( MCP342X_MODE_CONTINUOUS |
                    MCP342X_CHANNEL_1 |
                    MCP342X_SIZE_18BIT |
                    MCP342X_GAIN_1X
                  );

    AI14.startConversion();
    AI14.getResult(&adc0);
    Serial.print("\tCh1: ");

    float val0 = mapfloat(adc0, 25286, 126635, 4, 20);
    Serial.println(val0);
    AI14.configure( MCP342X_MODE_CONTINUOUS |
                    MCP342X_CHANNEL_2 |
                    MCP342X_SIZE_18BIT |
                    MCP342X_GAIN_1X
                  );

    AI14.startConversion();
    AI14.getResult(&adc1);
    Serial.print("\tCh2: ");
    float val1 = mapfloat(adc1, 25286, 126635, 4, 20);
    Serial.println(val1);

    AI14.configure( MCP342X_MODE_CONTINUOUS |
                    MCP342X_CHANNEL_3 |
                    MCP342X_SIZE_18BIT |
                    MCP342X_GAIN_1X
                  );

    AI14.startConversion();
    AI14.getResult(&adc2);
    Serial.print("\tCh3: ");
    float val2 = mapfloat(adc2, 25286, 126635, 4, 20);
    Serial.println(val2);

    AI14.configure( MCP342X_MODE_CONTINUOUS |
                    MCP342X_CHANNEL_4 |
                    MCP342X_SIZE_18BIT |
                    MCP342X_GAIN_1X
                  );

    AI14.startConversion();
    AI14.getResult(&adc3);
    Serial.print("\tCh4: ");
    float val3 = mapfloat(adc3, 25286, 126635, 4, 20);
    Serial.println(val3);


    json.concat("{\"A14\":");
    json.concat(val0);
    json.concat(",");
    json.concat("\"A13\":");
    json.concat(val1);
    json.concat(",");
    json.concat("\"A12\":");
    json.concat(val2);
    json.concat(",");
    json.concat("\"A11\":");
    json.concat(val3);
    json.concat("}");
    Serial.print("json:");
    Serial.println(json);

//    Serial.print("sampling:");
//    Serial.println(sett.sampling);
//    Serial.print("MODE:");
//    Serial.println(sett.MODE);
    int isSend = client.publish(sett.MODE, json.c_str(), pro_int);
    Serial.println(isSend);
//    if (isSend < 1) ESP.restart();
    previousMillis = millis();

  }
  ms = millis();

  if (ms % 60000 == 0)
  {

    Serial.println("Attach WiFi for，OTA "); Serial.println(WiFi.RSSI() );
    //    SerialBT.println("Attach WiFi for，OTA "); SerialBT.println(WiFi.RSSI() );

    setupWIFI();
    setupOTA();

  }

  runner.execute();
  server.handleClient();
  ArduinoOTA.handle();
}





//void t3CallConnector() {
//  Serial.println("Attach WiFi for，OTA "); Serial.println(WiFi.RSSI() );
//
//  setupWIFI();
//  setupOTA();
//  if ( !client.connected() )
//  {
//    reconnectMqtt();
//  }
//
//}
