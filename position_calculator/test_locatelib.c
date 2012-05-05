#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "locatelib.h"

static const char *dms_coord_format = "%d %f %d %f\n";

static int
input_receiver_position(struct utm_coordinate *p)
{
  struct dms_coordinate c;

  if (scanf(dms_coord_format, 
      &c.latDegrees, &c.latMinutes, &c.lonDegrees, &c.lonMinutes) != 4) {
    return -1;
  }

  convertDMS_to_UTM(&c, p);

  return 0;
}

static int
input_rssi_values(uint8_t *rssi)
{
  if (scanf("%hhd %hhd %hhd\n", &rssi[0], &rssi[1], &rssi[2]) != 3) {
    return -1;
  }

  return 0;
}

int
main()
{

  struct utm_coordinate nodepos[3];
  const struct utm_coordinate *nodepos_p[3] = 
  { &nodepos[0], &nodepos[1], &nodepos[2] };
  struct utm_coordinate newpos;
  uint8_t rssi[3];
  int i;
  

  /* First read in receiver locations, one per line:
   * latdegrees latminutes londegrees lonminutes\n
   *
   * Next, one line for starting location
   * Then three rssi values per line for as many iterations as wanted.
   * One output line with utm coordinates for each input row
   *
   * Node1RSSI Node2RSSI Node3RSSI 
   */

   for (i = 0; i < 3; ++i) {
     if (input_receiver_position(&nodepos[i]) < 0)
       exit(EXIT_FAILURE);
   }

   if (input_receiver_position(&newpos) < 0)
     exit(EXIT_FAILURE);

   while (input_rssi_values(rssi) >= 0) {
     double distances[3];
     for (i = 0; i < 3; i++) {
       double r_db;
       r_db = convert_rssi_to_db(rssi[i]);
       distances[i] = distance_to_transmitter(r_db,
           0.001, 3, 3,  2400000);
     }
                      
     location_gradient_descent(nodepos_p, distances, &newpos, 0.1); 
     printf("%f %f\n", newpos.eastings, newpos.northings);
   }

   exit(EXIT_SUCCESS);

}


