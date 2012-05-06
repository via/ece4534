#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "nmeaparser.h"

/* Example NMEA string 
 * $GPGGA,152521.000,3713.9,N,08025.3994,W,1,7,1.11,640.4,M,-32.8,M,,*65
 */

static int
verify_checksum(char *nmea)
{
  char *start, *stop;
  char chksum;
  char chksum2;
  char b1, b2;
  start = nmea + 1;
  stop = strrchr(nmea, '*') - 1;

  b1 = stop[2];
  if (b1 >= 'A')
    b1 = b1 - 'A';
  else
    b1 = b1 - '0';

  b2 = stop[3];
  if (b2 >= 'A')
    b2 = b2 - 'A';
  else
    b2 = b2 - '0';

  chksum = b1 * 16 + b2;
  chksum2 = start[0];
  for (start = start + 1; start != stop + 1; ++start)
    chksum2 ^= *start;

  if (chksum2 != chksum)
    return -1;

  return 0;

}


int 
parse_nmea(struct location *loc, char *nmea) 
{
  char lat_degrees[3];
  char lon_degrees[4];

  char lat_minutes[20];
  char lon_minutes[20];

  char *comma, *comma2;
  char *dst;

  if (verify_checksum(nmea) < 0)
    return -1;

  comma = 1 + strchr(nmea, ','); /*comma is now first comma */
  comma = 1 + strchr(comma, ','); /*comma is now after second comma, after time */

  comma2 = strchr(comma, ','); /*comma2 is comma after lat */
  strncpy(lat_degrees, comma, 2);
  lat_degrees[2] = '\0';

  loc->lat_degrees = atoi(lat_degrees);
  if (*(comma2 + 1) == 'S')
    loc->lat_degrees = -loc->lat_degrees;

  strncpy(lat_minutes, comma + 2, comma2 - comma - 2);
  lat_minutes[comma2 - comma] = '\0';

  loc->lat_minutes = atof(lat_minutes);

  comma = 3 + comma2;
  comma2 = strchr(comma, ',');
  strncpy(lon_degrees, comma, 3);
  lon_degrees[3] = '\0';

  loc->lon_degrees = atoi(lon_degrees);
  if (*(comma2 + 1) == 'E')
    loc->lon_degrees = -loc->lon_degrees;

  strncpy(lon_minutes, comma + 3, comma2 - comma - 3);
  lon_minutes[comma2 - comma] = '\0';

  loc->lon_minutes = atof(lon_minutes);

  return 0;

}
