#ifndef NMEAPARSER_H
#define NMEAPARSER_H

struct location {
  signed char lat_degrees;
  signed char lon_degrees;
  float lat_minutes;
  float lon_minutes;
};

int parse_nmea(struct location *loc, char *nmea);

#endif

