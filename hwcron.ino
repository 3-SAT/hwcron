/*

Hardware Cron
--------------------------------

This program is for turning on/off devices over time daily (HH:MM:SS).

Alarm sets pin on, and timeout.
When timeout goes to zero, pin is going off.
When another alarm goes on in time of timeout.
Timeout is extended or 

Written in Arduino environment 1.8.0
Additional libraries: 

External:
- RtcDS3231 - url: 
- SerialCommand - url: 

Bundled with Arduino environment:
- EEPROM
- Wire

Example commands for setup:

set 0 7 0 0 10 600 
set 1 8 0 0 10 600 
set 2 9 0 0 10 600 
set 3 10 0 0 10 600 
set 4 11 0 0 10 600 
set 5 12 0 0 10 600 
set 6 13 0 0 10 600 
set 7 14 0 0 10 600 
set 8 16 0 0 10 600 
set 9 18 0 0 10 600 

ena 0
ena 1
ena 2
ena 3
ena 4
ena 5
ena 6
ena 7
ena 8
ena 9

settime 2017 01 24 23 59 59
settime 2017 01 24 17 51 00

post 0 99999
post 1 99999

*/

#define MAX_ALARMS 10
#define DEFAULT_TIMEOUT (5*60)

#include <avr/pgmspace.h>

#include <EEPROM.h>
#include <Wire.h> 
#include <RtcDS3231.h>
RtcDS3231<TwoWire> Rtc(Wire);

#define SERIALCOMMANDBUFFER 48
#include "SerialCommand.h"

SerialCommand SCmd;

#define countof(a) (sizeof((a)) / sizeof((a)[0]))

#define HMS_GEQ(a,b) ((int32_t((a).h * 3600) + ((a).m * 60) + (a).s) >= (int32_t((b).h * 3600) + ((b).m * 60) + (b).s))

// cron time
typedef struct {
  int8_t h;
  int8_t m;
  int8_t s;
} hms_t;


#define MAX_PINS 4

typedef struct {
  uint8_t pin;
  uint8_t initial; 
  uint8_t inverted;
  uint8_t control_pin;
} availble_pins_t;

// setup pins 
availble_pins_t availble_pins[MAX_PINS] = {
  {10,1,1,9},
  {11,1,1,0},
  {12,1,1,0},
  {13,1,1,0}
};

// pin state
typedef struct {
  int32_t timeout;
  uint8_t last_control_pin_state;
} pin_state_t;

pin_state_t pin_state[MAX_PINS] = {{0},{0},{0},{0}};

//NEW functions **************************************************************************************************************************************************
typedef struct {
  uint8_t pin;
  uint8_t pin_power;
} power_conf;

#define POWER_PINS 1//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


#define CONFIG_START 32
#define CONFIG_VERSION_1 0
#define CONFIG_VERSION_2 0
#define CONFIG_VERSION_3 4//changeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeddddddddddddddddddddddddddd

typedef struct {
  int8_t h;
  int8_t m;
  int8_t s;
  uint8_t enabled;
  int32_t timeout;
  uint8_t pin;
} alarm_config_t;

struct StoreStruct {
  uint8_t version[3];
  alarm_config_t alarms[MAX_ALARMS];

  power_conf power_pins[POWER_PINS];

} config_storage = {
  {CONFIG_VERSION_1,CONFIG_VERSION_2,CONFIG_VERSION_3}
};

void loadConfig() {
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION_1 &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION_2 &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION_3){
    for (unsigned int t=0; t<sizeof(config_storage); t++)
      *((char*)&config_storage + t) = EEPROM.read(CONFIG_START + t);
  } else {
    for (unsigned int t=0; t<MAX_ALARMS; t++){
			config_storage.alarms[t].enabled = 0;
			config_storage.alarms[t].h = 0;
			config_storage.alarms[t].m = 0;
			config_storage.alarms[t].s = 0;
			config_storage.alarms[t].pin = 0;
			config_storage.alarms[t].timeout = 0;
    }
    for(unsigned int t=0; t < POWER_PINS; t++){//new config !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      config_storage.power_pins[t].pin = 0;
      config_storage.power_pins[t].pin_power = 0;
    }
  }
}

