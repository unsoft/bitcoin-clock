#ifndef BITCOIN_CLOCK_TIMEZONE_CONFIG_H
#define BITCOIN_CLOCK_TIMEZONE_CONFIG_H

#include <cstring>

struct BitcoinClockTimeZone
{
  const char* name;
  const char* posixRule;
};

static const BitcoinClockTimeZone BITCOIN_CLOCK_TIME_ZONES[] = {
    {"Asia/Seoul", "KST-9"},
    {"Asia/Tokyo", "JST-9"},
    {"Asia/Shanghai", "CST-8"},
    {"Asia/Singapore", "SGT-8"},
    {"UTC", "UTC0"},
    {"America/New_York", "EST5EDT,M3.2.0/2,M11.1.0/2"},
    {"America/Chicago", "CST6CDT,M3.2.0/2,M11.1.0/2"},
    {"America/Denver", "MST7MDT,M3.2.0/2,M11.1.0/2"},
    {"America/Phoenix", "MST7"},
    {"America/Los_Angeles", "PST8PDT,M3.2.0/2,M11.1.0/2"},
    {"America/Anchorage", "AKST9AKDT,M3.2.0/2,M11.1.0/2"},
    {"Pacific/Honolulu", "HST10"},
    {"Europe/London", "GMT0BST,M3.5.0/1,M10.5.0/2"},
    {"Europe/Paris", "CET-1CEST,M3.5.0/2,M10.5.0/3"},
    {"Australia/Sydney", "AEST-10AEDT,M10.1.0/2,M4.1.0/3"},
    {"Pacific/Auckland", "NZST-12NZDT,M9.5.0/2,M4.1.0/3"}};

inline const BitcoinClockTimeZone* findBitcoinClockTimeZone(const char* name)
{
  for (const BitcoinClockTimeZone& zone : BITCOIN_CLOCK_TIME_ZONES)
  {
    if (std::strcmp(zone.name, name) == 0)
      return &zone;
  }

  return nullptr;
}

#endif
