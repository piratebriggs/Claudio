/**
 * GLOBAL PIN CONFIGURATION
 */
const int DHT_OUT = 3;

/**
 * EEPROM libraries and resources
 */
#include "EEPROM.h"
#define EEPROM_SIZE 64
 
/**
 * DHT-11 Temp and humidity sensor  libraries and resources
 */
#include "DHT.h"
#define DHTTYPE DHT11
DHT dht(DHT_OUT, DHTTYPE);       

/**
 * LCD over I2C via PCF8574
 */
#include <Wire.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h> // include i/o class header

hd44780_I2Cexp lcd; // declare lcd object: auto locate & config display for hd44780 chip

/**    
 *  WIFI Libraries and resources   
 */
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
//Absolute path to file containing WiFi credentials
//const char* ssid       = "MyApSSID";
//const char* password   = "MyApPassphrase";
#include <wifi_credentials.h>
const int connTimeout = 10; //Seconds

WiFiServer wifiServer(1234);

/** 
 *  WS2812
 */
#include <NeoPixelBus.h>

const uint16_t PixelCount = 8; // this example assumes 4 pixels, making it smaller will cause a failure
NeoPixelBus<NeoGrbFeature, NeoEsp8266Dma800KbpsMethod> strip(PixelCount, 1);
RgbColor red(255, 0, 0);

/** 
 *  RGB
 */

const uint8_t PinR = 13;
const uint8_t PinG = 14;
const uint8_t PinB = 12;


/** 
 *  TIME libraries and resources
 */
#include "time.h"
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 0;
const int   daylightOffset_sec = 0;
 
/**
 * PWM Constants
 */
const int freq = 5000;
const int tftledChannel = 0;
const int resolution = 8;

 
/**
 * GLOBALS
 */
