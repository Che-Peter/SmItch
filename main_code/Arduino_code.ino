#include <EEPROM.h>
#include "RTClib.h"

#define debug true
#define buttondebounce 200
#define updatestarttime 5000

RTC_DS3231 rtc;

#define based true  //true is it is the first or alone
byte sizet = 16, sizeton2 = 8;

struct smarttime {
  byte hh;
  byte mm;
  byte ss;
  byte dd;
  byte mn;
  int yy;
  int weekday;
} thetime;


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


struct smartstore {  //to store
  byte dstate;
  bool enable;
  byte starthh;
  byte stophh;
  byte startmm;
  byte stopmm;
  bool days[8];
  byte compound;
} devices[33];

struct asdfsodhf {
  int yy;
  int s1pos;
} where;

struct splitting {
  bool bstate;
  bool inschedule;
  bool type;
  bool schedule_enabled;
};

bool state[33];

bool scheduled[33];
bool scheduledtime[33];
bool lscheduledtime[33];
bool lbuttonstate[17];
bool controled[17];
bool updatecerror[17];
byte mm;
bool updated;
bool availablee;

const int lamps[18] = { 0, 3, 5, 7, 9, 11, 24, 15, 17, 23, 25, 27, 29, 31, 30, 28, 26 };
const int currents[18] = { 0, A2, A2, A2, A2, A2, A2, A2, A0, A1, A3, A5, A7, A9, A11, A13, A15 };
const int suckets[18] = { 0, 16, 14, 12, 10, 8, 6, 4, 2, 46, 44, 42, 40, 38, 36, 34, 32 };
const int buttons[18] = { 0, 43, 41, 39, 37, 35, 33, A6, 22, 47, 48, A12, A10, 53, A8, 49, 45 };

//const int buttons[18] = {0, 22, A6, 33, 35, 37, 39, 41, 43, 47, 48, A12, A10, 53, A8, 49, 45};
// for first smitch only (Mrs. Aminkeng)

int updatepos;
unsigned long lcurrenttime, lcurrentupdate, buzzertime, lcurrentfound[17], lbuttonfound[17], llbuttonfound[17], startsending, updatetime, lastread, tsending;
unsigned long powertime = 0, secondstimee;
String infor;
char tosend[100], c;
int counting;
String sending = "", mydata = "";
bool canlaunchit = false, modified[33], sendit, timeerror;
int powerpin = A4;
bool ispowered = false, isready;

void setup() {
  isready = false;
  int i, p;
  Serial1.begin(9600);
  Serial.begin(9600);
  int q;
  for (q = 1; q <= sizeton2; q++) pinMode(lamps[q], OUTPUT);
  for (q = 1; q <= sizeton2; q++) pinMode(suckets[q], OUTPUT);
  for (q = 1; q <= sizeton2; q++) pinMode(buttons[q], INPUT);
  for (q = 1; q <= sizeton2; q++) lcurrentfound[i] = 0;
  pinMode(powerpin, OUTPUT);
  if (!rtc.begin()) {
    timeerror = true;
  }
  if (rtc.lostPower()) {
    timeerror = true;
  }
  readtime();
  EEPROM.get(200, gdata);
  if (EEPROM.read(4001) == 1) {
    EEPROM.get(4002, where);
    for (i = 1; i <= sizet; i++) {
      EEPROM.get(where.s1pos + i * 15, devices[i]);
    }
  } else {
    where.yy = 2024;
    where.s1pos = 500;
    EEPROM.put(4002, where);
    EEPROM.write(4001, 1);
    for (i = 1; i <= sizet; i++) {
      devices[i].dstate = 0;
      devices[i].enable = true;
      devices[i].starthh = 0;
      devices[i].startmm = 0;
      devices[i].stophh = 0;
      devices[i].stopmm = 0;
      if (i == 1) devices[i].starthh = 30;
      for (int p = 1; p <= 7; p++) devices[i].days[p] = false;
      EEPROM.put(where.s1pos + i * 15, devices[i]);
    }
  }

  //  while (1) {
  //    for (i = 1; i <= sizeton2; i++) {
  //      Serial.print("S"); Serial.println(i);
  //      for (int j = 1; j <= 5; j++) {
  //        digitalWrite(suckets[i], HIGH);
  //        delay(500);
  //        digitalWrite(suckets[i], LOW);
  //        delay(500);  //      }
  //    }
  //  }

  ondelay();
  for (i = 1; i <= sizet; i++) state[i] = devices[i].dstate;
  if (analogRead(A14) > 500) checkschedules();
  availablee = false;
  for (p = 1; p <= sizet; p++) {
    if ((based == true) && (p == 1)) continue;
    if (devices[p].dstate == true) {
      delay(100);
      // Serial.print((p <= sizeton2) ? 'P' : 'S');
      // Serial.print((p <= sizeton2) ? p : p - sizeton2); Serial.print(" state: ");
      //Serial.println(devices[p].dstate);
      devices[p].dstate = false;
      control(p, true, false);
    }
  }
  //devices[1].starthh = 30;
  updatepos = 1;
  availablee = false;
  //if (debug) Serial.println("ready");
  ispowered = false;
  sendit = true;
  powertime = millis();
  canlaunchit = true;
  Serial.print("ready: ");
  Serial.println(millis());
}

