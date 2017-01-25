# hwcron

Hardware Cron

Arduino based daily scheduler.
Uses RTC (DS3231) at *alarms* to switch on relays for specified *timeout*.

Developed to switch on RouterBoard RB411U with GSM WAN modem and WIFI on to access remote site for lower power consumption.

## Features:
- remote access serial command control
- 1 second time resolution
- alarm config
- i/o defined at compile 
- maximum number of alarms at compile 
- hardware buttons for manual operation
- config stored in EEPROM

## Commands:
- settime #sets RTC time
- print #print alarms 
- status #print pins status
- temp #print temperature
- time #print time
- set <alarm_id> <HH> <MM> <SS> [<pin> <timeout>] #set alarm
- ena <alarm_id> # enable alarm
- dis <alarm_id> # disable alarm
- post <pin> <[-]time_sec> # postpone_pin by seconds
- on <pin> <[-]time_sec> # on the pin 
- off <pin> # off the pin

## Build environment
Developed using Arduino version 1.8.0.
Tested with Arduino pro mini 5v/16MHz.

### External Libraries:
- RtcDS3231 - url: (todo)
- SerialCommand - url: (todo)

### Boundled with Arduino environment:
- EEPROM
- Wire
