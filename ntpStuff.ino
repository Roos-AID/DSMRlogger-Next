/*
***************************************************************************  
**  Program  : ntpStuff, part of DSMRlogger-Next
**  Version  : v2.3.0-rc5
**
**  Copyright (c) 2020 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/
#if defined(USE_NTP_TIME)
#define NTP_DEFAULT_TIMEZONE "Europe/Amsterdam"
#define NTP_HOST_DEFAULT "pool.ntp.org"
#define NTP_RESYNC_TIME 1800 //seconds = every 30 minutes
bool      settingNTPenable = true;
String    settingNTPtimezone = NTP_DEFAULT_TIMEZONE;
String    settingNTPhostname = NTP_HOST_DEFAULT;

//Use the NTP SDK ESP 8266 
#include <time.h>
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

enum NtpStatus_t {
	TIME_NOTSET,
	TIME_SYNC,
	TIME_WAITFORSYNC,
  TIME_NEEDSYNC
};

NtpStatus_t NtpStatus = TIME_NOTSET;
time_t NtpLastSync = 0; //last sync moment in EPOCH seconds

//==[ NTP stuff ]==============================================================


void startNTP(){
  // Initialisation ezTime
  if (!settingNTPenable) return;
  if (settingNTPtimezone.length()==0) settingNTPtimezone = NTP_DEFAULT_TIMEZONE; //set back to default timezone
  if (settingNTPhostname.length()==0) settingNTPhostname = NTP_HOST_DEFAULT; //set back to default timezone

  //void configTime(int timezone_sec, int daylightOffset_sec, const char* server1, const char* server2, const char* server3)
  configTime(0, 0, CSTR(settingNTPhostname), nullptr, nullptr);
  NtpStatus = TIME_WAITFORSYNC;
}


void getNTPtime(){
  struct timespec tp;   //to enable clock_gettime()
  double tNow;
  long dt_sec, dt_ms, dt_nsec;
  clock_gettime(CLOCK_REALTIME, &tp);  
  tNow = tp.tv_sec+(tp.tv_nsec/1.0e9);
  dt_sec = tp.tv_sec;
  dt_ms = tp.tv_nsec / 1000000UL;
  dt_nsec = tp.tv_nsec;
  DebugTf("tNow=%20.10f tNow_sec=%16.10ld tNow_nsec=%16.10ld dt_sec=%16li(s) dt_msec=%16li(sm) dt_nsec=%16li(ns)\r\n", tNow, tp.tv_sec,tp.tv_nsec, dt_sec, dt_ms, dt_nsec);
  DebugFlush();
}

void loopNTP(){
if (!settingNTPenable) return;
  switch (NtpStatus){
    case TIME_NOTSET:
    case TIME_NEEDSYNC:
      NtpLastSync = time(nullptr); //remember last sync
      DebugTln(F("Start time syncing"));
      startNTP();
      DebugTf("Starting timezone lookup for [%s]\r\n", CSTR(settingNTPtimezone));
      NtpStatus = TIME_WAITFORSYNC;
      break;
    case TIME_WAITFORSYNC:
      if ((time(nullptr)>0) || (time(nullptr) >= NtpLastSync)) { 
        NtpLastSync = time(nullptr); //remember last sync         
        auto myTz =  timezoneManager.createForZoneName(CSTR(settingNTPtimezone));
        if (myTz.isError()){
          //DebugTf("Error: Timezone Invalid/Not Found: [%s]\r\n", CSTR(settingNTPtimezone));
          settingNTPtimezone = NTP_DEFAULT_TIMEZONE;
          myTz = timezoneManager.createForZoneName(CSTR(settingNTPtimezone)); //try with default Timezone instead
        } else {
          //found the timezone, now set the time 
          auto myTime = ZonedDateTime::forUnixSeconds(NtpLastSync, myTz);
          if (!myTime.isError()) {
            //finally time is synced!
            setTime(myTime.hour(), myTime.minute(), myTime.second(), myTime.day(), myTime.month(), myTime.year());
            NtpStatus = TIME_SYNC;
            DebugTln(F("Time synced!"));
          }
        }
      } 
    break;
    case TIME_SYNC:
      if ((time(nullptr)-NtpLastSync) > NTP_RESYNC_TIME){
        //when xx seconds have passed, resync using NTP
         DebugTln(F("Time resync needed"));
        NtpStatus = TIME_NEEDSYNC;
      }
    break;
  } 
 
  // DECLARE_TIMER_SEC(timerNTPtime, 10, CATCH_UP_MISSED_TICKS);
  // if DUE(timerNTPtime) DebugTf("Epoch Seconds: %d\r\n", time(nullptr)); //timeout, then break out of this loop
  // if DUE(timerNTPtime) getNTPtime();
}

bool isNTPtimeSet(){
  return NtpStatus == TIME_SYNC;
}

void waitforNTPsync(int16_t timeout = 30){  
  //wait for time is synced to NTP server, for maximum of timeout seconds
  //feed the watchdog while waiting 
  //update NTP status
  time_t t = time(nullptr); //get current time
  DebugTf("Waiting for NTP sync, timeout: %d\r\n", timeout);
  DECLARE_TIMER_SEC(waitforNTPsync, timeout, CATCH_UP_MISSED_TICKS);
  DECLARE_TIMER_SEC(timerWaiting, 5, CATCH_UP_MISSED_TICKS);
  while (true){
    //feed the watchdog while waiting
    Wire.beginTransmission(0x26);   
    Wire.write(0xA5);   
    Wire.endTransmission();
    delay(100);
    if DUE(timerWaiting) DebugTf("Waiting for NTP sync: %lu seconds\r\n", (time(nullptr)-t));
    // update NTP status
    loopNTP();
    //stop waiting when NTP is synced 
    if (isNTPtimeSet()) {
      Debugln(F("NTP time synced!"));
      break;
    }
    //stop waiting when timeout is reached 
    if DUE(waitforNTPsync) {
      DebugTln(F("NTP sync timeout!"));
      break;
    } 
  }
}


//==[ end of NTP stuff ]=======================================================


/*
#include <WiFiUdp.h>            // - part of ESP8266 Core https://github.com/esp8266/Arduino
WiFiUDP           Udp;

const int         timeZone = 1;       // Central European (Winter) Time
unsigned int      localPort = 8888;   // local port to listen for UDP packets

// NTP Servers:
static const char ntpPool[][30] = { "time.google.com",
                                    "nl.pool.ntp.org",
                                    "0.nl.pool.ntp.org",
                                    "1.nl.pool.ntp.org",
                                    "3.nl.pool.ntp.org"
                                   };
static int        ntpPoolIndx = 0;

char              ntpServerName[50];

const int         NTP_PACKET_SIZE = 48; // NTP time is in the first 48 bytes of message
byte              packetBuffer[NTP_PACKET_SIZE]; //buffer to hold incoming & outgoing packets

static int        ntpServerNr = 0;
static bool       externalNtpTime = false;
static IPAddress  ntpServerIP; // NTP server's ip address


//=======================================================================
bool startNTP() 
{
  DebugTln(F("Starting UDP"));
  Udp.begin(localPort);
  DebugT(F("Local port: "));
  DebugTln(String(Udp.localPort()));
  DebugTln(F("waiting for NTP sync"));
  setSyncProvider(getNtpTime);
  setSyncInterval(60);  // seconds!
  if (timeStatus() == timeSet)      // time is set,
  {
    return true;                    // exit with time set
  }
  return false;

} // startNTP()


//=======================================================================
time_t getNtpTime() 
{
  DECLARE_TIMER_MS(waitForPackage, 1500);   

  while(true) 
  {
    yield;
    ntpPoolIndx++;
    if ( ntpPoolIndx > (sizeof(ntpPool) / sizeof(ntpPool[0]) -1) ) 
    {
      ntpPoolIndx = 0;
    }
    snprintf(ntpServerName, sizeof(ntpServerName), "%s", String(ntpPool[ntpPoolIndx]).c_str());

    while (Udp.parsePacket() > 0) ; // discard any previously received packets
    TelnetStream.println("Transmit NTP Request");
    // get a random server from the pool
    WiFi.hostByName(ntpServerName, ntpServerIP);
    TelnetStream.print(ntpServerName);
    TelnetStream.print(": ");
    TelnetStream.println(ntpServerIP);
    TelnetStream.flush();
    sendNTPpacket(ntpServerIP);
    RESTART_TIMER(waitForPackage);
    while (!DUE(waitForPackage))
    {
      int size = Udp.parsePacket();
      if (size >= NTP_PACKET_SIZE) 
      {
        //TelnetStream.print("Receive NTP Response: ");
        Udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
        unsigned long secsSince1900;
        // convert four bytes starting at location 40 to a long integer
        secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
        secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
        secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
        secsSince1900 |= (unsigned long)packetBuffer[43];
        time_t t = (secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR);
        snprintf(cMsg, sizeof(cMsg), "%02d:%02d:%02d", hour(t), minute(t), second(t));   
        TelnetStream.printf("[%s] Received NTP Response => new time [%s]  (Winter)\r\n", cMsg, cMsg);
        // return epoch ..
        return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      }
    }
    TelnetStream.println("No NTP Response :-(");

  } // while true ..
  
  return 0; // return 0 if unable to get the time

} // getNtpTime()


// send an NTP request to the time server at the given address
//=======================================================================
void sendNTPpacket(IPAddress &address) 
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  Udp.beginPacket(address, 123); //NTP requests are to port 123
  Udp.write(packetBuffer, NTP_PACKET_SIZE);
  Udp.endPacket();
  
} // sendNTPpacket()


//=======================================================================
time_t dateTime2Epoch(char const *date, char const *time) 
{
    char s_month[5];
    int year, day, h, m, s;
    tmElements_t t;

    static const char month_names[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

    if (sscanf(date, "%s %d %d", s_month, &day, &year) != 3) 
    {
      DebugTf("Not a valid date string [%s]\r\n", date);
      return 0;
    }
    t.Day  = day;
    // Find where is s_month in month_names. Deduce month value.
    t.Month = (strstr(month_names, s_month) - month_names) / 3 + 1;
    
  //DebugTf("date=>[%d-%02d-%02d]\r\n", t.Year, t.Month, t.Day);
    
    // Find where is s_month in month_names. Deduce month value.
    t.Month = (strstr(month_names, s_month) - month_names) / 3 + 1;

    if (sscanf(time, "%2d:%2d:%2d", &h, &m, &s) != 3) 
    {
      DebugTf("Not a valid time string [%s] (time set to '0')\r\n", time);
      h = 0;
      m = 0;
      s = 0;
    }
    t.Hour    = h;
    t.Minute  = m;
    t.Second  = s;
    
  //DebugTf("time=>[%02d:%02d:%02d]\r\n", t.Hour, t.Minute, t.Second);

    t.Year = CalendarYrToTm(year);
    
  //DebugTf("converted=>[%d-%02d-%02d @ %02d:%02d:%02d]\r\n", t.Year, t.Month, t.Day, t.Hour, t.Minute, t.Second);

    return makeTime(t);
    
} // dateTime2Epoch()
*/
#endif
/***************************************************************************
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the
* "Software"), to deal in the Software without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit
* persons to whom the Software is furnished to do so, subject to the
* following conditions:
*
* The above copyright notice and this permission notice shall be included
* in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
* OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT
* OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
* THE USE OR OTHER DEALINGS IN THE SOFTWARE.
* 
***************************************************************************/
