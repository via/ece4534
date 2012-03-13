#ifndef LOCATELIB_H
#define LOCATELIB_H

/*locatelib
 *
 *Provides some functions for calculating the position of a transmitting
 *station given the received signal strength of three receivers
 */

#include <math.h>

typedef struct {
    int     latDegrees;
    int     latMinutes;
    double  latSeconds;
    int     lonDegrees;
    int     lonMinutes;
    double  lonSeconds;
} dms_coordinate;

typedef struct {
    double  eastings;
    double  northings;
} utm_coordinate;

/*convert_rssi_to_db
 *
 *Convert's the Microchip MiWi RSSI 8-bit value to dBW
 */
double convert_rssi_to_db( uint8_t* rssi_value );

/*convertDMS_to_UTM
 *
 *This function will convert a DMS coordinate to UTM.  It uses an algorithm
 *from http://www.uwgb.edu/dutchs/UsefulData/UTMFormulas.htm
 *
 *This uses the NAD82 datum
 */
void convertDMS_to_UTM( dms_coordinate* input_coordinate, \
                        utm_coordinate* output_coordinate);


/*distance_to_transmitter
 *
 *Use Frees Transmission Equation to calculate the distance from a receiver to
 *a transmitter.  Gain input values are in dB (power in dBW).  Frequency is in
 *Hz
 */
double distance_to_transmitter( const double power_received, \
                                const double power_transmitted, \
                                const double receive_gain, \
                                const double transmit_gain, \
                                const double frequency);

/*location_gradient_descent
 *
 *Performs a gradient descent to find the point closest to all calculated
 *distance radii
 *
 *This ::**REQUIRES**:: three receiver locations and estimated distances.
 *It is possible to write the algorithm for an N-receiver system, but
 *for now it only uses three receivers.
 */
void location_gradient_descent( const utm_coordinate** receiver_positions, \
                                const double* distance_data, \
                                utm_coordinate* current_position, \
                                const double stepsize );

#endif
