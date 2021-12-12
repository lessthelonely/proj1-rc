#ifndef ALARM_H
#define ALARM_H

/**
     * Alarm handler: increases a counter of the number of tries of a certain action and checks if it has exceeded the number of tries permitted
*/
void atende();

/**
     * Installs alarm 
*/
void install_alarm();

/**
     * Deactivates the alarm
*/
void deactivate_alarm();

#endif
