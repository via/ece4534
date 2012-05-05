#ifndef NMEAPARSER_H
#define NMEAPARSER_H

struct location {
  int8_t lat_degrees;
  int8_t lon_degrees;
  float lat_minutes;
  float lon_minutes;
};

int parse_nmea(struct location *loc, char *nmea);

#endif