void loop() {
  if (Serial.available()) {
    int i;
    String temp = Serial.readString();
    Serial.print("Actual schedule1-8: ");
    for (i = 1; i <= 8; i++) {
      struct splitting s = split(devices[i].compound);
      Serial.print(s.schedule_enabled);
    }
    Serial.println();
    Serial.print("Actual schedule8-16: ");
    for (i = 1; i <= 8; i++) {
      struct splitting s = split(devices[i + sizeton2].compound);
      Serial.print(s.schedule_enabled);
    }
    Serial.println();
  }
  if ((availablee == false) && (millis() - powertime >= 20000) && (ispowered == true)) {
    digitalWrite(powerpin, LOW);
  }
  if ((sendit == false) && (millis() - tsending >= 5000)) {
    //if (debug) Serial.println("about to send again");
    sprintf(tosend, "%c%02d%d%d%dZ", 'P', 1, 2, 2, 2);
    sending += String(tosend);
    sendit = true;
  }
  if (millis() - lastread < 0) {  //overflow, reset all millis vairiables
    int q;
    lcurrenttime = tsending = lcurrentupdate = buzzertime = startsending = updatetime = lastread = millis();
    for (q = 1; q <= sizeton2; q++) {
      lcurrentfound[q] = lbuttonfound[q] = llbuttonfound[q] = millis();
    }
  }
  readesp();
  if (millis() - lastread >= 500) {
    if (based == true) {
      if (devices[1].dstate == true) {
        if (millis() - buzzertime >= (devices[1].starthh * 1000)) {
          control(1, true, true);
        }
      }
    }
    if (analogRead(A14) > 500) {
      if (ispowered == false) {
        digitalWrite(powerpin, HIGH);
        ispowered = true;
      }
      if (availablee == false) {
        ondelay();
        turnon();
        sprintf(tosend, "%c%02d%d%d%dZ", 'P', 1, 3, 1, 1);
        sending += String(tosend);
        while (sending.length() > 0) {
          Serial1.print(sending[0]);
          sending.remove(0, 1);
        }
      }
      availablee = true;
    } else {
      if (availablee == true) {
        turnoff();
        availablee = false;
        powertime = millis();
        sprintf(tosend, "%c%02d%d%d%dZ", 'P', 1, 3, 0, 1);
        //Serial.println("updating off available");
        sending += String(tosend);
        while (sending.length() > 0) {
          Serial1.print(sending[0]);
          sending.remove(0, 1);
        }
        int j;
        for (j = 1; j <= sizet; j++) {
          if (modified[j] == true) {
            EEPROM.put(where.s1pos + 15 * j, devices[j]);
            modified[j] = false;
            //Serial.print("storing: "); Serial.println(j);
          }
        }
        //Serial.println("done saving");
      }
    }
    lastread = millis();
  }
  if (availablee == true) {
    if (updated == false) {
      if (millis() > updatestarttime) {
        if (canlaunchit == true) {
          if (millis() - updatetime >= 3000) {
            //Serial.print("U: "); Serial.print((updatepos <= sizeton2) ? ('P') : ('S'));
            //Serial.print((updatepos <= sizeton2) ? (updatepos) : (updatepos - sizeton2)); Serial.print(" : ");
            sprintf(tosend, "%c%02d%d%d%dZ", (updatepos <= sizeton2) ? ('P') : ('S'), (updatepos <= sizeton2) ? (updatepos) : (updatepos - sizeton2), 1,
                    devices[updatepos].dstate, (state[updatepos] != devices[updatepos].dstate));
            state[updatepos] = devices[updatepos].dstate;
            sending += String(tosend);
            sprintf(tosend, "%c%02d%d%dZ", (updatepos <= sizeton2) ? ('P') : ('S'), (updatepos <= sizeton2) ? (updatepos) : (updatepos - sizeton2), 5,
                    devices[updatepos].enable);
            state[updatepos] = devices[updatepos].dstate;
            sending += String(tosend);
            struct splitting s = split(devices[updatepos].compound);
            sprintf(tosend, "%c%02d%d%dZ", (updatepos <= sizeton2) ? ('P') : ('S'), (updatepos <= sizeton2) ? (updatepos) : (updatepos - sizeton2), 6,
                    s.schedule_enabled);
            state[updatepos] = devices[updatepos].dstate;
            sending += String(tosend);
            //Serial.print(sending);
            while (sending.length() > 0) {
              Serial1.print(sending[0]);
              sending.remove(0, 1);
            }
            if (updatepos + 1 <= sizet) {
              updatepos++;
            } else {
              tsending = millis();
              sendit = false;
              //if (debug) Serial.println("done updating");
              updated = true;
              sprintf(tosend, "%c%02d%d%d%dZ", 'P', 1, 2, 2, 2);
              sending += String(tosend);
              Serial.println("done updating esp");
            }
            updatetime = millis();
          }
        }
      }
    }
    if (millis() - startsending > 10) {
      if (sending.length() > 0) {
        Serial1.print(sending[0]);
        // if(debug) Serial.print(sending[0]);
        sending.remove(0, 1);
      }
      startsending = millis();
    }
    readbutton();
    if (millis() - lcurrentupdate >= 1000) {
      int z, x;
      //if(debug) Serial.println("working..");
      readtime();
      if (thetime.mm != mm) {  //a minute has passed so update schedules
        mm = thetime.mm;
        if (debug) Serial.println();
        if (debug) Serial.print("TIME: ");
        if (debug) Serial.print(thetime.hh);
        if (debug) Serial.print(":");
        if (debug) Serial.print(thetime.mm);
        if (debug) Serial.print(" D: ");
        if (debug) Serial.println(thetime.weekday);
        struct splitting s;
        for (i = 1; i <= 32; i++) {
          if (debug) Serial.print((i <= sizeton2) ? 'P' : 'S');
          if (debug) Serial.print((i <= sizeton2) ? i : (i - sizeton2));
          if (debug) Serial.print(" Scheduled: ");
          if (debug) Serial.print(scheduled[i]);
          s = split(devices[i].compound);
          if (debug) Serial.print(" in: ");
          if (debug) Serial.print(s.inschedule);
          if (debug) Serial.print(" Time: ");
          if (debug) Serial.println(scheduledtime[i]);
        }
        //if (debug) Serial.println();
        checkschedules();
        if (debug) Serial.println();
        for (i = 1; i <= 32; i++) {
          if (debug) Serial.print((i <= sizeton2) ? 'P' : 'S');
          if (debug) Serial.print((i <= sizeton2) ? i : (i - sizeton2));
          if (debug) Serial.print(" Scheduled: ");
          if (debug) Serial.print(scheduled[i]);
          if (debug) Serial.print(" in: ");
          if (debug) Serial.print(devices[i].inschedule);
          if (debug) Serial.print(" Time: ");
          if (debug) Serial.println(scheduledtime[i]);
        }
        int i;
      }
      if (where.yy != thetime.yy) {  //change location
        int ps1pos = where.s1pos;
        if (where.s1pos + 500 < 4000) where.s1pos += 500;
        else where.s1pos = 500;
        where.yy = thetime.yy;
        for (int i = 1; i <= sizet; i++) {
          EEPROM.get(ps1pos + i * 15, devices[i]);
          EEPROM.put(where.s1pos + i * 15, devices[i]);
          modified[i] = false;
        }
        EEPROM.put(4002, where);
      }
      lcurrentupdate = millis();
    }

    int j, i;
    for (i = 1; i <= sizet; i++) {
      if (debug) Serial.print((i <= sizeton2) ? ('P') : ('S'));
      if (debug) Serial.println((i <= sizeton2) ? (i) : (i - sizeton2));
      if (debug) Serial.println(devices[i].dstate);
      if (debug) Serial.println(devices[i].enable);
      if (debug) Serial.print(String(devices[i].starthh) + String(":") + String(devices[i].startmm));
      if (debug) Serial.print(" TO ");
      if (debug) Serial.println(String(devices[i].stophh) + String(":") + String(devices[i].stopmm));
      for (j = 1; j < 8; j++)
        if (debug) Serial.print(String(devices[i].days[j]) + String(" "));
      if (debug) Serial.println();
      if (debug) Serial.println();
    }
  }
}