void saveConfig() {
  for (unsigned int t=0; t<sizeof(config_storage); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&config_storage + t));
}

RtcDateTime restart_time;

void setup () 
{
    Serial.begin(115200);

    Serial.print(F("Hardware Cron, compiled on: "));
    Serial.print(F(__DATE__));
    Serial.print(F(":"));
    Serial.println(F(__TIME__));
    Serial.print(F("config version: "));
    Serial.print(CONFIG_VERSION_1);
    Serial.print(',');
    Serial.print(CONFIG_VERSION_2);
    Serial.print(',');
    Serial.println(CONFIG_VERSION_3);

		loadConfig();
  	SCmd.addDefaultHandler(scmd_unrecognized);
		SCmd.addCommand("?",scmd_help);

		SCmd.addCommand("settime",scmd_settime);
		SCmd.addCommand("time",scmd_printtime);
		SCmd.addCommand("temp",scmd_print_temperature);

		// SCmd.addCommand("setpin",scmd_setpin);

		SCmd.addCommand("print",scmd_print_alarms);
		SCmd.addCommand("status",scmd_pins_status);
		SCmd.addCommand("set",scmd_set_alarm);

		SCmd.addCommand("ena",scmd_enable_alarm);
		SCmd.addCommand("dis",scmd_disable_alarm);
	  SCmd.addCommand("post",scmd_postpone_pintimeout);
    SCmd.addCommand("on",scmd_postpone_pintimeout);
    SCmd.addCommand("off",scmd_pinoff);

    SCmd.addCommand("pinpower", scmd_pin_power);

    Rtc.Begin();
		rtc_setup();

		// pin setup
		restart_time = Rtc.GetDateTime();

    //pinMode(LED_BUILTIN, OUTPUT);

	  for (int8_t t = 0; t < MAX_PINS; t++){
      if (availble_pins[t].control_pin > 0 ) 
        pinMode(availble_pins[t].control_pin, INPUT_PULLUP);

	    pinMode(availble_pins[t].pin, OUTPUT);
	    digitalWrite(availble_pins[t].pin, ((availble_pins[t].initial == 1) ?  HIGH : LOW));////////////////////////////////////////////////////////////////////////////////////////////
	  }
   for(int8_t t=0; t < POWER_PINS; t++){
     analogWrite(config_storage.power_pins[t].pin, config_storage.power_pins[t].pin_power);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
   }
}

void loop () 
{
		for (int i = 0; i < 90; ++i)
		{
	    SCmd.readSerial();
	    delay(10); //ms
		}

		//digitalWrite(LED_BUILTIN, LOW);
    delay(100); //ms

		static hms_t last_checked = {-1,0,0};

    if (!Rtc.IsDateTimeValid()) 
    {
        Serial.println(F("RTC lost confidence in the DateTime!"));
    }

    RtcDateTime now = Rtc.GetDateTime();

//  printDateTime(now); Serial.println();

    hms_t current;

    current.h = now.Hour();
    current.m = now.Minute();
    current.s = now.Second();

    if (last_checked.h == -1) last_checked = current;

/*
    Serial.print(current.h);
    Serial.print(F(":"));
    Serial.print(current.m);
    Serial.print(F(":"));
    Serial.print(current.s);
    Serial.println();
*/
    // process alarms
    for(uint8_t i = 0; i < MAX_PINS; ++i){

      if (availble_pins[i].control_pin > 0){

        if( digitalRead(availble_pins[i].control_pin) == LOW ){
          if( pin_state[i].last_control_pin_state == HIGH ){
            if( pin_state[i].timeout < DEFAULT_TIMEOUT ){
              pin_state[i].timeout = DEFAULT_TIMEOUT;
            }
          }
        }

        pin_state[i].last_control_pin_state = digitalRead(availble_pins[i].control_pin);
      }

      // timeout the timeput
      if (pin_state[i].timeout > 0){
        pin_state[i].timeout --;

        if (pin_state[i].timeout == 0){
          set_pin(availble_pins[i].pin, 0);
          Serial.print(F("OFF: "));
          Serial.print(i);
          Serial.println();
        } else {
          set_pin(availble_pins[i].pin, 1);
          if (pin_state[i].timeout < 10 || (pin_state[i].timeout % 10) == 0 ){
            Serial.print(F("TO: "));
            Serial.print(availble_pins[i].pin);
            Serial.print(F(":"));
            Serial.print(pin_state[i].timeout);
            Serial.println();
          }
        }
        
      }  

    }

    // process alarms
    for(uint8_t i = 0; i < MAX_ALARMS; ++i){
      // process if enabled
      if(config_storage.alarms[i].enabled != true) continue;

      // fire alarm
      if (HMS_GEQ(current,config_storage.alarms[i]) and !HMS_GEQ(last_checked,config_storage.alarms[i]) ){

        // print
        printDateTime(now);
        Serial.println();

        Serial.print(F("ON: "));
        Serial.print(i);
        Serial.print(' ');
        Serial.print(config_storage.alarms[i].h);
        Serial.print(':');
        Serial.print(config_storage.alarms[i].m);
        Serial.print(':');
        Serial.print(config_storage.alarms[i].s);
        Serial.print(F(" pin: "));
        Serial.print(config_storage.alarms[i].pin);

        {
          int8_t pin_idx = find_pin_index(config_storage.alarms[i].pin);

          Serial.print(F(" pin_idx:"));
          Serial.print(pin_idx);

          if (pin_idx >= 0) {
            // fire on

            if (config_storage.alarms[i].timeout > pin_state[pin_idx].timeout){
              pin_state[pin_idx].timeout = config_storage.alarms[i].timeout;
            }

          } else {
            Serial.print(F(" pin_error"));
          }

          Serial.println();      

        }
	    }

	  }


    last_checked = current;
}

