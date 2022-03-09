/*
***************************************************************************  
**  Program  : DSMRlogger-Next.h - definitions for DSMRlogger-Next
**  Version  : v2.3.0-rc5
**
**  Copyright (c) 2020 Robert van den Breemen
**
**  Based on the original:
**          DSMRLoggerAPI - Copyright (c) 2020 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/  

#if defined(ESP8266)
  #define ESP_RESET()             ESP.reset()
  #define ESP_GET_FREE_BLOCK()    ESP.getMaxFreeBlockSize()
  #define ESP_GET_CHIPID()        ESP.getChipId()
  const char *flashMode[]         { "QIO", "QOUT", "DIO", "DOUT", "Unknown" };
//    #define LED_ON      LOW
//    #define LED_OFF     HIGH
  #ifdef USE_REQUEST_PIN
    #define DTR_ENABLE  12
  #endif
  #define SM_SERIAL Serial
  #include <LittleFS.h>

#elif defined(ESP32)
  #define ESP_RESET()             ESP.restart()
  #define ESP_GET_FREE_BLOCK()    ESP.getMaxAllocHeap()
  #define ESP_GET_CHIPID()        ((uint32_t)ESP.getEfuseMac()) //The chipID is essentially its MAC address (length: 6 bytes) 
  const char *flashMode[]         { "QIO", "QOUT", "DIO", "DOUT", "FAST READ", "SLOWREAD", "Unknown" };
  
  #include <rom/rtc.h>          // SDK ESP32 for reset reason function (see helper function)
  // ESP32 JDJ REV2 
  // LED PIN  24              //GPIO  --  data pin for WS2812B pixel
  // RX  PIN  16              //GPIO  -- UART2 RX -- connected to DSMR
  // TX  PIN  17              //GPIO  -- UART2 TX -- n.c.
  // DTR PIN  27              //GPIO  -- DTR Enable for Smart slimmeMeter
  // Extra pins for IO - uses - eg. S0 pulse counter or whatever you like
  // PINS     26, 25, 32, 33  //GPIO
  // JTAG HEADER available for debug
    // ESP JDJ REV1
  //#define RXD2 13         //GPIO13 for prototype (rev 1)
  //#define TXD2 1          //GPIO1 for prototype
  // ESP32 JDJ REV2 
  // LED PIN  24              //GPIO  --  data pin for WS2812B pixel
  // RX  PIN  16              //GPIO  -- UART2 RX -- connected to DSMR
  // TX  PIN  17              //GPIO  -- UART2 TX -- n.c.
  #define RXD2 16           //GPIO16 for rev 2   
  #define TXD2 17           //GPIO17 for rev 2   
  #define SM_SERIAL Serial2
  #ifdef USE_REQUEST_PIN
    #define DTR_ENABLE  27
  #endif

 // #include <LITTLEFS.h>        // with ESP32 1.0.6
 // #define LittleFS LITTLEFS  // with ESP32 1.0.6
   #include <LittleFS.h>  // with ESP32 2.0.4 
#endif

#include <AceTime.h>
//Use acetime
using namespace ace_time;
static BasicZoneProcessor timeProcessor;
static const int CACHE_SIZE = 3;
// static BasicZoneManager<CACHE_SIZE> manager(zonedb::kZoneRegistrySize, zonedb::kZoneRegistry);
static BasicZoneProcessorCache<CACHE_SIZE> zoneProcessorCache;
static BasicZoneManager timezoneManager(zonedb::kZoneRegistrySize, zonedb::kZoneRegistry, zoneProcessorCache);


#include <TimeLib.h>            // https://github.com/PaulStoffregen/Time
#include <TelnetStream.h>       // https://github.com/jandrassy/TelnetStream/commit/1294a9ee5cc9b1f7e51005091e351d60c8cddecf
#include "safeTimers.h"

#ifdef USE_SYSLOGGER
  #include "ESP_SysLogger.h"      // https://github.com/mrWheel/ESP_SysLogger
  ESPSL sysLog;                   // Create instance of the ESPSL object
  #define writeToSysLog(...) ({ sysLog.writeDbg( sysLog.buildD("[%02d:%02d:%02d][%7d][%-12.12s] " \
                                                               , hour(), minute(), second()     \
                                                               , ESP.getFreeHeap()              \
                                                               , __FUNCTION__)                  \
                                                               ,__VA_ARGS__); })
#else
  #define writeToSysLog(...)  // nothing
#endif

//  https://github.com/matthijskooijman/arduino-dsmr
#include <dsmr.h>               // Version Jan 2022 !!!

#define _DEFAULT_HOSTNAME  "DSMR-API"  

#define SETTINGS_FILE      "/DSMRsettings.ini"

#define LED_ON            LOW
#define LED_OFF          HIGH
#define FLASH_BUTTON        0
#define MAXCOLORNAME       15
#define JSON_BUFF_MAX     255
#define MQTT_BUFF_MAX     200

#define CSTR(x) x.c_str()


//-------------------------.........1....1....2....2....3....3....4....4....5....5....6....6....7....7
//-------------------------1...5....0....5....0....5....0....5....0....5....0....5....0....5....0....5
#define DATA_FORMAT       "%-8.8s;%10.3f;%10.3f;%10.3f;%10.3f;%10.3f;\n"
#define DATA_CSV_HEADER   "YYMMDDHH;      EDT1;      EDT2;      ERT1;      ERT2;       GDT;"
#define DATA_RECLEN       75

#define HOURS_FILE        "/RINGhours.csv"
#define _NO_HOUR_SLOTS_   (48 +1)

#define DAYS_FILE         "/RINGdays.csv"
#define _NO_DAY_SLOTS_    (14 +1)

#define MONTHS_FILE       "/RINGmonths.csv"
#define _NO_MONTH_SLOTS_  (24 +1)

enum    { PERIOD_UNKNOWN, HOURS, DAYS, MONTHS, YEARS };

//prototype esp helper
void esp_reboot();
uint32_t esp_get_free_block();

#include "Debug.h"
#include "espHelper.h"
#include "oledStuff.h"
#include "networkStuff.h"

/**
 * Define the DSMRdata we're interested in, as well as the DSMRdatastructure to
 * hold the parsed DSMRdata. This list shows all supported fields, remove
 * any fields you are not using from the below list to make the parsing
 * and printing code smaller.
 * Each template argument below results in a field of the same name.
 */