void ondelay() {
  isready = false;
  unsigned long ontiming = millis();
  while (millis() - ontiming <= 3000) {
    readesp();
  }
  isready = true;
}
void turnoff() {
  int p;
  for (p = 1; p <= sizeton2; p++) {
    delay(100);
    digitalWrite(suckets[p], LOW);
  }
  for (p = 1; p <= sizeton2; p++) {
    delay(100);
    digitalWrite(lamps[p], LOW);
  }
}

void turnon() {
  int p;
  for (p = 1; p <= sizeton2; p++) {
    digitalWrite(suckets[p], devices[p + sizeton2].dstate);
    if (devices[p + sizeton2].dstate == true) delay(100);
  }
  for (p = 1; p <= sizeton2; p++) {
    digitalWrite(lamps[p], devices[p].dstate);
    if (devices[p].dstate == true) delay(100);
  }
}

bool checkforschedule(int port) {
  if (timeerror) return;
  if (devices[port].starthh <= devices[port].stophh) {    //in the same day
    if (devices[port].starthh == devices[port].stophh) {  //set in the same time
      if ((thetime.hh == devices[port].starthh) && (thetime.mm >= devices[port].startmm) && (thetime.mm < devices[port].stopmm)) return true;
    } else {  //different hours
      if ((thetime.hh == devices[port].starthh) && (thetime.mm >= devices[port].startmm)) return true;
      if ((thetime.hh == devices[port].stophh) && (thetime.mm < devices[port].stopmm)) return true;
      if ((thetime.hh > devices[port].starthh) && (thetime.hh < devices[port].stophh)) return true;
    }
  } else {                                                                                                 //in two different days
    if (thetime.hh > devices[port].starthh) return true;                                                   //at the end of the day after start hh
    else if ((thetime.hh == devices[port].starthh) && (thetime.mm >= devices[port].startmm)) return true;  //end hour higher minute
    else if (thetime.hh < devices[port].stophh) return true;                                               //at the begining before stop hour
    else if ((thetime.hh == devices[port].stophh) && (thetime.mm < devices[port].stopmm)) return true;
  }
  return false;
}

