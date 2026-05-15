#include <SoftwareSerial.h>
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include <Firebase_ESP_Client.h>
#include "RTClib.h"
//#include <addons/TokenHelper.h>
//#include <addons/RTDBHelper.h>


#define debug true
#define _TIMERINTERRUPT_LOGLEVEL_     1
#define USING_TIM_DIV1                false           // for shortest and most accurate timer
#define USING_TIM_DIV16               false           // for medium time and medium accurate timer
#define USING_TIM_DIV256              true            // for longest timer  but least accurate. Default
#include "ESP8266TimerInterrupt.h"
#define TIMER_INTERVAL_MS       50
ESP8266Timer ITimer;
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#define ssn 1    //s and p start 
#define esn 8    //s and p sto p
#define based true  //whether it should consider light status
RTC_DS3231 rtc;
const int sizet = (((esn - ssn) + 1) * 2);
const int sizeton2 = ((esn - ssn) + 1);
const int ref = (ssn - 1);
SoftwareSerial EspSerial(4, 5);
#define WIFI_SSID "SmItch"        //"SmItch"        // Your WiFi SSID
#define WIFI_PASSWORD "SmItch123"  //"SmItch123"    // Your WiFi Password
#define API_KEY "INSERT_YOUR_KEY"
#define DATABASE_URL "INSERT_YOUR_URL"
#define FIREBASE_PROJECT_ID "INSERT_YOUR_PROJECT_ID"  // Your Firebase Project ID. Can be found in project settings.
#define USER_EMAIL "insert_access_email"
#define USER_PASSWORD "insert_access_password"
#define PRODUCT_ID "input_specific_product_id" //production number 001
#define setcalltime 5000
#define data_address 10
#define Wifi_time_out 5000
const int red_led = 13;
const int green_led = 12;
FirebaseData stream1, stream2;
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
String slot_ID;
String serialdata;
int port;
volatile bool slotchanged = false;
bool scheduled_complete1 = false, scheduled_complete2 = false, enabled_complete1 = false, enabled_complete2 = false;
struct store {
  char wifi_ssid[50];
  char wifi_password[50];
  char document_id[40];
  char password[21];
  bool wroteoff;
  bool wroteon;
  bool wroteemergency;
  bool ok;
} gdata;
struct sdufhdjk {
  bool online;
  bool failed_to_upload;
  bool old_failed_to_upload;
  bool datachanged;
  byte field;
  char actions[25];
  char enableds[25];
  char schedules[25];
  char slot_id[4];
  bool datachanged2;
  byte field2;
  unsigned long starthh;
  unsigned long stophh;
  bool days[8];
  bool updated;
  bool updating;
  byte updatepos;
  bool initialized;
  bool enabledchanged;
  bool type;
  bool schedule_enabled;
  bool failed;
  bool network;
  bool updateschedule;
  int buzzerlength;
} readings;
struct smart {  //interaction
  byte index;
  byte lamp;  //lamp/sucket
  byte dstate;
  bool enable;
  byte starthh;
  byte stophh;
  byte startmm;
  byte stopmm;
  bool days[8];
  bool type;
  bool error;
  bool changed;
  bool schedule_enabled;
} devices[sizet + 1];
struct aisdfui {
  bool updated;
  bool started;
} enabled;
struct sdfkn {
  byte port;
  bool updated;
  bool state;
  bool started;
} scheduled;
char tosend[40];
String sending, DATAPATH, DATAPATH2, subscribed, mydata;
bool flag = false, timeupdated, ledstate, first;
unsigned long startuploadtime, startsending, timming, startupdatetime, timingit, redtime, greentime;
unsigned long connect_time, starttime, st, thetime;
int count = 1, i;
bool complete = false, redosubscription = false, green = false, redstate, bluestate, firsttim = true;
volatile bool greenstate = false;
volatile byte numbercount = 1;
bool ongreen, availablee = false, oavailablee = false, enableled, firsthandle = true;
bool emergency, electricityon, electricityoff, streamed, firsttime, resetit = false, reseting;
unsigned long emergencytime, streamtime, lastsend, resetingtime = 0;
unsigned long ontime, offtime, timepass, startcheck;
unsigned long epochTime;

void IRAM_ATTR TimerHandler() {
  if ((ongreen) && (enableled)) {
    numbercount++;
    if (numbercount >= 60) {
      digitalWrite(green_led, HIGH);  //Toggle LED Pin
      numbercount = 1;
      greenstate = true;
    } else {
      if (greenstate == true) {
        digitalWrite(green_led, LOW);
        greenstate = false;
      }
    }
  }
}

bool askdata() {
  const char requsetslist[5][10] = {"***ok#", "***wssid#",  "***wpasw#",  "***docid#", "***pass#"};
  int i = 0;
  bool found;
  String data;
  char ch;
  while (i <= 4) {
    found = false;
    sending = String(requsetslist[i]);
    while (sending.length() > 0) {
      EspSerial.print(sending[0]);
      sending.remove(0, 1);
      delay(1);
    }
    redtime = millis();
    while (millis() - redtime <= 5000) {
      yield();
      //readarduino();
      ch = EspSerial.read();
      if (ch == '*') {
        data = "";
        while ((ch != '#') && (millis() - redtime <= 5000)) {
          yield();
          if (EspSerial.available()) {
            ch = EspSerial.read();
            data += String(ch);
          }
        }
        while (data[0] == '*') data = data.substring(1, data.length() + 1);
        data = data.substring(data.indexOf(',') + 1, data.length() + 1);
        data = data.substring(0, data.indexOf(','));
        if (i == 0) {//if stored
          gdata.ok = (data.toInt() == 1) ? true : false;
          if (gdata.ok == false) return true;
        } else if (i == 1) { //wifi ssid
          data.toCharArray(gdata.wifi_ssid, data.length() + 1);
        } else if (i == 2) { //wifi pass
          data.toCharArray(gdata.wifi_password, data.length() + 1);
        } else if (i == 3) { // doc_id
          data.toCharArray(gdata.document_id, data.length() + 1);
        } else if (i == 4) { //password
          data.toCharArray(gdata.password, data.length() + 1);
        }
        found = true;
        break;
      }
    }
    if (found == false) return false;
    i++;
  }
  return true;
}

