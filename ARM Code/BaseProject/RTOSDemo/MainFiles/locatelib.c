#include "locatelib.h"

void
distance_and_bearing( struct utm_coordinate* reference, struct utm_coordinate* target, double* distance, double* bearing)
{
    const double pi              = 3.14159265359;
    double n_diff = target->northings - reference->northings;
    double e_diff = target->eastings - reference->eastings;
    *distance = pow( pow(n_diff,2)+pow(e_diff,2), 0.5 );
    *bearing = (180/pi)*asin(n_diff/(*distance));
    if( e_diff > 0 ){
        *bearing = 90 - *bearing;
    }else{
        *bearing += 270;     
    }
}

double 
convert_rssi_to_db(uint8_t rssi_value)
{
    //convert to a 16+ bit integer (so we don't mess up signed arithmetic)
    int  value = 0x00FF & rssi_value;
    //-90 dBm = 0x00 -> -120 dBW = 0x00
    return value*(double)55/255 - 120;
}

void
convertDMS_to_UTM(struct dms_coordinate *input_coordinate,
                  struct utm_coordinate *output_coordinate)
{
    /*See http://www.uwgb.edu/dutchs/UsefulData/UTMFormulas.htm
     *for the explanation
     *
     */
    //Constants defined
    const double equatorial      = 6378137;
    const double pi              = 3.14159265359;
    const double k0              = 0.9996;
    const double e_squared       = 0.006694380005;
    const double A               = 6367449.14594;
    const double B               = -16038.5083688;
    const double C               = 16.83222008030;
    const double D               = -0.0218007663577;
    const double e_prime_squared = 0.00673949675732;
    const double long0           = 1.41371669412;

    //Convert to radians
    double latitude = input_coordinate->latDegrees + 
                          input_coordinate->latMinutes / (double)60;
    double longitude = input_coordinate->lonDegrees + 
                          input_coordinate->lonMinutes / (double)60;

    double lat_rads = latitude * pi / (double)180;
    double lon_rads = longitude * pi / (double)180;

    //Calculate final constants
    double nu = equatorial / pow((1 - e_squared * pow(sin(lat_rads), 
            (double)2)), 0.5);
    double p = lon_rads - long0;

    //Determine the Meridional Arc
    double M = A * lat_rads + B * sin(2 * lat_rads) + C * sin(4 * lat_rads) + 
          D * sin(6 * lat_rads);

    //Calculate northings and eastings
    output_coordinate->northings = M * k0 + k0 * nu * sin(2 * lat_rads)/4 * 
     pow(p, 2) + (k0 * nu * sin(lat_rads) * pow(cos(lat_rads), 3) / 24) * 
     (5 - pow(tan(lat_rads), 2) + 9 * e_prime_squared  * pow(cos(lat_rads), 2) + 
      4 * pow(e_prime_squared, 2) * pow(cos(lat_rads), 4)) * pow(p, 4);

    output_coordinate->eastings = (k0 * nu * cos(lat_rads) * p) + 
      (k0 * nu * pow(cos(lat_rads), 3) / 6) * 
      (1 - pow(tan(lat_rads), 2) + e_prime_squared * pow(cos(lat_rads), 2)) * 
      pow(p, 3);

    output_coordinate->eastings = -1*output_coordinate->eastings + 500000;
    return;
}


double 
distance_to_transmitter(const double power_received,
                        const double power_transmitted, 
                        const double receive_gain, 
                        const double transmit_gain, 
                        const double frequency )
{
    const double pi = 3.14159265359;
    double lambda = 299792458 / frequency;
    //Friis transmission equation
    double distance = pow(10, (power_received - power_transmitted - 
          receive_gain - transmit_gain) / 20) * 4 * pi / lambda;
    return 1 / distance;
}

void 
location_gradient_descent(const struct utm_coordinate *receiver_positions, 
                          const double *distance_data, 
                          struct utm_coordinate *current_position, 
                          const double stepsize)
{
    double x_dev = 0.0;
    double y_dev = 0.0;
    double K = 0.0;
    int i;
    for(i = 0; i < 3; i++)
    {
        K = sqrt(pow(current_position->eastings - 
              receiver_positions[i].eastings, 2) + 
              pow(current_position->northings - 
              receiver_positions[i].northings, 2) );

        if (K == 0)
          K = 1e-10;

        x_dev += (distance_data[i] - K) * (-1 / (2 * K)) * 
          (-2 * current_position->eastings + 2 * receiver_positions[i].eastings);
        y_dev += (distance_data[i] - K) * (-1 / (2 * K)) * 
          (-2 * current_position->northings + 2 * receiver_positions[i].northings);

    }
    current_position->northings += 2 * y_dev * stepsize;
    current_position->eastings += 2 * x_dev * stepsize;

}