void checkschedules() {
  int x;
  struct splitting s;
  for (x = 1; x <= sizet; x++) {
    if ((based == true) && (x == 1)) continue;
    scheduled[x] = devices[x].days[thetime.weekday + 1];
  }
  for (x = 1; x <= sizet; x++) {
    if ((based == true) && (x == 1)) continue;
    scheduledtime[x] = checkforschedule(x);
  }
  for (x = 1; x <= sizet; x++) {
    if ((based == true) && (x == 1)) continue;
    s = split(devices[x].compound);
    /*Serial.print("enable for "); Serial.print((x <= sizeton2) ? 'P' : 'S');
      Serial.print((x <= sizeton2) ? x : x - sizeton2); Serial.print(": "); Serial.print(s.schedule_enabled);
      Serial.print(" <= "); Serial.println(s.schedule_enabled);
    */
    if ((scheduled[x] == true) && (s.schedule_enabled == true)) {  //active day and active schedules
      if (scheduledtime[x] != lscheduledtime[x]) {                 //there is a change
        if (scheduledtime[x] == true) {
          s = split(devices[x].compound);
          s.bstate = devices[x].dstate;
          s.inschedule = true;
          //devices[x].bstate = devices[x].dstate;
          //devices[x].inschedule = true;
          devices[x].compound = combine(s);
          modified[x] = true;
          /*
            if (debug) Serial.print("Entering time for: ");  if (debug) Serial.print((x <= sizeton2) ? 'P' : 'S');
            if (debug) Serial.println((x <= sizeton2) ? x : x - sizeton2);
            if (debug) Serial.print("entering state: ");  if (debug) Serial.println(devices[x].dstate);
            if (debug) Serial.print("schedule type: ");  if (debug) Serial.println(s.type);
          */
          if (s.type == false) {
            if (devices[x].dstate == true) {  //turn off load
              scheduledtime[x] = false;
              control(x, true, true);  //turn off load
              scheduledtime[x] = true;
            }
          } else {
            if (devices[x].dstate == false) {  //turn off load
              scheduledtime[x] = false;
              control(x, true, true);  //turn off load
              scheduledtime[x] = true;
            }
          }
        } else if (scheduledtime[x] == false) {
          //if (debug) Serial.print("Exiting time for: ");  if (debug) Serial.print((x <= sizeton2) ? 'P' : 'S');
          //if (debug) Serial.println((x <= sizeton2) ? x : x - sizeton2);

          s = split(devices[x].compound);
          s.inschedule = false;
          devices[x].compound = combine(s);
          //if (debug) Serial.print("exiting D state: ");  if (debug) Serial.println(s.bstate);
          if (s.type == false) {
            if (devices[x].dstate != s.bstate) {  //state different
              control(x, true, true);
            } else {
              modified[x] = true;
            }
          } else {
            if (devices[x].dstate == true) {  //state different
              control(x, true, true);
            } else {
              modified[x] = true;
            }
          }
        }
        lscheduledtime[x] = scheduledtime[x];
      } else {
        if (scheduledtime[x] == false) {  //could have just moved out of schedule
          s = split(devices[x].compound);
          if (s.inschedule == true) {
            //if (debug) Serial.print("move out time for: ");  if (debug) Serial.print((x >= sizeton2) ? 'P' : 'S');
            //if (debug) Serial.println((x <= sizeton2) ? x : x - sizeton2);
            s = split(devices[x].compound);
            s.inschedule = false;
            devices[x].compound = combine(s);
            if (s.type == false) {
              if (devices[x].dstate != s.bstate) {  //state different
                control(x, true, true);
              } else {
                modified[x] = true;
              }
            } else {
              if (devices[x].dstate == true) {  //state different
                control(x, true, true);
              } else {
                modified[x] = true;
              }
            }
          }
        }
      }
    } else {  //not active days or deactivated schedules
      lscheduledtime[x] = false;
      s = split(devices[x].compound);
      if (s.inschedule == true) {
        //if (debug) Serial.print("Exiting day: ");  if (debug) Serial.print((x <= sizeton2) ? 'P' : 'S');
        //if (debug) Serial.println((x <= sizeton2) ? x : x - sizeton2);
        s = split(devices[x].compound);
        s.inschedule = false;
        devices[x].compound = combine(s);
        if (s.type == false) {
          if (devices[x].dstate != s.bstate) {  //state different
            control(x, true, true);
          } else {
            modified[x] = true;
          }
        } else {
          if (devices[x].dstate == true) {  //state different
            control(x, true, true);
          } else {
            modified[x] = true;
          }
        }
      }
    }
  }
}