int8_t find_pin_index(uint8_t pin){
  for (int8_t p = 0; p < MAX_PINS; ++p)
  {
    if (availble_pins[p].pin == pin) return p;
  }
  return -1;
}

void set_pin(uint8_t pin, uint8_t pin_out){

	// set pin
  int8_t pin_idx = find_pin_index(pin);
  if (pin_idx <0) return ;

	if(availble_pins[pin_idx].inverted == 1)
		{digitalWrite(availble_pins[pin_idx].pin, ((pin_out == 0) ? HIGH : LOW));}
	else 
		{digitalWrite(availble_pins[pin_idx].pin, ((pin_out == 1) ? HIGH : LOW));}
}


void printDateTime(const RtcDateTime& dt)
{
    char datestring[20];

    snprintf_P(datestring, 
            countof(datestring),
            PSTR("%04u %02u %02u %02u %02u %02u"),
            dt.Year(),
            dt.Month(),
            dt.Day(),
            dt.Hour(),
            dt.Minute(),
            dt.Second() );
    Serial.print(datestring);
}

void print_err(){	Serial.println(F("Error"));}

void print_ok(){	Serial.println(F("Ok"));}

void scmd_pin_power(){//new functions !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  char *arg;
  int8_t array_entry = 0;
  int8_t pin_number = 0;
  uint8_t power_set = 0;
  arg = SCmd.next(); if (arg != NULL)  array_entry = atol(arg); else {print_err(); return;};
  arg = SCmd.next(); if (arg != NULL)  power_set = ((atol(arg) * 100)/255); else {print_err(); return;};	//wypisuje w procentach
  if(array_entry >= 0 && array_entry < POWER_PINS){
    arg = SCmd.next(); if (arg != NULL)  pin_number = atol(arg); else {pin_number = config_storage.power_pins[array_entry].pin;};
    config_storage.power_pins[array_entry].pin = pin_number;
    config_storage.power_pins[array_entry].pin_power = power_set;
    saveConfig();
    analogWrite(pin_number, power_set);
  }
  else{print_err(); return;}
}

void scmd_unrecognized(){
  Serial.println(F("? for help")); 
}

