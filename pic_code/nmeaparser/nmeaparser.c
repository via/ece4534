#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "nmeaparser.h"

/* Example NMEA string 
 * $GPGGA,152521.000,3713.9N,08025.3994,W,1,7,1.11,640.4,M,-32.8,M,,*65
 */

int 
parse_nmea(struct location *loc, const char *nmea) 
{
  char lat_degrees[3];
  char lon_degrees[4];

  char lat_minutes[20];
  char lon_minutes[20];

  const char *comma, *comma2;
  char *dst;

  if (strncmp(nmea, "$GPGGA", 6) != 0)
    return -1;

  comma = 1 + strchr(nmea, ','); /*comma is now first comma */
  comma = 1 + strchr(comma, ','); /*comma is now after second comma, after time */

  comma2 = strchr(comma, ','); /*comma2 is comma after lat */
  strncpy(lat_degrees, comma, 2);
  lat_degrees[2] = '\0';

  loc->lat_degrees = atoi(lat_degrees);
  if (*(comma2 - 1) == 'S')
    loc->lat_degrees = -loc->lat_degrees;

  strncpy(lat_minutes, comma + 2, comma2 - comma + 1);

  loc->lat_minutes = atof(lat_minutes);

  comma = 1 + comma2;
  comma2 = strchr(comma, ',');
  strncpy(lon_degrees, comma, 3);
  lon_degrees[3] = '\0';

  loc->lon_degrees = atoi(lon_degrees);
  if (*(comma2 + 1) == 'E')
    loc->lon_degrees = -loc->lon_degrees;

  strncpy(lon_minutes, comma + 3, comma2 - comma);

  loc->lon_minutes = atof(lon_minutes);

}


int
main()
{

  const char *string = "$GPGGA,152521.000,3713.9S,08025.3994,E,1,7,1.11,640.4,M,-32.8,M,,*65";

  struct location l;

  parse_nmea(&l, string);

  printf("%d %f  %d %f\n", l.lat_degrees, l.lat_minutes, l.lon_degrees, l.lon_minutes);

  return 0;
}