void readtime() {
  DateTime now = rtc.now();
  thetime.hh = now.hour();
  thetime.mm = now.minute();
  thetime.ss = now.second();
  thetime.dd = now.day();
  thetime.mn = now.month();
  thetime.yy = now.year();
  thetime.weekday = now.dayOfTheWeek();
  unsigned long seconds = now.unixtime();
  if (seconds != 0) {
    secondstimee = seconds;
  }
}

void readesp() {
  int i;
  struct splitting s;
  //char states[2][4] = {"OFF", "ON"};
  //char enable[2][10] = {"DISABLE", "ENABLE"};
  //char days[8][6] = {"", "Mon", "Tue", "Wed", "Thur", "Fri", "Sat", "Sun"};
  if (Serial1.available()) {
    c = Serial1.read();
    if (c == '*') {
      String gsending = "";
      String data = "";
      while (c != '#') {
        if (Serial1.available()) {
          c = Serial1.read();
          if (c) data += String(c);
        }
      }
      updatetime = millis();
      //strcpy(gdata.wifi_ssid, "test");
      //strcpy(gdata.wifi_password, "test1234");
      //Serial.print("raw: "); Serial.println(data);
      bool changed = false;
      while (data[0] == '*') data = data.substring(1, data.length());

      //process and respond
      if (data.indexOf("wssid") >= 0) {
        gsending = "***wssid," + String(gdata.wifi_ssid) + ",#";
      } else if (data.indexOf("wpasw") >= 0) {
        gsending = "***wpasw," + String(gdata.wifi_password) + ",#";
      }
      if (data.indexOf("docid") >= 0) {
        gsending = "***docid," + String(gdata.document_id) + ",#";
      } else if (data.indexOf("pass") >= 0) {
        gsending = "***pass," + String(gdata.password) + ",#";
      } else if (data.indexOf("ok") >= 0) {
        gsending = "***ok," + String(gdata.ok) + ",#";
        //Serial.print("Respond:  **ok," + String(1) + ",#");
      }

      else if (data.indexOf("wssir") >= 0) {
        data = data.substring(data.indexOf(',') + 1, data.length() + 1);
        data = data.substring(0, data.indexOf(','));
        data.toCharArray(gdata.wifi_ssid, data.length() + 1);
        changed = true;
        gsending = "**wssir#";
      } else if (data.indexOf("wpawr") >= 0) {
        data = data.substring(data.indexOf(',') + 1, data.length() + 1);
        data = data.substring(0, data.indexOf(','));
        data.toCharArray(gdata.wifi_password, data.length() + 1);
        changed = true;
        gsending = "**wpawr#";
      }
      if (data.indexOf("docir") >= 0) {
        data = data.substring(data.indexOf(',') + 1, data.length() + 1);
        data = data.substring(0, data.indexOf(','));
        data.toCharArray(gdata.document_id, data.length() + 1);
        changed = true;
        gsending = "**docir#";
      } else if (data.indexOf("pasr") >= 0) {
        data = data.substring(data.indexOf(',') + 1, data.length() + 1);
        data = data.substring(0, data.indexOf(','));
        data.toCharArray(gdata.password, data.length() + 1);
        changed = true;
        gsending = "**pasr#";
      } else if (data.indexOf("okr") >= 0) {
        changed = true;
        gdata.ok = true;
        gsending = "**okr#";
      }
      if (changed == true) EEPROM.put(200, gdata);
      while (gsending.length() > 0) {
        Serial1.print(gsending[0]);
        gsending.remove(0, 1);
        delay(2);
      }
      return;
    }
    if (c != 'Z') {
      mydata += String(c);
      //   if(debug) Serial.println(mydata);
      return;
    }
    // if(debug) Serial.println("i am out");
    infor = mydata;
    mydata = "";
    if (debug) Serial.print("F: ");
    if (debug) Serial.println(infor);
    int port = (infor[1] - '0') * 10 + infor[2] - '0';
    if (infor[0] == 'S') port += sizeton2;
    switch (infor[3]) {
      case '1':
        {  //update state
          if (infor[0] == 'U') {
            updated = false;
            canlaunchit = true;
            updatepos = 1;
            if (availablee == true) {
              sprintf(tosend, "%c%02d%d%d%dZ", 'P', 1, 3, 1, 1);
              sending += String(tosend);
              while (sending.length() > 0) {
                Serial1.print(sending[0]);
                sending.remove(0, 1);
              }
            } else {
              sprintf(tosend, "%c%02d%d%d%dZ", 'P', 1, 3, 0, 1);
              sending += String(tosend);
              while (sending.length() > 0) {
                Serial1.print(sending[0]);
                sending.remove(0, 1);
              }
            }
            if (based) {
              updatepos++;
            }
          } else {
            if ((availablee == true) && (isready)) {
              control(port, 1, true);
            } else {
              sprintf(tosend, "%c%02d%d%d%dZ", (port <= sizeton2) ? 'P' : 'S', (port <= sizeton2) ? port : port - sizeton2, 1, devices[port].dstate, 1);
              sending += String(tosend);
              while (sending.length() > 0) {
                Serial1.print(sending[0]);
                sending.remove(0, 1);
              }
            }
          }
          // if(debug) Serial.println("before");
          //sprintf(tosend,  "%c %d TURNED %s", infor[0], port % 16, states[devices[port].dstate]);
          //   if(debug) Serial.println(tosend);
          // if(debug) Serial.println("after");
          break;
        }
      case '2':
        {  //update enable

          if (devices[port].enable != (infor[4] == '1')) {
            //if (debug) Serial.print("different for: ");  if (debug) Serial.print((port <= sizeton2) ? 'P' : 'S');  if (debug) Serial.println((port <= sizeton2) ? port : port - sizeton2);
            devices[port].enable = infor[4] - '0';
            modified[port] = true;
            Serial.print("receiving enable for: ");
            Serial.println(port);
          }
          sprintf(tosend, "%c%02d%d%d%dZ", (port <= sizeton2) ? 'P' : 'S', (port <= sizeton2) ? port : port - sizeton2, 5, devices[port].enable, 1);
          sending += String(tosend);
          while (sending.length() > 0) {
            Serial1.print(sending[0]);
            sending.remove(0, 1);
          }
          break;
        }
      case '3':
        {  //update schedule
          //"S053hhmmhhmm0000000" = set timer for thursday and sunday as from 2 30 to 15 40;
          devices[port].starthh = (infor[4] - '0') * 10 + infor[5] - '0';
          devices[port].startmm = (infor[6] - '0') * 10 + infor[7] - '0';
          devices[port].stophh = (infor[8] - '0') * 10 + infor[9] - '0';
          devices[port].stopmm = (infor[10] - '0') * 10 + infor[11] - '0';
          s = split(devices[port].compound);
          bool oldtype = s.type;
          s.type = infor[19] - '0';
          s.schedule_enabled = infor[20] - '0';
          sprintf(tosend, "%c%02d%d%dZ", (port <= sizeton2) ? ('P') : ('S'), (port <= sizeton2) ? (port) : (port - port), 6,
                  s.schedule_enabled);
          sending += String(tosend);
          Serial.println("updating: ");
          Serial.println(tosend);
          while (sending.length() > 0) {
            Serial1.print(sending[0]);
            sending.remove(0, 1);
          }
          Serial.print("type: ");
          Serial.println(s.type);
          Serial.print("schedule: ");
          Serial.println(s.schedule_enabled);
          devices[port].compound = combine(s);
          //Serial.print("compound: "); Serial.println(devices[port].compound);
          for (i = 1; i <= 7; i++) {
            devices[port].days[i] = infor[i + 11] - '0';
          }
          modified[port] = true;
          if (s.schedule_enabled == true) {
            if (((devices[port].days[thetime.weekday + 1]) == true) && (checkforschedule(port) == true)) {
              if (oldtype != s.type) {
                if ((s.type == true) && (devices[port].dstate == false)) {
                  if (isready) control(port, true, true);
                } else if ((s.type == false) && (devices[port].dstate == true)) {
                  if (isready) control(port, true, true);
                }
              }
            }
          }
          if (availablee == true) checkschedules();
          /*sprintf(tosend, "%c %d schedule from %d:%d to %d %d", infor[0], port % 16, devices[port].starthh, devices[port].startmm,
                  devices[port].stophh, devices[port].stopmm);
               if(debug) Serial.print(tosend);
               if(debug) Serial.print(" FOR ");
            for (i = 1; i <= 7; i++) {
            if (devices[port].days[i]) {
                 if(debug) Serial.print(days[i]);    if(debug) Serial.print(" ");
            }
            }*/
          //   if(debug) Serial.println();
          break;
        }
      case '4':
        {  //receive real time
          //if (debug) Serial.println("real time received");
          thetime.dd = (infor[4] - '0') * 10 + infor[5] - '0';
          thetime.mn = (infor[6] - '0') * 10 + infor[7] - '0';
          thetime.yy = (infor[8] - '0') * 1000 + (infor[9] - '0') * 100 + (infor[10] - '0') * 10 + (infor[11] - '0');
          thetime.hh = (infor[12] - '0') * 10 + infor[13] - '0';
          thetime.mm = (infor[14] - '0') * 10 + infor[15] - '0';
          thetime.ss = (infor[16] - '0') * 10 + infor[17] - '0';
          timeerror = false;
          rtc.adjust(DateTime(thetime.yy, thetime.mn, thetime.dd, thetime.hh, thetime.mm, thetime.ss));
          /*
               if(debug) Serial.println("DATE AND TIME CORRECTED TO");
            sprintf(tosend, "DATE: %d/%d/%d\nTIME: %d:%d:%d", thetime.dd, thetime.mn, thetime.yy, thetime.hh, thetime.mm, thetime.ss);
               if(debug) Serial.println(tosend);
            //"S054ddmnyyyyhhmmss" = set timer for thursday and sunday as from 2 30 to 15 40;
            //thetime.dd = infor[]
          */
          break;
        }
      case '5':
        {
          if (availablee == true) {
            sprintf(tosend, "%c%02d%d%d%dZ", 'P', 1, 3, 1, 1);
            sending += String(tosend);
            while (sending.length() > 0) {
              Serial1.print(sending[0]);
              sending.remove(0, 1);
            }
          } else {
            sprintf(tosend, "%c%02d%d%d%dZ", 'P', 1, 3, 0, 1);
            sending += String(tosend);
            while (sending.length() > 0) {
              Serial1.print(sending[0]);
              sending.remove(0, 1);
            }
          }
          break;
        }
      case '6':
        {
          if (availablee == false) {
            //Serial.println("just updated online");
            digitalWrite(powerpin, LOW);
          }
          break;
        }
      case '8':
        {
          //Serial.println("i am in case 8");
          int p;
          if (infor[4] == '0') {  //turn off all lamps
            for (p = 1; p <= sizeton2; p++) {
              if ((p == 1) && (based == true)) continue;
              if (devices[p].dstate == true) {
                digitalWrite(lamps[p], LOW);
                delay(100);
                devices[p].dstate = false;
              }
            }
            updated = false;
            canlaunchit = true;
            if (based) updatepos = 1;
            else updatepos = 1;
          } else if (infor[4] == '1') {  //turn on all all lamps
            for (p = 1; p <= sizeton2; p++) {
              if ((p == 1) && (based == true)) continue;
              if (devices[p].dstate == false) {
                digitalWrite(lamps[p], HIGH);
                delay(100);
                devices[p].dstate = true;
              }
            }
            updated = false;
            canlaunchit = true;
            if (based) updatepos = 1;
            else updatepos = 1;
          } else if (infor[4] == '2') {  //turn off all suckets
            for (p = 1; p <= sizeton2; p++) {
              if (devices[p + sizeton2].dstate == true) {
                digitalWrite(suckets[p], LOW);
                delay(100);
                devices[p + sizeton2].dstate = false;
              }
            }
            updated = false;
            canlaunchit = true;
            if (based) updatepos = 1;
            else updatepos = 1;
          } else if (infor[4] == '3') {  //turn on all all lamps
            for (p = 1; p <= sizeton2; p++) {
              if (devices[p + sizeton2].dstate == false) {
                digitalWrite(suckets[p], HIGH);
                delay(100);
                devices[p + sizeton2].dstate = true;
              }
            }
            updated = false;
            canlaunchit = true;
            if (based) updatepos = 1;
            else updatepos = 1;
          }
          break;
        }
      case '9':
        {  //receive schedule from S1
          s = split(devices[port].compound);
          s.schedule_enabled = infor[4] - '0';
          devices[port].compound = combine(s);
          modified[port] = true;
          sprintf(tosend, "%c%02d%d%d%dZ", (port <= sizeton2) ? 'P' : 'S', (port <= sizeton2) ? port : port - sizeton2, 6, s.schedule_enabled, 1);
          sending += String(tosend);
          while (sending.length() > 0) {
            Serial1.print(sending[0]);
            sending.remove(0, 1);
          }
          break;
        }
    }
  }
  //Serial.println("done sending");
  // if(debug) Serial.println("done");
}