void scmd_help(){
  Serial.println(); 
  Serial.print(F("Hardware Cron, compiled on: "));
  Serial.print(F(__DATE__));
  Serial.print(F(":"));
  Serial.println(F(__TIME__));
  Serial.print(F("config version: "));
  Serial.print(CONFIG_VERSION_1);
  Serial.print(',');
  Serial.print(CONFIG_VERSION_2);
  Serial.print(',');
  Serial.println(CONFIG_VERSION_3);

  Serial.print(F("Running since: ")); 
  printDateTime(restart_time);
  Serial.println(); 
  Serial.println(); 

  RtcDateTime now = Rtc.GetDateTime();
  Serial.println(F("Commands:")); 
  Serial.print(F("settime "));
  printDateTime(now);
  Serial.println(F(" #sets RTC time")); 


  Serial.println(F("print #print alarms ")); 
  Serial.println(F("status #print pins status")); 
  Serial.println(F("temp #print temperature")); 
  Serial.println(F("time #print time")); 
  Serial.println(F("set <alarm_id> <HH> <MM> <SS> [<pin> <timeout>] #set alarm")); 
  Serial.println(F("ena <alarm_id> # enable alarm")); 
  Serial.println(F("dis <alarm_id> # disable alarm")); 
  Serial.println(F("post <pin> <[-]time_sec> # postpone_pin by seconds")); 
  Serial.println(F("on <pin> <[-]time_sec> # on the pin ")); 
  Serial.println(F("off <pin> # off the pin")); 

}

void scmd_print_temperature(){
    RtcTemperature temp = Rtc.GetTemperature();
    Serial.print(temp.AsFloat());
    Serial.println("C");
}

void scmd_settime(){

  char *arg;

	uint16_t year;
  uint8_t month;
  uint8_t dayOfMonth;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;

  arg = SCmd.next(); if (arg != NULL) year = atol(arg); else {print_err(); return;};
  arg = SCmd.next(); if (arg != NULL) month = atol(arg); else {print_err(); return;};
  arg = SCmd.next(); if (arg != NULL) dayOfMonth = atol(arg); else {print_err(); return;};
  arg = SCmd.next(); if (arg != NULL) hour = atol(arg); else {print_err(); return;};
  arg = SCmd.next(); if (arg != NULL) minute = atol(arg); else {print_err(); return;};
  arg = SCmd.next(); if (arg != NULL) second = atol(arg); else {print_err(); return;};

  RtcDateTime dtm = RtcDateTime( year,month, dayOfMonth, hour, minute, second);
  printDateTime(dtm);
  Serial.println(); 

  Rtc.SetDateTime(dtm);

	print_ok();

}

void scmd_printtime(){

  RtcDateTime dtm = Rtc.GetDateTime();
  printDateTime(dtm);
  Serial.println(); 

}

void scmd_print_alarms(){

	Serial.println(F("Alarms config:"));
	for (int i = 0; i < MAX_ALARMS; ++i)
	{
		Serial.print(i);
		Serial.print(F("| "));	

		Serial.print(F("e:"));	
		Serial.print(config_storage.alarms[i].enabled);	
		Serial.print(F(" hms:"));	
		Serial.print(config_storage.alarms[i].h);	
		Serial.print(':');	
		Serial.print(config_storage.alarms[i].m);	
		Serial.print(':');	
		Serial.print(config_storage.alarms[i].s);
		Serial.print(F(" pin:"));	
		Serial.print(config_storage.alarms[i].pin);
    Serial.print(F(" t/o:"));
    Serial.print(config_storage.alarms[i].timeout); 
		Serial.println();
	}

};

void scmd_pins_status(){

	Serial.println(F("Pins status:"));
	for (uint8_t i = 0; i < MAX_PINS; ++i)
	{
		Serial.print(availble_pins[i].pin);
		Serial.print(F(" t:"));
		Serial.print(pin_state[i].timeout);	
		Serial.println();
	}

};

