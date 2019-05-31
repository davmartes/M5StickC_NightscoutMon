#include "M5StickC_NSconfig.h"

void readConfiguration(tConfig *cfg) {
  strlcpy(cfg->url, "XXX.herokuapp.com", 64);
  cfg->bootPic[0]=0;
  strcpy(cfg->userName, "User");
  cfg->timeZone = 3600; // time zone shift in second (1 hour = 3600 seconds)
  cfg->dst = 3600; // daylight shift in seconds
  cfg->show_mgdl = 1; // 0 for mmol/L, 1 for mg/dL
  cfg->show_current_time = 0; // not used currently
  cfg->show_COB_IOB = 0; // show COB and IOB
  cfg->snooze_timeout = 30; // not used currently
  cfg->alarm_repeat = 5; // not used currently
  cfg->dev_mode = 0; // only for developer purposes
  cfg->yellow_low = 3.3; // show glycemia value yellow under this value (always in mmol/L)
  cfg->yellow_high = 10; // show glycemia value yellow over this value (always in mmol/L)
  cfg->red_low = 2.2; // show glycemia value red under this value (always in mmol/L)
  cfg->red_high = 11.1; // show glycemia value red over this value (always in mmol/L)
  cfg->snd_warning = 3.3; // LED alarm under this value
  cfg->snd_alarm = 2.2; // LED alarm under this value
  cfg->snd_warning_high = 18.8; // LED alarm over this value
  cfg->snd_alarm_high = 13.8; // LED alarm over this value
  cfg->snd_no_readings = 60; // LED alarm if no date more then these minutes
  cfg->warning_music[0]=0; // not used currently
  cfg->warning_volume = 30; // not used currently
  cfg->alarm_music[0]=0; // not used currently
  cfg->alarm_volume = 100; // not used currently
  cfg->brightness1 = 10; // default display brightness (0-15, but reasonable values are 7-15)
  cfg->brightness2 = 15; // the second level of display brightness (0-15, but reasonable values are 7-15)
  cfg->brightness3 = 7; // the third level of display brightness (0-15, but reasonable values are 7-15)
  strlcpy(cfg->wlan1ssid, "XXX",32);
  strlcpy(cfg->wlan1pass, "XXX", 32);
  strlcpy(cfg->wlan2ssid, "XXX", 32);
  strlcpy(cfg->wlan2pass, "XXX", 32);
  strlcpy(cfg->wlan3ssid, "XXX", 32);
  strlcpy(cfg->wlan3pass, "XXX", 32);
}