struct splitting split(byte data) {
  struct splitting s;
  if (data >= 30) {  //  1   1
    s.type = true;
    s.schedule_enabled = true;
    data = data % 10;
  } else if (data >= 20) {  // 1   0
    s.type = false;
    s.schedule_enabled = true;
    data = data % 10;
  } else if (data >= 10) {  //0   1
    s.type = true;
    s.schedule_enabled = false;
    data = data % 10;
  } else {  //0    0
    s.schedule_enabled = false;
    s.type = false;
  }
  if (data == 0) {
    s.bstate = false;
    s.inschedule = false;
  } else if (data == 1) {
    s.bstate = false;
    s.inschedule = true;
  } else if (data == 2) {
    s.bstate = true;
    s.inschedule = false;
  } else if (data == 3) {
    s.bstate = true;
    s.inschedule = true;
  }
  return s;
}

byte combine(struct splitting s) {
  if (s.schedule_enabled == false) {
    if (s.type == false) {
      if (s.bstate == false) {
        if (s.inschedule == false) return 0;
        else return 1;
      } else {
        if (s.inschedule == false) return 2;
        else return 3;
      }
    } else {
      if (s.bstate == false) {
        if (s.inschedule == false) return 10;
        else return 11;
      } else {
        if (s.inschedule == false) return 12;
        else return 13;
      }
    }
  } else {
    if (s.type == false) {
      if (s.bstate == false) {
        if (s.inschedule == false) return 20;
        else return 21;
      } else {
        if (s.inschedule == false) return 22;
        else return 23;
      }
    } else {
      if (s.bstate == false) {
        if (s.inschedule == false) return 30;
        else return 31;
      } else {
        if (s.inschedule == false) return 32;
        else return 33;
      }
    }
  }
}