void scmd_set_alarm(){

  char *arg;

  uint8_t alarm_id;
  hms_t hms;
	int32_t timeout;
  uint8_t pin;

  arg = SCmd.next(); if (arg != NULL) alarm_id = atol(arg); else {print_err(); return;};
  if(alarm_id > MAX_ALARMS) {print_err(); return;};
  
  arg = SCmd.next(); if (arg != NULL) hms.h = atol(arg); else {print_err(); return;};
  if(hms.h > 23) {print_err(); return;};
  arg = SCmd.next(); if (arg != NULL) hms.m = atol(arg); else {print_err(); return;};
  if(hms.m > 59) {print_err(); return;};
  arg = SCmd.next(); if (arg != NULL) hms.s = atol(arg); else {print_err(); return;};
  if(hms.s > 59) {print_err(); return;};


  arg = SCmd.next(); 

	if (arg == NULL){
	  config_storage.alarms[alarm_id].h = hms.h;
  	config_storage.alarms[alarm_id].m = hms.m;
  	config_storage.alarms[alarm_id].s = hms.s;
	  saveConfig();
	  print_ok();
	  return;
	}

  if (arg != NULL) pin = atol(arg); else {print_err(); return;};

  for (uint8_t i = 0; i < MAX_PINS; ++i)
  {
  	if(pin == availble_pins[i].pin) break;
  	if(i == MAX_PINS - 1) {print_err(); return;};
  }
  arg = SCmd.next(); if (arg != NULL) timeout = atol(arg); else {print_err(); return;};

  config_storage.alarms[alarm_id].h = hms.h;
	config_storage.alarms[alarm_id].m = hms.m;
	config_storage.alarms[alarm_id].s = hms.s;
  config_storage.alarms[alarm_id].timeout = timeout;
  config_storage.alarms[alarm_id].pin = pin;
  saveConfig();

  print_ok();
}

void scmd_enable_alarm(){

  char *arg;

  uint8_t alarm_id;

  arg = SCmd.next(); if (arg != NULL) alarm_id = atol(arg); else {print_err(); return;};

  if(alarm_id > MAX_ALARMS) {print_err(); return;};
  config_storage.alarms[alarm_id].enabled = 1;

  saveConfig();

  print_ok();
}

void scmd_disable_alarm(){

  char *arg;

  uint8_t alarm_id;

  arg = SCmd.next(); if (arg != NULL) alarm_id = atol(arg); else {print_err(); return;};

  if(alarm_id > MAX_ALARMS) {print_err(); return;};
  config_storage.alarms[alarm_id].enabled = 0;

  saveConfig();

  print_ok();
}

void scmd_postpone_pintimeout(){

  char *arg;

  uint8_t pin;
	int32_t timeout;

  timeout = DEFAULT_TIMEOUT;

  arg = SCmd.next(); if (arg != NULL) pin = atol(arg); else {print_err(); return;};
  arg = SCmd.next(); if (arg != NULL) timeout = atol(arg);

  int8_t pin_idx = find_pin_index(pin);
  if (pin_idx < 0) {print_err(); return;}
  
  if(pin_state[pin_idx].timeout + timeout < 1 ){
		pin_state[pin_idx].timeout = 1;
		print_ok();	
    return;
	} 

  if (pin_state[pin_idx].timeout == 0){
    set_pin(config_storage.alarms[pin_idx].pin, 1);
		pin_state[pin_idx].timeout = timeout;
	  print_ok(); 
    return;
	}

  pin_state[pin_idx].timeout += timeout;

  print_ok();

}

void scmd_pinoff(){

  char *arg;

  uint8_t pin;

  arg = SCmd.next(); if (arg != NULL) pin = atol(arg); else {print_err(); return;};

  int8_t pin_idx = find_pin_index(pin);
  if (pin_idx < 0) {print_err(); return;}
  

  if(pin_state[pin_idx].timeout > 0 ){
    pin_state[pin_idx].timeout = 1;
    print_ok(); 
    return;
  } 

  print_ok();

}

void rtc_setup(){

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    printDateTime(compiled);
    Serial.println();

    if (!Rtc.IsDateTimeValid()) 
    {
        Serial.println(F("RTC lost confidence in the DateTime!"));
        Rtc.SetDateTime(compiled);
    }

    if (!Rtc.GetIsRunning())
    {
        Serial.println(F("RTC was not actively running, starting now"));
        Rtc.SetIsRunning(true);
    }

    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) 
    {
        Serial.println(F("RTC is older than compile time!  (Updating DateTime to compile time)"));
        Rtc.SetDateTime(compiled);
    }
/*
    else if (now > compiled) 
    {
        Serial.println(F("RTC is newer than compile time. (this is expected)"));
    }
    else if (now == compiled) 
    {
        Serial.println(F("RTC is the same as compile time! (not expected but all is fine)"));
    }
*/
    Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone); 

}

