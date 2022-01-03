/* 
***************************************************************************  
**  Program  : processTelegram, part of DSMRlogger-Next
**  Version  : v2.3.0-rc5
**
**  Copyright (c) 2020 Willem Aandewiel
**
**  TERMS OF USE: MIT License. See bottom of file.                                                            
***************************************************************************      
*/
//==================================================================================
void processTelegram()
{
  DebugTf("Telegram[%d]=>DSMRdata.timestamp[%s]\r\n", telegramCount
                                                    , DSMRdata.timestamp.c_str());

//----- update OLED display ---------
  if (settingOledType > 0)
  {
    String DT   = buildDateTimeString(DSMRdata.timestamp.c_str(), sizeof(DSMRdata.timestamp));

    snprintf(cMsg, sizeof(cMsg), "%s - %s", DT.substring(0, 10).c_str(), DT.substring(11, 16).c_str());
    oled_Print_Msg(0, cMsg, 0);
    snprintf(cMsg, sizeof(cMsg), "-Power%7d Watt", (int)(DSMRdata.power_delivered *1000));
    oled_Print_Msg(1, cMsg, 0);
    snprintf(cMsg, sizeof(cMsg), "+Power%7d Watt", (int)(DSMRdata.power_returned *1000));
    oled_Print_Msg(2, cMsg, 0);
  }
                                                    
  strlcpy(newTimestamp, DSMRdata.timestamp.c_str(), sizeof(newTimestamp)); 
  DebugTf("Timestamp(now)=[%s]\r\n", newTimestamp);

  isDST = isdsmrDST(newTimestamp);
  //isDST = isdsmrDST(DSMRdata.timestamp.c_str());
  if (isDST)
    {
      DebugTln("Summertime");
    }
    else 
    {
      DebugTln("Wintertime");
    }

  //--- newTimestamp is the timestamp from the last telegram
  newT = epoch(newTimestamp, strlen(newTimestamp), true); // update system time
  //--- actTimestamp is the timestamp from the previous telegram
  actT = epoch(actTimestamp, strlen(actTimestamp), false);
  
  //--- Skip first 3 telegrams .. just to settle down a bit ;-)
  if ((int32_t)(telegramCount - telegramErrors) < 3) 
  {
    strlcpy(actTimestamp, newTimestamp, sizeof(actTimestamp));
    actT = epoch(actTimestamp, strlen(actTimestamp), false);   // update system time
    return;
  }
  
  DebugTf("epoch actHour[%d] -- newHour[%d]\r\n", (int)actT, (int)newT);
  DebugTf("Hour(local TZ)  actHour[%02d] -- newHour[%02d]\r\n", hour(actT), hour(newT));
   //--- if we have a new hour() update the previous hour
  if (hour(actT) != hour(newT)) {
    writeToSysLog("actHour[%02d] -- newHour[%02d]", hour(actT), hour(newT));
  }


//fix Rob Roos
//--- has the hour changed (or the day or month)  
  //--- in production testing on hour only would
  //--- suffice, but in testing I need all three
  //--- if we have a new hour() update the previous hour(actT)
  if (     (HourFromTimestamp(actTimestamp) != HourFromTimestamp(newTimestamp)  ) 
       ||   (DayFromTimestamp(actTimestamp) != DayFromTimestamp(newTimestamp)   ) 
       || (MonthFromTimestamp(actTimestamp) != MonthFromTimestamp(newTimestamp) ) )
  {
    writeToSysLog("Update RING-files");
    writeDataToFiles();
    writeLastStatus();
    //--- now see if the day() has changed also
    if ( DayFromTimestamp(actTimestamp) != DayFromTimestamp(newTimestamp) )
    {
      //--- YES! actTimestamp := newTimestamp
      //--- and update the files with the actTimestamp
      strlcpy(actTimestamp,  newTimestamp, sizeof(actTimestamp));
      writeDataToFiles();
    }
    else  //--- NO, only the hour has changed
    {
      char      record[DATA_RECLEN + 1] = "";
      //--- actTimestamp := newTimestamp
      strlcpy(actTimestamp, newTimestamp, sizeof(actTimestamp));

      buildDataRecordFromSM(record);
      uint16_t recSlot = timestampToHourSlot(actTimestamp, strlen(actTimestamp));
      //--- and update the files with the actTimestamp
      writeDataToFile(HOURS_FILE, record, recSlot, HOURS);
      DebugTf(">%s\r\n", record); // record ends in a \n
    }
  } 

//fix


//old code: before fix Rob Roos

  // //--- has the hour changed (or the day or month)  
  // //--- in production testing on hour only would
  // //--- suffice, but in testing I need all three
  // //--- if we have a new hour() update the previous hour(actT)
  // if (     (hour(actT) != hour(newT)  ) 
  //      ||   (day(actT) != day(newT)   ) 
  //      || (month(actT) != month(newT) ) )
  // {
  //   writeToSysLog("Update RING-files");
  //   writeDataToFiles();
  //   writeLastStatus();
  //   //--- now see if the day() has changed also
  //   if ( day(actT) != day(newT) )
  //   {
  //     //--- YES! actTimestamp := newTimestamp
  //     //--- and update the files with the actTimestamp
  //     strlcpy(actTimestamp,  newTimestamp, sizeof(actTimestamp));
  //     writeDataToFiles();
  //   }
  //   else  //--- NO, only the hour has changed
  //   {
  //     char      record[DATA_RECLEN + 1] = "";
  //     //--- actTimestamp := newTimestamp
  //     strlcpy(actTimestamp, newTimestamp, sizeof(actTimestamp));
  //     buildDataRecordFromSM(record);
  //     uint16_t recSlot = timestampToHourSlot(actTimestamp, strlen(actTimestamp));
  //     //--- and update the files with the actTimestamp
  //     writeDataToFile(HOURS_FILE, record, recSlot, HOURS);
  //     DebugTf(">%s\r\n", record); // record ends in a \n
  //   }
  // }

//old code: before fix Rob Roos

  strlcpy(actTimestamp, newTimestamp, sizeof(actTimestamp));
  actT = epoch(actTimestamp, strlen(actTimestamp), true);   // update system time

  //isdsmrDST(actTimestamp, strlen(actTimestamp));

// If the MQTT timer is DUE, also send MQTT message
#ifdef USE_MQTT
  if ( DUE(publishMQTTtimer) )
  {
    sendMQTTData();      
  }  
#endif
// And send it using InfluxDB
#ifdef USE_INFLUXDB
    handleInfluxDB();
#endif



} // processTelegram()


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