using MyData = ParsedData<
  /* String */         identification
  /* String */        ,p1_version
  /* String */        ,timestamp
  /* String */        ,equipment_id
  /* FixedValue */    ,energy_delivered_tariff1
  /* FixedValue */    ,energy_delivered_tariff2
  /* FixedValue */    ,energy_returned_tariff1
  /* FixedValue */    ,energy_returned_tariff2
  /* String */        ,electricity_tariff
  /* FixedValue */    ,power_delivered
  /* FixedValue */    ,power_returned
  /* FixedValue */    ,electricity_threshold
  /* uint8_t */       ,electricity_switch_position
  /* uint32_t */      ,electricity_failures
  /* uint32_t */      ,electricity_long_failures
  /* String */        ,electricity_failure_log
  /* uint32_t */      ,electricity_sags_l1
  /* uint32_t */      ,electricity_sags_l2
  /* uint32_t */      ,electricity_sags_l3
  /* uint32_t */      ,electricity_swells_l1
  /* uint32_t */      ,electricity_swells_l2
  /* uint32_t */      ,electricity_swells_l3
  /* String */        ,message_short
  /* String */        ,message_long
  /* FixedValue */    ,voltage_l1
  /* FixedValue */    ,voltage_l2
  /* FixedValue */    ,voltage_l3
  /* FixedValue */    ,current_l1
  /* FixedValue */    ,current_l2
  /* FixedValue */    ,current_l3
  /* FixedValue */    ,power_delivered_l1
  /* FixedValue */    ,power_delivered_l2
  /* FixedValue */    ,power_delivered_l3
  /* FixedValue */    ,power_returned_l1
  /* FixedValue */    ,power_returned_l2
  /* FixedValue */    ,power_returned_l3
  /* uint16_t */      ,gas_device_type
  /* String */        ,gas_equipment_id
  /* uint8_t */       ,gas_valve_position
  /* TimestampedFixedValue */ ,gas_delivered
  /* uint16_t */      ,thermal_device_type
  /* String */        ,thermal_equipment_id
  /* uint8_t */       ,thermal_valve_position
  /* TimestampedFixedValue */ ,thermal_delivered
  /* uint16_t */      ,water_device_type
  /* String */        ,water_equipment_id
  /* uint8_t */       ,water_valve_position
  /* TimestampedFixedValue */ ,water_delivered
  /* uint16_t */      ,slave_device_type
  /* String */        ,slave_equipment_id
  /* uint8_t */       ,slave_valve_position
  /* TimestampedFixedValue */ ,slave_delivered
>;

enum    { TAB_UNKNOWN, TAB_ACTUEEL, TAB_LAST24HOURS, TAB_LAST7DAYS, TAB_LAST24MONTHS, TAB_GRAPHICS, TAB_SYSINFO, TAB_EDITOR };

typedef struct {
    uint32_t  Label;
    float     EDT1;
    float     EDT2;
    float     ERT1;
    float     ERT2;
    float     GDT;
} dataStruct;

static dataStruct hourData;   // 0 + 1-24
static dataStruct dayData;    // 1 - 7 (0=header, 1=sunday)
static dataStruct monthData;  // 0 + year1 1 t/m 12 + year2 1 t/m 12