/*
  00 = 0 = bstate = false && inschedule = false;
  01 = 1 = bstate = false && inschedule = true;
  10 = 2 = bstate = true && inschedule = false;
  11 = 3 = bstate = true && inschedule = true;
*/

void control(int port, bool online, bool change) {
  // Serial.println("CALLING");
  struct splitting s;
  //Serial.print("controlling port: ");
  //Serial.println(port);
  s = split(devices[port].compound);
  if ((based == true) && (port == 1)) {
    //Serial.print("acting on p1: "); Serial.println(!devices[port].dstate);
    if ((online == false) && (devices[1].dstate == true) && (devices[1].enable == false)) return;
    devices[port].dstate = !devices[port].dstate;
    digitalWrite(lamps[port], devices[port].dstate);
    sprintf(tosend, "%c%02d%d%d%dZ", 'P', port, 1, devices[port].dstate, 1);
    sending += String(tosend);
    while (sending.length() > 0) {
      Serial1.print(sending[0]);
      sending.remove(0, 1);
    }
    sending += String(tosend);
    if (devices[port].dstate == true) {
      buzzertime = millis();
    }
    return;
  }
  if (((scheduled[port] == true) && (scheduledtime[port] == true) && (s.schedule_enabled == true) && (online == false)) || ((online == false) && (devices[port].enable == false))) {
    //do nothing
    // if(debug) Serial.println("do nothing");
  } else {
    //state is allowed to be altered
    devices[port].dstate = !devices[port].dstate;
    if (port <= sizeton2) {
      digitalWrite(lamps[port], devices[port].dstate);
      sprintf(tosend, "%c%02d%d%d%dZ", 'P', port, 1, devices[port].dstate, 1);
      sending += String(tosend);
      while (sending.length() > 0) {
        Serial1.print(sending[0]);
        sending.remove(0, 1);
      }
      sending += String(tosend);
      //   if(debug) Serial.println(tosend); //to serial 1
    } else {
      digitalWrite(suckets[port - sizeton2], devices[port].dstate);
      sprintf(tosend, "%c%02d%d%d%dZ", 'S', port - sizeton2, 1, devices[port].dstate, 1);
      sending += String(tosend);
      while (sending.length() > 0) {
        Serial1.print(sending[0]);
        sending.remove(0, 1);
      }
      sending += String(tosend);
      //if(debug) Serial.println(tosend); //to serial 1
    }
    if ((scheduled[port] == true) && (scheduledtime[port] == true) && (s.schedule_enabled == true) && (online == true)) {
      s.bstate = devices[port].dstate;
      devices[port].compound = combine(s);
    }
    //if (debug) Serial.print("TURNING ");  if (debug) Serial.print((port <= sizeton2) ? 'P' : 'S');
    //if (debug) Serial.print((port <= sizeton2) ? port : port - sizeton2);  if (debug) Serial.print(" TO: ");
    //if (debug) Serial.println(devices[port].dstate);
    //if ((updated == true) || ((updated == false) && (change == false))) {
    modified[port] = true;
  }
}

void readbutton() {
  int y;
  for (y = 1; y <= sizeton2; y++) {
    if (digitalRead(buttons[y]) == HIGH) {
      lbuttonfound[y] = millis();
      if (lbuttonstate[y] == 0) {
        llbuttonfound[y] = millis();
        //Serial.print("P"); Serial.println(y);
        lbuttonstate[y] = 1;
        controled[y] = false;
        //control(y, false, true);
      }
    } else {
      lbuttonstate[y] = 0;
      controled[y] = true;
    }
  }
  for (y = 1; y <= sizeton2; y++) {
    if ((millis() - llbuttonfound[y] > 100) && (lbuttonstate[y] == 1) && (controled[y] == false)) {
      controled[y] = true;
      control(y, false, true);
    }
  }
  for (y = 1; y <= sizeton2; y++) {
    if ((millis() - lbuttonfound[y] > buttondebounce) && (lbuttonstate[y] == 1)) {
      lbuttonstate[y] = 0;
    }
  }
}