void setup() {
  subscribed = "P" + String(ssn);
  readings.updated = false;
  readings.updating = false;
  Serial.begin(9600);
  EspSerial.begin(9600);
  pinMode(red_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  lastsend = millis();
  firsttime = true;
  EEPROM.begin(50);
  availablee = false;
  enableled = false;
  redtime = 0;
  redstate = false;
  redtime = millis();
  readings.network = EEPROM.read(1);
  delay(500);
  while (!askdata()) {
    delay(1000);
  }
  delay(500);
  sprintf(tosend, "%c%02d%d%dZ", 'P', 1, 5, 1);
  sending = String(tosend);
  while (sending.length() > 0) {
    EspSerial.print(sending[0]);
    sending.remove(0, 1);
  }
  //Serial.print("ok again: "); Serial.println(gdata.ok);
  if (gdata.ok) { //data has been stored
    //Serial.println("have data already");
    // get_wifi_credentials();
    if (initWiFi(String(gdata.wifi_ssid), String(gdata.wifi_password), 1) == false) tryagain();
  } else {  //data has not been store
    //Serial.println("have data already");
    while (!initWiFi(WIFI_SSID, WIFI_PASSWORD, 0));
    if (!initWiFi(String(gdata.wifi_ssid), String(gdata.wifi_password), 1)) tryagain();
  }
  // if(debug) Serial.println("continueing");
#if defined(ESP8266)
  stream1.setBSSLBufferSize(2048 /* Rx in bytes, 512 - 16384 */, 512 /* Tx in bytes, 512 - 16384 */);
  stream2.setBSSLBufferSize(2048 /* Rx in bytes, 512 - 16384 */, 512 /* Tx in bytes, 512 - 16384 */);
#endif
  if (redosubscription == false) {
    configure();
  }
  subscribetostream();
  streamtime = millis();
  DATAPATH = String(F("slots/")) + String(gdata.document_id) + String("/");
  DATAPATH2 = String(F("slots_arduino/")) + String(gdata.document_id);
  starttime = millis();
  timming = millis();
  st = millis();  //delet after
  updatetime(2);
  timeupdated = updatetime(1);
  readings.failed_to_upload = false;
  readings.old_failed_to_upload = false;
  first =  true;
  streamed = false;
  timingit = millis();
  ITimer.attachInterruptInterval(TIMER_INTERVAL_MS * 1000, TimerHandler);
  streamtime = millis();
  firsttime = true;
}

void loop() {
  readserial();
  if ((reseting == true) && (millis() - resetingtime >= 500)) {
    resetall();
    reseting = false;
  }
  if (millis() - startsending < 0) {
    startuploadtime = startsending = timming = startupdatetime = timingit = redtime = greentime = millis();
    connect_time = starttime = st = thetime = startcheck = streamtime = lastsend = millis();
  }
  if ((millis() - lastsend >= 300000) || (firsttime)) {
    if (updatetime(2)) lastsend = millis();
  }
  if ((millis() - streamtime >= 120000) && (streamed == false)) {
    //Serial.println("resubscribing...");
    subscribetostream();
    streamtime = millis();
    bluestate = true;
    ongreen = false;
    greenstate = false;
    digitalWrite(green_led, greenstate);
    digitalWrite(red_led, bluestate);
  }
  if (availablee == true) {
    if (complete == true) {
      if ((readings.updated == false) && (readings.updating == true)) {
        if (millis() - startupdatetime >= 10) {
          readings.updated = updateit();
          startupdatetime = millis();
          if (readings.updated == true) readings.updating = false;
        }
      }
    }
  }

  if (readings.enabledchanged  == true) {
    sprintf(tosend, "%c%02d%d%dZ", 'U', 1, 1, 1);
    sending += String(tosend);
    while (sending.length() > 0) {
      EspSerial.print(sending[0]);
      sending.remove(0, 1);
    }
    readings.enabledchanged = false;
  }
  if (readings.datachanged == true) {  //for actions and enabled only
    readings.datachanged = false;
    handchanges();
  }
  readarduino();
  if (Firebase.ready()) {
    //do nothing for now but call this function
  }

  if (readings.updateschedule) {
    if (slotchanged) {
      Firebase.RTDB.beginStream(&stream2, "/slots/" + String(gdata.document_id) + "/" + subscribed);
      Firebase.RTDB.setStreamCallback(&stream2, streamCallback2, streamTimeoutCallback2);
      slotchanged = false;
    }
  }
  if  (millis() - startsending > 10) {
    if (readings.updateschedule) {
      if ((electricityon == true) || (electricityoff == true) || (emergency == true)) {
        if (based == true) {
          if ((electricityon == true) && (ontime > 0)) {
            sendnotification(1, 0);
          }
          if ((electricityoff == true) && (offtime > 0)) {
            sendnotification(0, 0);
          }
          if ((emergency) && (emergencytime > 0)) {
            sendnotification(emergency, 1);
          }
        }
      }
    }
    if (ongreen == false) {
      if (readings.updateschedule == true) {
        if (bluestate == false) {
          bluestate = true;
          greenstate = false;
          if (enableled) {
            digitalWrite(red_led, bluestate);
            digitalWrite(green_led, LOW);
          }
        }
      }
    } else {
      if (bluestate == true) {
        bluestate = false;
        digitalWrite(red_led, bluestate);
      }
    }
    if (sending.length() > 0) {
      EspSerial.print(sending[0]);
      sending.remove(0, 1);
    }
    startsending = millis();
  }

  if ((readings.failed_to_upload == true) || (readings.failed == true) || (millis() - startcheck >= 5000)) {
    if (readings.failed_to_upload == true) readings.failed_to_upload = false;
    if (WiFi.status() != WL_CONNECTED) {  //lost internet connection
      tryagain();
    } else {
      if (readings.failed_to_upload == true) {
        ongreen = false;
      }
    }
    startcheck = millis();
    if (readings.failed == true) readings.failed = false;
  }
  if ((timeupdated == false) && (millis() - timming >= 5000)) {
    if ((readings.updating == false) && (readings.updateschedule == false)) {
      timming = millis();
    }
  }
  if ((millis() - thetime >= 30000) && (readings.initialized == true)) {
    thetime = millis();
  }
}
void configure() {
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}
void subscribetostream() {
  Firebase.RTDB.beginStream(&stream1, "custom_path" + String(gdata.document_id));
  Firebase.RTDB.setStreamCallback(&stream1, streamCallback1, streamTimeoutCallback1);
  //Firebase.RTDB.beginStream(&stream2, "/slots/" + String(gdata.document_id) + "/P" + String(ssn));
  //Firebase.RTDB.setStreamCallback(&stream2, streamCallback2, streamTimeoutCallback2);
}
bool updateit() {
  if (readings.updatepos == (sizeton2 + 1)) {
    readings.initialized = true;
    if (availablee == true) {
      electricityon = true;
      updatetime(3);
      ontime = epochTime;
      sendnotification(1, 0);
    } else {
      electricityoff = true;
      updatetime(3);
      offtime = epochTime;
      sendnotification(0, 0);
    }
    if (readings.updateschedule == false) {
      ongreen = false;
      greenstate = true;
      digitalWrite(green_led, greenstate);
    } else {
      ongreen = true;
    }
    first = true;
    resetactions();
    return true;
  }
  if (first == true) {
    first = false;
    if (devices[readings.updatepos].changed == true)
      sendata("P" + String(ref + readings.updatepos), devices[readings.updatepos].dstate, false, readings.updatepos);
  } else if (first == false) {
    if (devices[readings.updatepos + sizeton2].changed == true)
      sendata("S" + String(ref + readings.updatepos), devices[readings.updatepos + sizeton2].dstate,
              false, readings.updatepos + sizeton2);
    readings.updatepos++;
    first = true;
  }
  return false;
}
void tryagain() {
  streamed = false;
  streamtime = millis();
  while (1) {
    readarduino();
    readserial();
    if (initWiFi(WIFI_SSID, WIFI_PASSWORD, 0) == true) {
      if (initWiFi(String(gdata.wifi_ssid), String(gdata.wifi_password), 1) == true) break;
    }
    if (initWiFi(String(gdata.wifi_ssid), String(gdata.wifi_password), 1) == true) break;
  }
  //configure();
  subscribetostream();
}
boolean updatetime(byte sendit) {  // NTP Client to get time
  if (gettime1(sendit) == false) {
    return gettime2(sendit);
  } else return true;
}
bool gettime1(byte sendit) {
  WiFiUDP ntpUDP;
  NTPClient timeClient(ntpUDP, "pool.ntp.org");
  timeClient.begin();              // initializing NTP to get time
  timeClient.setTimeOffset(3600);  //IST is UTC+1
  if (timeClient.update() == false) return false;
  epochTime = timeClient.getEpochTime();
  return sendepoch(sendit);
}
bool gettime2(byte sendit) {
  unsigned int localPort = 2390;
  IPAddress timeServerIP;
  const char* ntpServerName = "time.nist.gov";
  const int PACKET_SIZE = 48;
  byte packetBuffer[PACKET_SIZE];
  WiFiUDP udp;
  udp.begin(localPort);
  WiFi.hostByName(ntpServerName, timeServerIP);
  memset(packetBuffer, 0, PACKET_SIZE);
  packetBuffer[0] = 0b11100011;
  packetBuffer[1] = 0;
  packetBuffer[2] = 6;
  packetBuffer[3] = 0xEC;
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;
  udp.beginPacket(timeServerIP, 123); //NTP requests are to port 123
  udp.write(packetBuffer, PACKET_SIZE);
  udp.endPacket();
  delay(1000);
  int cb = udp.parsePacket();
  if (!cb) return false;
  udp.read(packetBuffer, PACKET_SIZE);
  unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
  unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
  unsigned long secsSince1900 = highWord << 16 | lowWord;
  const unsigned long seventyYears = 2208988800UL;
  epochTime = secsSince1900 - seventyYears + 3600;
  return sendepoch(sendit);
}
bool sendepoch(byte sendit) {
  int hh, mm, ss, dd, mn, yy;
  DateTime now = epochTime;
  yy = now.year();
  mn = now.month();
  dd = now.day();
  hh = now.hour();
  mm = now.minute();
  ss = now.second();
  if (sendit == 1) {
    sprintf(tosend, "specific_placeholder", 'S', 1, 4, dd, mn, yy, hh, mm, ss);
    sending += String(tosend);
    while (sending.length() > 0) {
      EspSerial.print(sending[0]);
      sending.remove(0, 1);
    }
  } else if (sendit == 2) {
    //Serial.println("in 2");
    if (WiFi.status() != WL_CONNECTED) {
      tryagain();
      return false;
    }
    String dataPath = "other_data/" + String(gdata.document_id) + String(F("/last_time"));
    //Serial.print("to: "); Serial.println(dataPath);
    readings.failed_to_upload = !(Firebase.RTDB.setInt(&fbdo, dataPath, epochTime));
    if (readings.failed_to_upload) {
      return false;
    } else firsttime = false;
  }
  return true;
}
void runred() {
  if ((bluestate == true) || (greenstate == true)) {
    bluestate = false;
    greenstate = false;
    digitalWrite(green_led, bluestate);
  }
  if (millis() - redtime >= 500) {
    redstate = !redstate;
    if (enableled) digitalWrite(red_led, redstate);
    redtime = millis();
  }
}
bool initWiFi(String ssid, String password, int prio) {
  ongreen = false;
  runred();
  readings.online = false;
  readings.updated = false;
  readings.updating = false;
  WiFi.disconnect();
  starttime = millis();
  while (WiFi.status() == WL_CONNECTED) {
    runred();
    readarduino();
    readserial();
    if (millis() - starttime >= Wifi_time_out) {
      return false;
    }
  }
  delay(500);
  WiFi.begin(ssid, password);
  starttime = millis();
  while (WiFi.status() != WL_CONNECTED) {
    runred();
    readarduino();
    readserial();
    if (millis() - starttime >= Wifi_time_out) {
      return false;
    }
  }
  readings.online = true;
  if (redstate == true) {
    redstate = false;
    digitalWrite(red_led, redstate);
  }
  if (readings.updateschedule == false) {
    ongreen = false;
    greenstate = true;
    digitalWrite(green_led, greenstate);
  } else {
    ongreen = true;
  }
  if (prio == 1) return true;
  if (get_wifi_credentials() == true) {
    redosubscription = true;
    return true;
  } else {
    return false;
  }
}
bool get_wifi_credentials() {
  //get your wifi credential from backend or from arduino or from esp
}
void sendata(String slot, bool  data, bool error, byte port) {
  if (WiFi.status() != WL_CONNECTED) {
    tryagain();
    return;
  }
  if ((slot == "S0") || (slot == "P0")) {
    return;
  }
  if ((slot[0] == 'P') && (port > 48)) return;
  if (port > 96) return;
  if (error) { //handle error here

  } else {
    bool failed;
    String dataPath = DATAPATH + slot + String(F("/state"));
    //if (debug) Serial.print(F("path: "));
    //if (debug) Serial.println(dataPath);
    readings.failed_to_upload = !(Firebase.RTDB.setBool(&fbdo, dataPath, data));
    //readings.failed_to_upload = false;
    if (port == 0) {
      if (readings.failed_to_upload == false) {
        oavailablee = availablee;
      }
    }
    if (readings.failed_to_upload == false) {
      if ((readings.updateschedule) || (port == sizeton2)) {
        if (readings.network == false) {
          readings.network = true;
          EEPROM.write(1, readings.network);
          EEPROM.commit();
        }
      }
      devices[port].changed = false;
      if (readings.updateschedule == false) {
        ongreen = false;
        greenstate = true;
        digitalWrite(green_led, greenstate);
      } else {
        ongreen = true;
      }
    } else {
      streamed = false;
      streamtime = millis();
      devices[port].changed = true;
    }
    if ((readings.failed_to_upload == true) && (readings.old_failed_to_upload == false)) { //network just left
      ongreen = false;
      readings.old_failed_to_upload = true;
      if (readings.updateschedule) {
        if (readings.network == true) {
          readings.network = false;
          EEPROM.write(1, readings.network);
          EEPROM.commit();
        }
      }
    }
    if ((readings.failed_to_upload == false) && (readings.old_failed_to_upload == true)) {
      //there was no network, but network just came back
      if (readings.initialized == true) {
        ongreen = false;
        readings.updated = false;
        readings.updating = true;
        readings.updatepos = 1;
        readings.old_failed_to_upload = false;
      }
    }
  }
}
void resetactions() {
  if (WiFi.status() != WL_CONNECTED) {
    tryagain();
    return;
  }
  readings.failed_to_upload = !(Firebase.RTDB.setString(&fbdo, DATAPATH2 + String("/actions"), F("0")));
}
void resetall() {
  if (WiFi.status() != WL_CONNECTED) {
    tryagain();
    return;
  }
  readings.failed_to_upload = !(Firebase.RTDB.setString(&fbdo, DATAPATH2 + String("/all"), F("0")));
}
void sendnotification(bool data, int port) {
  if (WiFi.status() != WL_CONNECTED) {
    tryagain();
    return;
  }
  if (readings.updateschedule == false) return;
  bool failed[5];
  if (port == 0) {
    String dataPath = "other_data/" + String(gdata.document_id) + String(F("/is_electricity_available"));
    readings.failed_to_upload = !(Firebase.RTDB.setBool(&fbdo, dataPath, data));
    if (readings.failed_to_upload == true) {
      return;
    }
    if (data) { //light on event
      String dataPath = "notifications/" + String(gdata.document_id) + String(F("/elect_on"));
      //gdata.wroteon = false;
      failed[1] = !(Firebase.RTDB.setString(&fbdo, dataPath + String("/title"), "AVAILABLE"));
      failed[2] = !(Firebase.RTDB.setString(&fbdo, dataPath + String("/type"), "Electricity Status"));
      if ((failed[1] == false) && (failed[2] == false)) {
      } else {
        readings.failed_to_upload = true;
      }
      failed[1] = !(Firebase.RTDB.setInt(&fbdo, dataPath + String("/time"), ontime));
      failed[2] = !(Firebase.RTDB.setBool(&fbdo, dataPath + String("/is_read"), false));
      readings.failed_to_upload = failed[1] || failed[2] || readings.failed_to_upload;
      //Serial.print("offtimeI: "); Serial.println(offtime);
    } else {
      String dataPath = "notifications/" + String(gdata.document_id) + String(F("/elect_on"));
      //gdata.wroteoff = false;
      failed[1] = !(Firebase.RTDB.setString(&fbdo, dataPath + String("/title"), "UNAVAILABLE"));
      failed[2] = !(Firebase.RTDB.setString(&fbdo, dataPath + String("/type"), "Electricity Status"));
      if ((failed[1] == false) && (failed[2] == false)) {
      } else {
        readings.failed_to_upload = true;
      }
      failed[1] = !(Firebase.RTDB.setInt(&fbdo, dataPath + String("/time"), offtime));
      failed[2] = !(Firebase.RTDB.setBool(&fbdo, dataPath + String("/is_read"), false));
      readings.failed_to_upload = failed[1] || failed[2] || readings.failed_to_upload;
      //Serial.print("offtimeI: "); Serial.println(ontime);
    }
    if (data) { //alter light on
      if (readings.failed_to_upload == true) {  //it failed to upload something
        electricityon = true;
      } else {
        electricityon = false;
      }
    } else { //electricity off event
      if (readings.failed_to_upload == true) {
        electricityoff = true;
      } else {
        electricityoff = false;
      }
    }
  } else if (port == 1) {
    String dataPath = "notifications/" + String(gdata.document_id) + String(F("/emergency"));
    //gdata.wroteemergency = false;
    failed[1] = !(Firebase.RTDB.setString(&fbdo, dataPath + String("/title"), "EMERGENCY AT HOME"));
    failed[2] = !(Firebase.RTDB.setString(&fbdo, dataPath + String("/type"), "Emergency"));
    if ((failed[1] == false) && (failed[2] == false)) {
    } else {
      readings.failed_to_upload = true;
    }
    failed[1] = !(Firebase.RTDB.setInt(&fbdo, dataPath + String("/time"), emergencytime));
    failed[2] = !(Firebase.RTDB.setBool(&fbdo, dataPath + String("/is_read"), false));
    readings.failed_to_upload = failed[1] || failed[2] || readings.failed_to_upload;
    if (readings.failed_to_upload == true) {  //it failed to upload something
      emergency = true;
    } else {
      emergency = false;
    }
  }
  if (readings.failed_to_upload == false) {
    if (readings.initialized) {
      if (readings.network == false) {
        readings.network = true;
        EEPROM.write(1, readings.network);
        EEPROM.commit();
      }
    }
  } else {
    streamed = false;
    streamtime = millis();
  }
  if ((readings.failed_to_upload == true) && (readings.old_failed_to_upload == false)) { //network just left
    readings.old_failed_to_upload = true;
    if (readings.updateschedule) {
      if (readings.network == true) {
        readings.network = false;
        EEPROM.write(1, readings.network);
        EEPROM.commit();
      }
    }
  }
  //Serial.println("done and went out");
}
void readarduino() {
  bool found = false;
  if (EspSerial.available()) {
    while (EspSerial.available()) {
      char c = EspSerial.read();
      if (c != 'Z') {
        mydata += String(c);
      } else {
        found = true;
        break;
      }
    }
    if (found == false) return;
    // if(debug) Serial.print("incoming: ");
    // if(debug) Serial.println(mydata);
    String infor = mydata;
    mydata = "";
    //Serial.print("A: "); Serial.println(infor);
    while (isAlpha(infor[0]) && isAlpha(infor[1])) {
      infor.remove(0, 1);
    }
    if (alphacount(infor) != 1) return;
    //Serial.print("AA: "); Serial.println(infor);
    int port = (infor[1] - '0') * 10 + infor[2] - '0';
    if (infor[0] == 'S') port += sizeton2;
    byte command = infor[4] - '0';
    bool uploadit = infor[5] - '0';
    switch (infor[3]) {
      case '1': { //update state oline
          Serial.print(infor + 'Z');
          if (readings.network == true) {
            if (uploadit) devices[port].changed = true;
          }
          else devices[port].changed = true;
          //Serial.print("original state: "); Serial.println(devices[port].dstate);
          //Serial.print("arduino state11: "); Serial.println(command);
          if (devices[port].dstate != command) {
            //Serial.println("different");
            devices[port].dstate = command;
            if (infor[0] == 'P') { //this is P
              sprintf(tosend, "%c%d", infor[0], (port + ref));
              if (readings.updateschedule == true) sendata(String(tosend), devices[port].dstate, false, port);
            } else if (infor[0] == 'S') {
              sprintf(tosend, "%c%d", infor[0], (port + ref - sizeton2));
              if (readings.updateschedule == true) sendata(String(tosend), devices[port].dstate, false, port);
            }
            if ((port == 1) && (based == true) && (devices[port].dstate == true)) {
              emergency = true;
              updatetime(3);
              emergencytime = epochTime;
              sendnotification(1, 1);
            }
          }
          break;
        }
      case '2': {
          complete = true;
          readings.updated = false;
          readings.updating = true;
          readings.updatepos = 1;
          //Serial.println("completed");
          break;
        }
      case '3': {
          infor[3] = '4';
          Serial.print(infor + 'Z');
          availablee = command;
          enableled = availablee;
          if (redstate == true) {
            redstate = false;
            digitalWrite(red_led, redstate);
          }
          if (greenstate == true) {
            greenstate = false;
            digitalWrite(red_led, greenstate);
          }
          if (bluestate == true) {
            bluestate = false;
            digitalWrite(green_led, redstate);
            digitalWrite(red_led, redstate);
          }
          if (based) {
            if (availablee == true) {
              updatetime(3);
              ontime = epochTime;
              sendnotification(1, 0);
            } else {
              updatetime(3);
              offtime = epochTime;
              //Serial.print("offtime: "); Serial.println(offtime);
              sendnotification(0, 0);
              delay(1000);
              sprintf(tosend, "%c%02d%d%dZ", 'P', 1, 6, 1);
              sending += String(tosend);
              while (sending.length() > 0) {
                EspSerial.print(sending[0]);
                sending.remove(0, 1);
              }
            }
          }
          if (availablee == true) {
            if (readings.online == true) resetactions();
          }
          break;
        }
      case '5': {
          Serial.print(infor + 'Z');
          if ((port == sizet) && (enabled_complete2 == false)) {
            enabled_complete2 = true;
            if (enabled_complete1 == true) {
              readings.field = 2;
              handchanges();
              devices[port].enable = command;
            }
          } else {
            if (devices[port].enable != command) {
              devices[port].enable = command;
              if ((enabled.started == true) || (enabled.updated)) {
                changestate(port, devices[port].enable, 1);
              }
            }
          }
          break;
        }
      case '6': {
          Serial.print(infor + 'Z');
          if ((port == sizet) && (scheduled_complete2 == false)) {
            scheduled_complete2 = true;
            devices[port].schedule_enabled = command;
            if (scheduled_complete1 == true) {
              //Serial.println("enabled complete");
              readings.field = 3;
              handchanges();
            }
          } else {
            if (devices[port].schedule_enabled != command) {
              devices[port].schedule_enabled = command;
              if ((scheduled.started == true) || (scheduled.updated)) {
                changestate(port, devices[port].schedule_enabled, 2);
              }
            }
          }
          break;
        }
    }
  }
}
void readserial() {
  bool found = false;
  if (Serial.available()) {
    while (Serial.available()) {
      char c = Serial.read();
      if (c != 'Z') {
        serialdata += String(c);
      } else {
        found = true;
        break;
      }
    }
    if (found == false) return;
    serialdata += 'Z';
    sending += String(serialdata);
    while (sending.length() > 0) {
      EspSerial.print(sending[0]);
      sending.remove(0, 1);
    }
    serialdata = "";
  }
}
byte alphacount(String data) {
  byte count = 0, pos;
  for (pos = 0; pos < data.length(); pos++) if (isAlpha(data[pos])) count++;
  return count;
}
int whichcommand2(String content) {
  if (content.indexOf(F("schedule_start")) > -1) return 3;
  if (content.indexOf(F("schedule_stop")) > -1) return 4;
  if (content.indexOf(F("schedule_title")) > -1) return 2;
  if (content.indexOf(F("schedule_action")) > -1) return 5;
  if (content.indexOf(F("schedule_days")) > -1) return 6;
  if (content.indexOf(F("schedule_state")) > -1) return 7;
  return 0;
}
void streamCallback2(FirebaseStream data) {
  /*
     erial.printf("sream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
    printResult(data);
    Serial.println();
    Serial.println("end");
  */

  redosubscription = false;
  bool otherchanges = false;
  bool daychanged = false, initialpresent = false;
  String days = "";
  bool mydays[8];
  for (i = 1; i <= 7; i++) {
    mydays[i] = false;
  }
  size_t len;
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json) {
    FirebaseJson *json = data.to<FirebaseJson *>();
    len = json->iteratorBegin();
    FirebaseJson::IteratorValue value;
    String content;
    for (size_t i = 0; i < len; i++) {
      value = json->valueAt(i);
      content = value.value.c_str();
      if (value.type == FirebaseJson::JSON_OBJECT) { //others different from days
        switch (whichcommand2(value.key.c_str())) {
          case 1: {
              //initialpresent = true;
              break;
            }
          case 2: {
              initialpresent = true;
              if ((subscribed == "P1") && (based == true)) {
                readings.field2 = 3;
                readings.datachanged2 = true;
                content = content.substring(1, content.length() - 1);
                readings.buzzerlength = content.toInt();
                if (readings.buzzerlength == 0) readings.buzzerlength =  30;
              }
              break;
            }
          case 3: {
              readings.starthh = content.toInt();
              readings.field2 = 1;
              readings.datachanged2 = true;
              break;
            }
          case 4: {
              readings.stophh = content.toInt();
              readings.field2 = 2;
              readings.datachanged2 = true;
              break;
            }
          case 5: {
              if (content.indexOf("null") >= 0) {
              } else {
                //Serial.print("true value of: "); Serial.println(content);
                readings.type = (content.indexOf("true") >= 0) ? true : false;
                readings.field2 = 3;
                readings.datachanged2 = true;
              }
              break;
            }
          case 6: {
              daychanged = true;
              break;
            }
          case 7: {
              initialpresent = true;
              readings.schedule_enabled = (content.indexOf("true") >= 0) ? true : false;
              readings.field2 = 3;
              readings.datachanged2 = true;
              break;
            }
          default: otherchanges = true;
        }
      } else {  //for day of the weeks
        daychanged = true;
        content = content.substring(1, content.length() - 1);
        days += content;
      }
    }
    if (daychanged == true) {
      mydays[1] = (days.indexOf("Sun") >= 0);
      mydays[2] = (days.indexOf("Mon") >= 0);
      mydays[3] = (days.indexOf("Tue") >= 0);
      mydays[4] = (days.indexOf("Wed") >= 0);
      mydays[5] = (days.indexOf("Thu") >= 0);
      mydays[6] = (days.indexOf("Fri") >= 0);
      mydays[7] = (days.indexOf("Sat") >= 0);
      readings.datachanged2 = true;
    }
    json->iteratorEnd();
    json->clear();
  }
  if (daychanged == true) {
    for (i = 1; i <= 7; i++) readings.days[i] = mydays[i];
  }
  if ((len > 1) || (readings.field2 == 3)) {
    if ((initialpresent == true) || ((readings.datachanged2 == true) && (otherchanges == false))) {
      if ((readings.updated == false) && (readings.updating == true)) return;
      double presenttime;
      char c = subscribed[0];
      byte count = subscribed.length();
      if (count == 3) port = (subscribed[1] - '0') * 10 + subscribed[2] - '0';
      else port = subscribed[1] - '0';
      if (c == 'S') port += sizeton2;
      byte nport = port - ref;
      if (nport <= sizet) {  //determine if within range
        int starthh, startmm, stophh, stopmm;
        presenttime = readings.starthh / 3600.0;
        starthh = presenttime;
        presenttime = presenttime - int(presenttime);
        startmm = round(presenttime * 60.0);
        devices[nport].starthh = starthh;
        devices[nport].startmm = startmm;
        presenttime = (readings.stophh * 1.0) / 3600.0;
        stophh = int(presenttime);
        stopmm = round(((presenttime - stophh) * 60.0));

        devices[nport].stophh = stophh;
        devices[nport].stopmm = stopmm;
        for (i = 1; i < 8; i++) {
          devices[nport].days[i] = readings.days[i];
        }
        devices[nport].type = readings.type;
        devices[nport].schedule_enabled = readings.schedule_enabled;
        if ((subscribed == "P1") && (based == true)) {
          devices[nport].starthh = readings.buzzerlength;
        }
        sprintf(tosend, "%c%02d%d%02d%02d%02d%02d%d%d%d%d%d%d%d%d%dZ", (nport <= sizeton2) ? ('P') : ('S'),
                (nport <= sizeton2) ? (nport) : (nport - sizeton2), 3,
                devices[nport].starthh, devices[nport].startmm, devices[nport].stophh, devices[nport].stopmm,
                devices[nport].days[1], devices[nport].days[2], devices[nport].days[3], devices[nport].days[4],
                devices[nport].days[5], devices[nport].days[6], devices[nport].days[7], devices[nport].type,
                devices[nport].schedule_enabled);
        sending += String(tosend);
        readings.updateschedule = true;
        ongreen = true;
      }
    }
  }
}
int whichcommand1(String content) {
  if (content.indexOf(F("actions")) >= 0) return 1;
  if (content.indexOf(F("enabled")) >= 0) return 2;
  if (content.indexOf(F("changed_schedule")) >= 0) return 3;
  if (content.indexOf(F("all")) >= 0) return 4;
  if (content.indexOf(F("schedule_action")) >= 0) return 6;
  if (content.indexOf(F("schedule")) >= 0) return 5;
  return 0;
}
void streamCallback1(FirebaseStream data) {
  /*Serial.println("stream 1");
    Serial.printf("stream path, %s\nevent path, %s\ndata type, %s\nevent type, %s\n\n",
                data.streamPath().c_str(),
                data.dataPath().c_str(),
                data.dataType().c_str(),
                data.eventType().c_str());
    printResult(data); // see addons/RTDBHelper.h
    Serial.println();
    Serial.println("end");
  */
  //Serial.println("stream callback1");
  streamed = true;
  readings.updateschedule = true;
  if (readings.updateschedule == false) {
    ongreen = false;
    greenstate = true;
    digitalWrite(green_led, greenstate);
  } else {
    ongreen = true;
  }
  bool allchanged = false;
  redosubscription = false;
  int counted = 0;
  String content;
  int thecommand;
  String path = data.dataPath().c_str();
  if (data.dataTypeEnum() == fb_esp_rtdb_data_type_string) {
    //Serial.println("String file");
    content = data.to<String>();
    switch (whichcommand1(path)) {
      case 1: {
          if (content.length() > 2) {
            if (firsttim == false) {
              readings.field = 1;
              readings.datachanged = true;
            }
            resetit = true;
            content.toCharArray(readings.actions, content.length() + 1);
          }
          break;
        }
      case 2: {
          readings.field = 2;
          content.toCharArray(readings.enableds, content.length() + 1);
          //Serial.print("enables: "); Serial.println(readings.enableds);
          readings.datachanged = true;
          enabled_complete1 = true;
          break;
        }
      case 3: {
          if (content != subscribed) {
            slotchanged = true;
            subscribed = content;
          }
          break;
        }
      case 4: {
          //Serial.print("all: "); Serial.println(content);
          if (firsttim == false) {
            thecommand = content.toInt();
            allchanged = true;
          }
          reseting = true;
          resetingtime = millis();
          break;
        }
      case 5: {
          readings.field = 3;
          content.toCharArray(readings.schedules, content.length() + 1);
          readings.datachanged = true;
          scheduled_complete1 = true;
          //Serial.print("schedules: "); Serial.println(readings.schedules);
          //Serial.print("HEX:  ");
          //Serial.println(content);
          //Serial.print("schdules:  ");
          //Serial.println(hex_to_bin(readings.schedules));
          //Serial.println();
          break;
        }
      default: readings.field = 0;
    }
  } else if (data.dataTypeEnum() == fb_esp_rtdb_data_type_json) {
    //Serial.println("firebase json");
    FirebaseJson *json = data.to<FirebaseJson *>();
    size_t len = json->iteratorBegin();
    FirebaseJson::IteratorValue value;
    for (int i = 0; i < len; i++) {
      value = json->valueAt(i);
      content = value.value.c_str();
      content = content.substring(1, content.length() - 1);
      if (value.type == FirebaseJson::JSON_OBJECT) { //others different from days
        //Serial.print("key: "); Serial.println(value.key.c_str());
        switch (whichcommand1(value.key.c_str())) {
          case 1: {
              if (firsttim == false) {
                readings.field = 1;
                readings.datachanged = true;
              }
              resetit = true;
              content.toCharArray(readings.actions, content.length() + 1);
              counted++;
              break;
            }
          case 2: {
              readings.field = 2;
              content.toCharArray(readings.enableds, content.length() + 1);
              //Serial.print("enables: "); Serial.println(readings.enableds);
              readings.datachanged = true;
              enabled_complete1 = true;
              counted++;
              break;
            }
          case 3: {
              if (content != subscribed) {
                slotchanged = true;
                subscribed = content;
              }
              counted++;
              break;
            }
          case 4: { //all
              //Serial.print("all: "); Serial.println(content );
              if (firsttim == false) {
                thecommand = content.toInt();
                allchanged = true;
              }
              reseting = true;
              resetingtime = millis();
              counted++;
              break;
            }
          case 5: {
              readings.field = 3;
              content.toCharArray(readings.schedules, content.length() + 1);
              readings.datachanged = true;
              scheduled_complete1 = true;
              counted++;
              //Serial.print("schedules: "); Serial.println(readings.schedules);
              //Serial.print("HEX:  ");
              //Serial.println(content);
              //Serial.print("schduleds:  ");
              //Serial.println(hex_to_bin(readings.schedules));
              //Serial.println();
              break;
            }
        }
      }
    }
    json->iteratorEnd();
    json->clear();
    if (firsttim == true) firsttim = false;
    if (counted >= 3) {
      readings.updating = true;
      readings.updated = false;
      readings.updatepos = 1;
      readings.datachanged = true;
      readings.enabledchanged = true;
    } else {
      if (allchanged == true) {
        int command = -1;
        if (thecommand == 1) command = 0; //turn of all lights
        else if (thecommand == 2) command = 1; //turn on all lights
        else if (thecommand == 4) command = 2; //turn off all sockets
        else if (thecommand == 8) command = 3;  //turn on all sockets
        if (command >= 0) {
          sprintf(tosend, "%c%02d%d%dZ", 'P', 1, 8, command);
          sending += String(tosend);
          while (sending.length() > 0) {
            EspSerial.print(sending[0]); //uncomment after
            sending.remove(0, 1);
          }
        }
      }
    }
  }
}
void streamTimeoutCallback1(bool timeout) {
  stream1.httpConnected();
  readings.failed = true;
}
void streamTimeoutCallback2(bool timeout) {
  stream2.httpConnected();
  readings.failed = true;
}
void changestate(int port, bool state, int who) {
  String slot;
  if (port <= sizeton2) slot = "P" + String(port);
  else slot = "S" + String(port - sizeton2);
  String dataPath;
  if (who == 1) dataPath = DATAPATH + slot + String(F("/is_enabled"));
  else dataPath = DATAPATH + slot + String(F("/schedule_state"));
  readings.failed_to_upload = !(Firebase.RTDB.setBool(&fbdo, dataPath, state));
  if (readings.failed_to_upload == false) {
    if (readings.updateschedule) {
      if (readings.network == false) {
        readings.network = true;
        EEPROM.write(1, readings.network);
        EEPROM.commit();
      }
    }
    if (readings.updateschedule == false) {
      ongreen = false;
      greenstate = true;
      digitalWrite(green_led, greenstate);
    } else {
      ongreen = true;
    }
  } else {
    streamed = false;
    streamtime = millis();
  }
  if ((readings.failed_to_upload == true) && (readings.old_failed_to_upload == false)) { //network just left
    ongreen = false;
    readings.old_failed_to_upload = true;
    if (readings.updateschedule) {
      if (readings.network == true) {
        readings.network = false;
        EEPROM.write(1, readings.network);
        EEPROM.commit();
      }
    }
  }
}
void handchanges() { //for action and enable only
  //Serial.println("in handling function");
  if (readings.updateschedule == false) {
    ongreen = false;
    greenstate = true;
    digitalWrite(green_led, greenstate);
  } else {
    ongreen = true;
  }
  bool found = false;
  String extracted;
  if (readings.field == 1) { //actions
    if (availablee == false) return;
    //Serial.println("inside actions");
    bool condition;
    extracted = hex_to_bin(String(readings.actions));
    int k, pos;
    bool found = false;
    for (k = ref - 1; k < (ref + sizeton2); k++) {
      if (extracted[k] == '1') { //found action change
        found = true;
        pos = (k + 1) - ref;
        devices[pos].dstate = !devices[pos].dstate;
        sprintf(tosend, "%c%02d%d%dZ", 'P', pos, 1, devices[pos].dstate);
        sending += String(tosend);
        Serial.print(tosend);
        while (sending.length() > 0) {
          EspSerial.print(sending[0]); //uncomment after
          sending.remove(0, 1);
        }
        sprintf(tosend, "%c%d", 'P', pos + ref);
        sendata(String(tosend), devices[pos].dstate, false, pos);
        if ((port == 1) && (based == true) && (devices[port].dstate == true)) {
          emergency = true;
          updatetime(3);
          emergencytime = epochTime;
          sendnotification(1, 1);
        }
        resetactions();
        break;
      }
    }
    if (found == false) { //search in s
      for (k = (48 + ref - 1); k < (48 + ref + sizeton2); k++) {
        if (extracted[k] == '1') { //found action change
          found = true;
          pos = (k + 1) - ref - 48 + sizeton2;
          devices[pos].dstate = !devices[pos].dstate;
          sprintf(tosend, "%c%02d%d%dZ", 'S', pos - sizeton2, 1, devices[pos].dstate);
          sending += String(tosend);
          Serial.print(tosend);
          while (sending.length() > 0) {
            EspSerial.print(sending[0]); //uncomment after
            sending.remove(0, 1);
          }
          sprintf(tosend, "%c%d", 'S', pos - sizeton2 + ref);
          sendata(String(tosend), devices[pos].dstate, false, pos);
          resetactions();
          break;
        }
      }
    }
    return;
  }

  found = false;
  if ((readings.field == 2) || (firsthandle == true)) { //enable
    if (enabled_complete2 == true) {
      bool condition;
      extracted = hex_to_bin(String(readings.enableds));
      for (i = ref + 1; i <= (ref + sizeton2); i++) {
        int pos = i - ref;
        condition = (devices[pos].enable) != ((extracted[i - 1] == '0') ? false : true);
        if (condition) {
          if (enabled.updated == true) {
            found = true;
            devices[pos].enable = extracted[i - 1] - '0';
            sprintf(tosend, "%c%02d%d%dZ", 'P', pos, 2, devices[pos].enable);
            Serial.print(tosend);
            sending += String(tosend);
            break;
          } else {
            changestate(pos, devices[pos].enable, 1);
          }
        }
      }
      if (found == false) {
        for (i = (48 + ref + 1); i <= (48 + ref + sizeton2); i++) {
          int pos = i - ref - 48 + sizeton2;
          condition = devices[pos].enable != ((extracted[i - 1] == '0') ? false : true);
          if (condition) {
            if (enabled.updated == true) {
              found = true;
              devices[pos].enable = extracted[i - 1] - '0';
              sprintf(tosend, "%c%02d%d%dZ", 'S', pos, 2, devices[pos].enable);
              Serial.print(tosend);
              sending += String(tosend);
              break;
            } else {
              changestate(pos, devices[pos].enable, 1);
            }
          }
        }
      }
      if ((found == false) && (enabled.updated == false)) {
        enabled.updated = true;
      }
    }
  }

  found = false;
  if ((readings.field == 3) || (firsthandle == true)) { //enable
    //Serial.println("handing schedule enables");
    if (firsthandle) {
      //Serial.println("first time update");
    }
    //Serial.print("schedule state: "); Serial.println(scheduled.updated);
    if (scheduled_complete2 == true) {
      bool condition;
      extracted = hex_to_bin(String(readings.schedules));
      for (i = ref + 1; i <= (ref + sizeton2); i++) {
        int pos = i - ref;
        condition = (devices[pos].schedule_enabled) != ((extracted[i - 1] == '0') ? false : true);
        if (condition) {
          //Serial.println("P changes at: "); Serial.print(pos);
          if (scheduled.updated == true) {
            found = true;
            devices[pos].schedule_enabled  = extracted[i - 1] - '0';
            sprintf(tosend, "%c%02d%d%dZ", 'P', pos, 9, devices[pos].schedule_enabled);
            Serial.print(tosend);
            sending += String(tosend);
            break;
          } else {
            changestate(pos, devices[pos].schedule_enabled, 2);
          }
        }
      }
      if (found == false) {
        for (i = (48 + ref + 1); i <= (48 + ref + sizeton2); i++) {
          int pos = i - ref - 48 + sizeton2;
          condition = devices[pos].schedule_enabled != ((extracted[i - 1] == '0') ? false : true);
          if (condition) {
            //Serial.println("S changes at: "); Serial.print(pos);
            if (scheduled.updated == true) {
              found = true;
              devices[pos].schedule_enabled = extracted[i - 1] - '0';
              sprintf(tosend, "%c%02d%d%dZ", 'S', pos, 9, devices[pos].schedule_enabled);
              Serial.print(tosend);
              sending += String(tosend);
              break;
            } else {
              changestate(pos, devices[pos].schedule_enabled, 2);
            }
          }
        }
      }
      if ((found == false) && (scheduled.updated == false)) {
        //Serial.println("done with schedule update");
        scheduled.updated = true;
      }
    }
  }
  if ((firsthandle == true) && (scheduled_complete2) && (scheduled_complete2)) {
    firsthandle = false;
  }
}
String hex_to_bin(String data) {
  String thestates = "", sub;
  for (int i = 0; i <= 23; i++) {
    sub = String(chartoint(data[i]), BIN);
    while (sub.length() < 4) sub = '0' + sub;
    thestates += sub;
  }
  return thestates;
}
int chartoint(char ch) {
  if (ch <= '9') return ch - '0';
  if (ch == 'A') return 10;
  if (ch == 'B') return 11;
  if (ch == 'C') return 12;
  if (ch == 'D') return 13;
  if (ch == 'E') return 14;
  if (ch == 'F') return 15;
  else return 0;
}