const char *weekDayName[]  { "Unknown", "Zondag", "Maandag", "Dinsdag", "Woensdag"
                            , "Donderdag", "Vrijdag", "Zaterdag", "Unknown" };
const char *monthName[]    { "00", "Januari", "Februari", "Maart", "April", "Mei", "Juni", "Juli"
                            , "Augustus", "September", "Oktober", "November", "December", "13" };
//const char *flashMode[]    { "QIO", "QOUT", "DIO", "DOUT", "Unknown" };

/**
struct FSInfo {
    size_t totalBytes;
    size_t usedBytes;
    size_t blockSize;
    size_t pageSize;
    size_t maxOpenFiles;
    size_t maxPathLength;
};
**/
#ifdef DTR_ENABLE
  P1Reader    slimmeMeter(&SM_SERIAL, DTR_ENABLE);
#else
  P1Reader    slimmeMeter(&SM_SERIAL, 0);
#endif

//===========================prototype's=======================================
int strcicmp(const char *a, const char *b);
void delayms(unsigned long);

//===========================GLOBAL VAR'S======================================
  WiFiClient  wifiClient;
  MyData      DSMRdata;
  uint32_t    readTimer;
  time_t      actT, newT;
  char        actTimestamp[20] = "";
  char        newTimestamp[20] = "";
  uint32_t    slotErrors = 0;
  uint32_t    nrReboots  = 0;
  uint32_t    loopCount = 0;
  uint32_t    telegramCount = 0, telegramErrors = 0;
  bool        showRaw = false;
  int8_t      showRawCount = 0;
  char      cMsg[150], fChar[10];

#ifdef USE_MQTT
  //  https://github.com/knolleary/pubsubclient
  #include <PubSubClient.h>           // MQTT client publish and subscribe functionality
  
  static PubSubClient MQTTclient(wifiClient);
#endif

#ifdef USE_MINDERGAS
  static char      settingMindergasToken[21] = "";
  static uint16_t  intStatuscodeMindergas    = 0; 
  static char      txtResponseMindergas[30]  = "";
  static char      timeLastResponse[16]      = "";  
#endif

#ifdef USE_INFLUXDB
  char      settingInfluxDBhostname[101] = "";
  uint16_t  settingInfluxDBport = 8086;
  char      settingInfluxDBdatabasename[30] = "";
#endif


String    lastReset           = "";
bool      LittleFSNotPopulated  = false;
bool      hasAlternativeIndex = false;
bool      mqttIsConnected     = false;
bool      Verbose1 = false, Verbose2 = false;
int8_t    thisHour = -1, prevNtpHour = 0, thisDay = -1, thisMonth = -1, lastMonth, thisYear = 15;
uint32_t  unixTimestamp;
uint64_t  upTimeSeconds;
IPAddress ipDNS, ipGateWay, ipSubnet;
float     settingEDT1, settingEDT2, settingERT1, settingERT2, settingGDT;
float     settingENBK, settingGNBK;
uint8_t   settingTelegramInterval;
uint8_t   settingSmHasFaseInfo = 1;
char      settingHostname[30];
char      settingIndexPage[50];
char      settingMQTTbroker[101], settingMQTTuser[40], settingMQTTpasswd[30], settingMQTTtopTopic[21];
int32_t   settingMQTTinterval, settingMQTTbrokerPort;
String    pTimestamp;
bool      isDST = false;


//SolarEdge ModbusTCP
// #include <ModbusIP_ESP8266.h>
// const int Modbus_SolarEdge_Reg_offset = 40069;                // Modbus Hreg Offset
// const int Modbus_SolarEdge_Reg_number = 39;
// IPAddress Modbus_SolarEdge_IP(192, 168, 88, 22);    // Address of Modbus Slave device
// const int Modbus_SolarEdge_port = 1502;             // Portnumber
// ModbusIP  Modbus_SolarEdge;                         //ModbusIP object

//===========================================================================================
// setup timers 
DECLARE_TIMER_SEC(updateSeconds,       1, CATCH_UP_MISSED_TICKS);
DECLARE_TIMER_SEC(updateDisplay,       5);
DECLARE_TIMER_MIN(reconnectWiFi,      30);
DECLARE_TIMER_MIN(synchrNTP,          10, SKIP_MISSED_TICKS);
DECLARE_TIMER_SEC(nextTelegram,       10, CATCH_UP_MISSED_TICKS);
DECLARE_TIMER_MIN(reconnectMQTTtimer,  2); // try reconnecting cyclus timer
DECLARE_TIMER_SEC(publishMQTTtimer,   60, CATCH_UP_MISSED_TICKS); // interval time between MQTT messages  
DECLARE_TIMER_MIN(minderGasTimer,     1, CATCH_UP_MISSED_TICKS);  // once minute
DECLARE_TIMER_SEC(antiWearTimer,      61);

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
