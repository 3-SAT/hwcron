# Hardware Cron (hwcron)

![HwCron](/doc/hwcron_prototype.jpg?raw=true "Hardware Cron prototype")

Arduino based daily scheduler.
Uses RTC (DS3231) at alarms to switch relays on for specified time.

It used on [Seawatch marine bouy](http://www.oceanor.no/seawatch/buoys-and-sensor/Seawatch) to turn on GSM communication couple times a day.
Using [RouterBoard RB411U](https://routerboard.com/rb411U) with GSM WAN modem and WIFI we can access remote site trough OpenVPN.
[Serial to ethernet server Moxa NPort5150](http://www.moxa.com/product/NPort_5150.htm) is used to connect to bouys main computer.
Hardware Cron is used to lower daily power consumption. GSM Router and serial server is using 600mA constatnt. The HWcron uses 28mA in idle.

Warning: Hardware it's a one time production/prototype! 28mA in idle is **far from** Guiness record. I'm aware of some of possible improvements that can be done to make this piece of hardware more like rocket science. It's just works for now.

## Self advertisement

- The blog about technology for scientists: [HackingForScience](https://www.facebook.com/HackingForScience/)
- Technology services for science: [Searis.pl](http://searis.pl/)

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

## Schematic
Schematic was done in Kicad 4.0.5.
Prototype version using availble modules 
is in HwCron_prototype directory.

## Build environment
Developed using Arduino version 1.8.0.
Tested with Arduino pro mini 5v/16MHz.

### Arduino external Libraries:
- RtcDS3231 - url: (todo)
- SerialCommand - url: (todo)

### Boundled with Arduino environment:
- EEPROM
- Wire

## TODO

- Utilize Mega uC internal WatchDog for safety.


