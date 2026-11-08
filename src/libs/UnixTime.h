#ifndef _UnixTime_h
#define _UnixTime_h
class UnixTime {
 public:
  UnixTime(int8_t gmt) {
    _gmt = gmt;
  }
  uint32_t getUnix() {
    int8_t my = (month >= 3) ? 1 : 0;
    uint16_t y = year + my - 1970;
    uint16_t dm = 0;
    for (int i = 0; i < month - 1; i++) dm += (i < 7) ? ((i == 1) ? 28 : ((i & 1) ? 30 : 31)) : ((i & 1) ? 31 : 30);
    return (((day - 1 + dm + ((y + 1) >> 2) - ((y + 69) / 100) + ((y + 369) / 100 / 4) + 365 * (y - my)) * 24ul + hour - _gmt) * 60ul + minute) * 60ul + second;
  }

  void getDateTime(uint32_t t) {
    t += _gmt * 3600ul;
    second = t % 60ul;
    t /= 60ul;
    minute = t % 60ul;
    t /= 60ul;
    hour = t % 24ul;
    t /= 24ul;
    dayOfWeek = (t + 4) % 7;
    if (!dayOfWeek) dayOfWeek = 7;
    uint32_t z = t + 719468;
    uint8_t era = z / 146097ul;
    uint16_t doe = z - era * 146097ul;
    uint16_t yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    uint16_t y = yoe + era * 400;
    uint16_t doy = doe - (yoe * 365 + yoe / 4 - yoe / 100);
    uint16_t mp = (doy * 5 + 2) / 153;
    day = doy - (mp * 153 + 2) / 5 + 1;
    month = mp + (mp < 10 ? 3 : -9);
    y += (month <= 2);
    year = y;
  }

  void setDateTime(uint16_t nyear, uint8_t nmonth, uint8_t nday, uint8_t nhour, uint8_t nminute, uint8_t nsecond) {
    year = nyear;
    month = nmonth;
    day = nday;
    hour = nhour;
    minute = nminute;
    second = nsecond;
  }

  uint16_t year;
  uint8_t month;
  uint8_t day;
  uint8_t hour;
  uint8_t minute;
  uint8_t second;
  uint8_t dayOfWeek;

 private:
  int8_t _gmt = 0;
};
#endif