String   prevTime = "";
String   currTime = "";
String   prevDate = "";
String   currDate = ""; 
uint16_t prevLux = 0;
uint16_t currLux = 0;
float    prevTemp = 0;
float    currTemp = 0;
float    prevHumi = 0;
float    currHumi = 0;
bool     onWifi = false;
String   weekDays[] = {"", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
String   months[] = {"", " Jan ", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
int      pinTouchR = 14; 
int      pinTouchM = 13; 
int      pinTouchL = 12;
int      statusTouchR = 0;
int      statusTouchM = 0;
int      statusTouchL = 0;
ulong    lastTouchR;
ulong    lastTouchM;
ulong    lastTouchL;
bool     doAlarm;
String   alarmTime;
bool     onAlarm;
bool     waitingToTouch = false;
bool     onConfig = false;

//Configurable values
int      bootWait; //Seconds
int      touchReadings;
int      thresholdTouchR; 
int      thresholdTouchM; 
int      thresholdTouchL;
int      longTouchThreshold; // Tenths of second
int      snoozeMinutes;
int      alarmBeeps;
byte     maxLux;
byte     minLux;
byte     minBrightness;



void setup() {
  /**
   * Serial port
   */
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  pinMode(PinR, OUTPUT);
  pinMode(PinG, OUTPUT);
  pinMode(PinB, OUTPUT);

  analogWrite(PinR, 0);
  analogWrite(PinG, 0);
  analogWrite(PinB, 0);

  delay(1000);
  analogWrite(PinR, 255/2);
  analogWrite(PinG, 0);
  analogWrite(PinB, 0);

  delay(1000);
  analogWrite(PinR, 0);
  analogWrite(PinG, 255/2);
  analogWrite(PinB, 0);

  delay(1000);
  analogWrite(PinR, 0);
  analogWrite(PinG, 0);
  analogWrite(PinB, 255/2);

  /**
   * EEPROM
   */
  //EEPROM.begin(EEPROM_SIZE);
  /**
   * Loads EEPROM configuration
   */
  //loadConfiguration();
  
  /**
   * TFT DISPLAY
   */  
  lcd.begin(16,2);               // initialize the lcd 
  lcd.home ();                   // go home

  /**
   * Pixels
   */  
  // this resets all the neopixels to an off state
  strip.Begin();
  strip.Show();
  for(int i =0; i<8; i++){
     strip.SetPixelColor(i, red);
  }
  strip.Show();
    
  /**
   * Temperature and humidity sensor
   */
  dht.begin();    
//  tft.print("Connecting to WiFi AP "); 
//  tft.println(ssid);     
  /**
   * Wifi connect
   */
  wifiConnect();
  if(onWifi == true){
//    tft.print("   Connection succeed, obtained IP ");
//    tft.println(WiFi.localIP());
  }else{
    //tft.println("   Connection failed. Unexpected operation results.");
  }
  Serial.printf("Obtaining NTP time from remote server...");
  /**
   * NTP Time
   */
  getNtpTime();
  delay(100); //We need a delay to allow info propagation
  
  /**
   * Wifi Server for configuration
   */
  Serial.println("Starting remote configuration server...");
  wifiServer.begin();

//  tft.println("End of booting process.");
   

}

void loop() {
  getCurrentTemp();
  getCurrentHumi();
  refreshTime();
  //configMode();
  ArduinoOTA.handle();
  delay(1000);
}

void setupOTA() {
  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname(ota_hostname);

  // No authentication by default
  ArduinoOTA.setPassword(ota_password);

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

/**
 * Checks for next alarm in queue
 */
void checkAlarm(){
  //TODO: Check in queue what is the nex alarm to be activated...
  alarmTime = hourMinuteToTime(getAlarmHour(), getAlarmMinute()); 
  doAlarm = true;
  displayAlarm(); 
}
/**
 * Connects to WIFI
 */

bool wifiConnect(){
  onWifi = false;
  int retries = 0;
  yield();
  Serial.printf("Connecting to %s ", ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      retries++;
      if(retries == (connTimeout * 2)){
        Serial.println(" TIMEOUT");
        break;
      }
  }
  if(WiFi.status() == WL_CONNECTED){
    onWifi = true;
    Serial.println(" CONNECTED");
  }
  return onWifi;
}
/**
 * Obtains time from NTP server
 */
bool getNtpTime(){
  bool result = false;
  if(onWifi == true){
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    result = true;
  }else{
    Serial.println("getNtpTime: Not connected to wifi!"); 
  }
  return result;
}

/**
 * Returns day of week (1-Mon, 2-Tue, ...., 7-Sun)
 */
int getDayOfWeek(){ //1-Mon, 2-Tue, ...., 7-Sun
  int wDay;
  time_t now;
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now); 
  wDay = timeinfo->tm_wday;
  if(wDay == 0){
    wDay = 7;
  }
  return wDay;
}
/**
 * Adds a number of snooze minutes to s String time
 */
String addSnooze(String sTime, int snooze){
  int hour;
  int minute;
  int tMinutes;
  int tHours;
  String newTime;
  char cTime[5]=" ";
  hour = sTime.substring(0,2).toInt();
  minute = sTime.substring(3,5).toInt();
  tMinutes = (minute+snooze) % 60;
  tHours = (hour + ((minute+snooze) / 60)) % 24;

  sprintf(cTime, "%02d:%02d", tHours, tMinutes); 
  newTime = (char*)cTime;
  return newTime;
}
/**
 * Returns a string formatted HH:MM based on hours and minutes
 */
String hourMinuteToTime(int hour, int minute){
  String sTime;
  char cTime[5]=" ";
  sprintf(cTime, "%02d:%02d", hour, minute); 
  sTime = (char*)cTime; 
  return sTime;
}
/*
 * Returns current time in HH:MM format
 */
void refreshTime(){
  //Time
  time_t now;
  struct tm * timeinfo;
  time(&now);
  timeinfo = localtime(&now); 
  prevTime = currTime;
  currTime = hourMinuteToTime(timeinfo->tm_hour, timeinfo->tm_min); 
  if(prevTime != currTime){
    timeChanged(prevTime, currTime);
    //If time has changed, lets check if date has changed too
    //Date
    int wDay;
    wDay = timeinfo->tm_wday;
    if(wDay == 0){
      wDay = 7;
    }
    String calDate = "";
    char cDate[30]=" ";
    //Serial.println(weekDays[wDay]);
    //sprintf(cDate, "%s, %i de %s de %i", weekDays[wDay], timeinfo->tm_mday, months[(timeinfo->tm_mon + 1)],(timeinfo->tm_year + 1900)); 
    calDate = calDate + weekDays[wDay];
    calDate = calDate + ", ";
    calDate = calDate + timeinfo->tm_mday;
    calDate = calDate + " ";
    calDate = calDate + months[(timeinfo->tm_mon + 1)];
    calDate = calDate + " ";    
    calDate = calDate + (timeinfo->tm_year + 1900);
    if(calDate.length() == 21){
      calDate = " " + calDate;
    }
    prevDate = currDate;
    currDate = calDate;
    if(prevDate != currDate){
      dateChanged(prevDate, currDate);
    }
  }
}

/**
 * Displays time string erasing the previous one
 */
void displayTime(){
  yield();
  lcd.home ();                   // go home
  lcd.print(currTime);
}

/**
 * Displays date string 
 */
void displayDate(){
  lcd.setCursor ( 0, 1 );        // go to the next line
  lcd.print (currDate);
}

/**
 * Displays temperature
 */
void displayTemp(){
/*  int bgColor = 0;
  if(currTemp == prevTemp){
    bgColor = ILI9341_LGREEN;
  }else if(currTemp < prevTemp){
    bgColor = ILI9341_LCYAN;
  }else{
    bgColor = ILI9341_LORANGE;
  }
  
  tft.setTextColor(ILI9341_BLACK);
  yield();
  tft.fillRect(14, 170, 92, 60, bgColor);
  tft.drawRect(14, 170, 92, 60, ILI9341_WHITE);
  yield();
  tft.setTextSize(2);
  tft.setCursor(34, 174);
  tft.print("TEMP");
  tft.setTextSize(3);
  tft.setCursor(16, 200);
  tft.print(currTemp);*/
}
/**
 * Displays relative humidity
 */
void displayHumi(){
/*  int bgColor = 0;
  if(currHumi == prevHumi){
    bgColor = ILI9341_LGREEN;
  }else if(currHumi < prevHumi){
    bgColor = ILI9341_LCYAN;
  }else{
    bgColor = ILI9341_LORANGE;
  }
  tft.setTextColor(ILI9341_BLACK);
  yield();
  tft.fillRect(113, 170, 92, 60, bgColor);
  tft.drawRect(113, 170, 92, 60, ILI9341_WHITE);
  yield();
  tft.setTextSize(2);
  tft.setCursor(135, 174);
  tft.print("H.R.");
  tft.setTextSize(3);
  tft.setCursor(115, 200);
  tft.print(currHumi);*/
}
/**
 * Displays Alarm Time 
 */
void displayAlarm(){
/*  if(doAlarm == true){
    tft.setTextColor(ILI9341_YELLOW);
  }else{
    tft.setTextColor(ILI9341_LGRAY);
  }
  yield();
  tft.fillRect(212, 129, 92, 36, ILI9341_BLACK);
  yield();
  tft.setTextSize(3);
  tft.setCursor(214, 133);
  tft.print(alarmTime);*/
}
/**
 * Returns temperature
 */
float getCurrentTemp(){
  prevTemp = currTemp;
  currTemp = dht.readTemperature();;
  if(prevTemp != currTemp){
    tempChanged(prevTemp , currTemp);
  }
  return currTemp;
}
/**
 * Returns humidity
 */
float getCurrentHumi(){
  prevHumi = currHumi;
  currHumi = dht.readHumidity();
  if(prevHumi != currHumi){
    humiChanged(prevHumi , currHumi);
  }
  return currHumi;
}
/**
 *  EVENTS
 */ 
/**
 * Event for change of time HH:MM
 */
void timeChanged(String prevTime, String currTime){
  Serial.println("timeChanged event fired!");
  displayTime();
  if(currTime == alarmTime){
    if(doAlarm == true){
      if(onAlarm == false){
        //playBuzzer(alarmBeeps);
        doAlarm = false;
      }
    }
  }
}
/**
 * Event for change of date weekDay, day de Month de Year
 */
void dateChanged(String prevDate, String currDate){
  Serial.print("dateChanged event fired! ");
  Serial.println(currDate);
  displayDate();
  //New day, lets check for the next alarm
  checkAlarm();
}

/**
 * Event for change of temperature
 */
void tempChanged(float prevTemp, float currTemp){
  Serial.print("tempChanged event fired! ");
  Serial.println(currTemp);
  displayTemp();
}

/**
 * Event for change of humidity
 */
void humiChanged(float prevHumi, float currHumi){
  Serial.println("humiChanged event fired!");
  Serial.println(currHumi);
  displayHumi();
}

void configOn(){
  Serial.println("Entered config mode");
}
void configOff(){
  Serial.println("Exited config mode");
}


/**
 * Remote configuration
 */
void configMode(){
  String instruction = "";
  String reply = "";
  WiFiClient client = wifiServer.available();
  if (client) {
    onConfig = true;
    configOn();
    while (client.connected()) {
      while (client.available()>0) {
        char c = client.read();
        if(c == 10){
          reply = configExecute(instruction);
          instruction = "";
          client.println(reply);
        }else{
          instruction = instruction + c;
        }
        
      }
      delay(10);
    }
    client.stop();
    onConfig = false;
    configOff();
  }  
} 
String configExecute(String instruction){
  String command;
  String item;
  String value;
  int setValue;
  command = instruction.substring(0, 3);
  item = instruction.substring(3, 9);
  Serial.print("Remote config: ");Serial.println(instruction);
  if(command == "GET"){
      value = getConfigValue(item);
  }else if(command == "SET"){
      setValue = instruction.substring(9, 12).toInt();
      value = setConfigValue(item, setValue);
  }else if(command == "ALL"){
      value = getAllItems();   
  }else if(command == "RST"){
      ESP.restart();  
  }else if(command == "TSS"){
      onAlarm = false;
      doAlarm = false;  
      displayAlarm();
  }else{
      value = "Invalid command";
  }
  return value;
}
String getConfigValue(String item){
  String value;
  if(item == "BOOTWT"){
    value = (String) getBootWait();
  }else if(item == "TOUCHR"){
    value = (String) getTouchReadings();
  }else if(item == "THTCHR"){
    value = (String) getThresholdTouchR();
  }else if(item == "THTCHM"){
    value = (String) getThresholdTouchM();
  }else if(item == "THTCHL"){
    value = (String) getThresholdTouchL();
  }else if(item == "LTCHTH"){
    value = (String) getLongTouchThreshold();
  }else if(item == "SNZMIN"){
    value = (String) getSnoozeMinutes();
  }else if(item == "ALBEEP"){
    value = (String) getAlarmBeeps();
  }else if(item == "MAXLUX"){
    value = (String) getMaxLux();
  }else if(item == "MINLUX"){
    value = (String) getMinLux();
  }else if(item == "MINBRG"){
    value = (String) getMinBrightness();
  }else if(item == "ALARHO"){
    value = (String) getAlarmHour();
  }else if(item == "ALARMI"){
    value = (String) getAlarmMinute();
  }else if(item == "DOALAR"){
    value = (String) getDoAlarm();  
  }else if(item == "ONALAR"){
    value = (String) getOnAlarm();    
  }else{
    value = "Invalid item";
  }  
  return value;
}
String setConfigValue(String item, int setValue){
  String value = "OK";
  if(item == "BOOTWT"){
    setBootWait(setValue);
  }else if(item == "TOUCHR"){
    setTouchReadings(setValue);
  }else if(item == "THTCHR"){
    setThresholdTouchR(setValue);
  }else if(item == "THTCHM"){
    setThresholdTouchM(setValue);
  }else if(item == "THTCHL"){
    setThresholdTouchL(setValue);
  }else if(item == "LTCHTH"){
    setLongTouchThreshold(setValue);
  }else if(item == "SNZMIN"){
    setSnoozeMinutes(setValue);
  }else if(item == "ALBEEP"){
    setAlarmBeeps(setValue);
  }else if(item == "MAXLUX"){
    setMaxLux(setValue);
  }else if(item == "MINLUX"){
    setMinLux(setValue);
  }else if(item == "MINBRG"){
    setMinBrightness(setValue);
  }else if(item == "ALARHO"){
    setAlarmHour(setValue);
  }else if(item == "ALARMI"){
    setAlarmMinute(setValue);
  }else if(item == "DOALAR"){
    setDoAlarm(setValue);
  }else if(item == "ONALAR"){
    setOnAlarm(setValue);    
  }else{
    value = "Invalid item";
  }  
  return value;
}
String getAllItems(){
  String result = "";
  result = result + "BOOTWT";
  result = result + (String) getBootWait();
  result = result + ";";
  result = result + "TOUCHR";
  result = result + (String) getTouchReadings();
  result = result + ";";
  result = result + "THTCHR";
  result = result + (String) getThresholdTouchR();
  result = result + ";";
  result = result + "THTCHM";
  result = result + (String) getThresholdTouchM();
  result = result + ";";
  result = result + "THTCHL";
  result = result + (String) getThresholdTouchL();
  result = result + ";";
  result = result + "LTCHTH";
  result = result + (String) getLongTouchThreshold();
  result = result + ";";
  result = result + "SNZMIN";
  result = result + (String) getSnoozeMinutes();
  result = result + ";";
  result = result + "ALBEEP";
  result = result + (String) getAlarmBeeps();
  result = result + ";";
  result = result + "MAXLUX";
  result = result + (String) getMaxLux();
  result = result + ";";
  result = result + "MINLUX";
  result = result + (String) getMinLux();
  result = result + ";";
  result = result + "MINBRG";
  result = result + (String) getMinBrightness();
  result = result + ";";
  result = result + "ALARHO";
  result = result + (String) getAlarmHour();
  result = result + ";";
  result = result + "ALARMI";
  result = result + (String) getAlarmMinute();
  result = result + ";";
  result = result + "DOALAR";
  result = result + (String) getDoAlarm();   
  result = result + ";";
  result = result + "ONALAR";
  result = result + (String) getOnAlarm();   
  result = result + ";";
  result = result + "CURTIM";
  result = result + currTime;   
  result = result + ";";
  result = result + "CURDAT";
  result = result + currDate;   
  result = result + ";";
  result = result + "ALARTM";
  result = result + alarmTime;   
  result = result + ";";
  result = result + "CURTMP";
  result = result + (String) currTemp;   
  result = result + ";";
  result = result + "CURHUM";
  result = result + (String) currHumi;   
  result = result + ";";
  result = result + "CURLUX";
  result = result + (String) currLux;   
  result = result + ";";
  return result;
}

/**
 * Load Configuration from EEPROM
 */
void loadConfiguration(){
  bootWait = getBootWait();
  touchReadings = getTouchReadings();
  thresholdTouchR = getThresholdTouchR();
  thresholdTouchM = getThresholdTouchM(); 
  thresholdTouchL = getThresholdTouchL();
  longTouchThreshold = getLongTouchThreshold();
  snoozeMinutes = getSnoozeMinutes();
  alarmBeeps = getAlarmBeeps();
  maxLux = getMaxLux();
  minLux = getMinLux();
  minBrightness = getMinBrightness(); 
}
/**
 * Boot Wait
 * Address 0
 */
byte getBootWait(){
  byte value = byte(EEPROM.read(0));
  if(value == 255){
    value = 20;
  }
  return value;
}
void setBootWait(byte value){
  EEPROM.write(0, value);
  EEPROM.commit();
  bootWait = value;
}
/* Touch readings
 * Address 1
 */
byte getTouchReadings(){
  byte value = byte(EEPROM.read(1));
  if(value == 255){
    value = 120;
  }
  return value;
}
void setTouchReadings(byte value){
  EEPROM.write(1, value);
  EEPROM.commit();
  touchReadings = value;
}
/* thresholdTouchR
 * Address 2
 */
byte getThresholdTouchR(){
  byte value = byte(EEPROM.read(2));
  if(value == 255){
    value = 70;
  }
  return value;
}
void setThresholdTouchR(byte value){
  EEPROM.write(2, value);
  EEPROM.commit();
  thresholdTouchR = value;
}
/* thresholdTouchM
 * Address 3
 */
byte getThresholdTouchM(){
  byte value = byte(EEPROM.read(3));
  if(value == 255){
    value = 65;
  }
  return value;
}
void setThresholdTouchM(byte value){
  EEPROM.write(3, value);
  EEPROM.commit();
  thresholdTouchM = value;
}
/* thresholdTouchL
 * Address 4
 */
byte getThresholdTouchL(){
  byte value = byte(EEPROM.read(4));
  if(value == 255){
    value = 65;
  }
  return value;
}
void setThresholdTouchL(byte value){
  EEPROM.write(4, value);
  EEPROM.commit();
  thresholdTouchL = value;
}
/* longTouchThreshold
 * Address 5
 */
byte getLongTouchThreshold(){
  byte value = byte(EEPROM.read(5));
  if(value == 255){
    value = 12;
  }
  return value;
}
void setLongTouchThreshold(byte value){
  EEPROM.write(5, value);
  EEPROM.commit();
  longTouchThreshold = value;
}
/* snoozeMinutes
 * Address 6
 */
byte getSnoozeMinutes(){
  byte value = byte(EEPROM.read(6));
  if(value == 255){
    value = 9;
  }
  return value;
}
void setSnoozeMinutes(byte value){
  EEPROM.write(6, value);
  EEPROM.commit();
  snoozeMinutes = value;
}
/* alarmBeeps
 * Address 7
 */
byte getAlarmBeeps(){
  byte value = byte(EEPROM.read(7));
  if(value == 255){
    value = 90;
  }
  return value;
}
void setAlarmBeeps(byte value){
  EEPROM.write(7, value);
  EEPROM.commit();
  alarmBeeps = value;
}
/* maxLux
 * Address 8
 */
byte getMaxLux(){
  byte value = byte(EEPROM.read(8));
  if(value == 255){
    value = 20;
  }
  return value;
}
void setMaxLux(byte value){
  EEPROM.write(8, value);
  EEPROM.commit();
  maxLux = value;
  //calculateAndSetBGLuminosity(currLux);
}
/* minLux
 * Address 9
 */
byte getMinLux(){
  byte value = byte(EEPROM.read(9));
  if(value == 255){
    value = 0;
  }
  return value;
}
void setMinLux(byte value){
  EEPROM.write(9, value);
  EEPROM.commit();
  minLux = value;
  //calculateAndSetBGLuminosity(currLux);
}
/* minBrightness
 * Address 10
 */
byte getMinBrightness(){
  byte value = byte(EEPROM.read(10));
  if(value == 255){
    value = 10;
  }
  return value;
}
void setMinBrightness(byte value){
  EEPROM.write(10, value);
  EEPROM.commit();
  minBrightness = value;
  //calculateAndSetBGLuminosity(currLux);
}
/* alarmHour
 * Address 11
 */
byte getAlarmHour(){
  byte value = byte(EEPROM.read(11));
  if(value == 255){
    value = 8;
  }
  return value;
}
void setAlarmHour(byte value){
  EEPROM.write(11, value);
  EEPROM.commit();
  alarmTime = hourMinuteToTime(value, getAlarmMinute()); 
  displayAlarm();
}
/* alarmMinute
 * Address 12
 */
byte getAlarmMinute(){
  byte value = byte(EEPROM.read(12));
  if(value == 255){
    value = 0;
  }
  return value;
}
void setAlarmMinute(byte value){
  EEPROM.write(12, value);
  EEPROM.commit();
  alarmTime = hourMinuteToTime(getAlarmHour(), value); 
  displayAlarm();
}
/* doAlarm
 * Address --- Not stored into EEPROM
 */
byte getDoAlarm(){
  byte value;
  if(doAlarm == true){
    value = 1;
  }else{
    value = 0;
  }
  return value;
}
void setDoAlarm(byte value){
  if(value == 1){
    doAlarm = true;
  }else{
    doAlarm = false;
  }
  displayAlarm();
}
/* onAlarm
 * Address --- Not stored into EEPROM
 */
byte getOnAlarm(){
  byte value;
  if(onAlarm == true){
    value = 1;
  }else{
    value = 0;
  }
  return value;
}
void setOnAlarm(byte value){
  if(value == 1){
    onAlarm = true;
  }else{
    onAlarm = false;
  }
  displayAlarm();
}
