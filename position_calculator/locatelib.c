#include "locatelib.h"

void convertDMS_to_UTM( dms_coordinate* input_coordinate, \
                        utm_coordinate* output_coordinate )
{
    //Constants defined
    const float equatorial = 6378137;
    const float polar = 6356752.3142;
    const float flattening = 1/298.257223563;
    const float pi = 3.14159265359;
    const float k0 = 0.9996;
    const float e = 0.0818191909334;
    const float e_squared = 0.006694380005;
    const float A = 6367449.14594;
    const float B = -16038.5083688;
    const float C = 16.83222008030;
    const float D = -0.0218007663577;
    const float e_prime_squared = 0.00673949675732;
    const float n = 0.00167922038994;
    const float long0 = 1.41371669412;

    //Convert to radians
    float latitude = input_coordinate->latDegrees + input_coordinate->latMinutes/(float)60 + input_coordinate->latSeconds/(float)3600;
    float longitude = input_coordinate->lonDegrees + input_coordinate->lonMinutes/(float)60 + input_coordinate->lonSeconds/(float)3600;

    float lat_rads = latitude*pi/(float)180;
    float lon_rads = longitude*pi/(float)180;

    //Calculate final constants
    float rho = equatorial*(1-e_squared)/pow((1-e_squared*pow(sin(lat_rads),(float)2)),1.5);
    float nu = equatorial/pow((1-e_squared*pow(sin(lat_rads),(float)2)),0.5);
    float p = lon_rads - long0;

    //Determine the Meridional Arc
    M = A*lat_rads + B*sin(2*lat_rads) + C*sin(4*lat_rads) + D*sin(6*lat_rads);

    //Calculate northings and eastings
    output_coordinate->northings = M*k0 + k0*nu*sin(2*lat_rads)/4*pow(p,2) + (k0*nu*sin(lat_rads)*pow(cos(lat_rads),3)/24)*(5-pow(tan(lat_rads),2)+9*e_prime_squared*pow(cos(lat_rads),2)+4*pow(e_prime_squared,2)*pow(cos(lat_rads),4))*pow(p,4);

    output_coordinate->eastings = k0*nu*cos(lat_rads)*p + (k0*nu*pow(cos(lat_rads),3)/6)*(1 - pow(tan(lat_rads),2) + e_prime_squared*pow(cos(lat_rads),2))*pow(p,3);

    return;
}

