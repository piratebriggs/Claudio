#include <Arduino.h>

void setupOTA();
void checkAlarm();
bool wifiConnect();
bool getNtpTime();
int getDayOfWeek();
String addSnooze(String sTime, int snooze);
String hourMinuteToTime(int hour, int minute);
void refreshTime();
void displayTime();
void displayDate();

void displayTemp();
void displayHumi();
void displayAlarm();

float getCurrentTemp();
float getCurrentHumi();
void timeChanged(String prevTime, String currTime);

void dateChanged(String prevDate, String currDate);
void tempChanged(float prevTemp, float currTemp);

void humiChanged(float prevHumi, float currHumi);
void configOn();
void configOff();
void configMode();
String configExecute(String instruction);
String getConfigValue(String item);
String setConfigValue(String item, int setValue);
String getAllItems();
void loadConfiguration();


byte getBootWait();
void setBootWait(byte value);
byte getTouchReadings();
void setTouchReadings(byte value);
byte getThresholdTouchR();
void setThresholdTouchR(byte value);
byte getThresholdTouchM();
void setThresholdTouchM(byte value);
byte getThresholdTouchL();
void setThresholdTouchL(byte value);
byte getLongTouchThreshold();
void setLongTouchThreshold(byte value);
byte getSnoozeMinutes();
void setSnoozeMinutes(byte value);
byte getAlarmBeeps();
void setAlarmBeeps(byte value);
byte getMaxLux();
void setMaxLux(byte value);
byte getMinLux();
void setMinLux(byte value);
byte getMinBrightness();
void setMinBrightness(byte value);
byte getAlarmHour();
void setAlarmHour(byte value);
byte getAlarmMinute();
void setAlarmMinute(byte value);
byte getDoAlarm();
void setDoAlarm(byte value);
byte getOnAlarm();
void setOnAlarm(byte value);

