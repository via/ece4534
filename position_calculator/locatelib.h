#ifndef LOCATELIB_H
#define LOCATELIB_H

/*Locate Lib
 *
 *
 *Provides some functions for calculating the position of a transmitting
 *station given the received signal strength of three receivers
 */


#include <math.h>

struct dms_coordinate{
    int     latDegrees,
    int     latMinutes,
    float   latSeconds,
    int     lonDegrees,
    int     lonMinutes,
    float   lonSeconds};

struct  utm_coordinate{
    float   eastings,
    float   northings};

/*convertDMS_to_UTM
 *
 *This function will convert a DMS coordinate to UTM.  It uses an algorithm
 *from http://www.uwgb.edu/dutchs/UsefulData/UTMFormulas.htm
 *
 *This uses the NAD83 datum
 */
void convertDMS_to_UTM( dms_coordinate* input_coordinate, \
                        utm_coordinate* output_coordinate );

/*distance_to_transmitter
 *
 *Use Friis Transmission Equation to calculate the distance from a receiver to
 *a transmitter. Gain input values are in dBs (power is dBW).  Frequency is 
 *in Hz.
 *
 *The output of this function is the estimated distance in meters.
 */
float distance_to_transmitter( const float power_received, \
                               const float power_transmitted, \
                               const float receive_gain, \
                               const float transmit_gain, \
                               const float frequency);


/*location_gradient_descent
 *
 *Performs a gradient descent to find the point closest to all calculated
 *distance radii.
 *
 *This ::**REQUIRES**:: three receiver locations and estimated distances.
 *It is possible to write the algorithm for an N-receiver system, but
 *for now it only uses three receivers.
 */
void location_gradient_descent( const utm_coordinate** receiver_positions, \
                                const float* distance_data, \
                                utm_coordinate* current_position, \
                                const float stepsize = 0.1 );

#endif
