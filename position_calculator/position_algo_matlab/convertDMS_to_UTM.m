function [eastings,northings] = convertDMS_to_UTM( latDeg, latMins, lonDeg, lonMins )
    equatorial = 6378137;
    polar = 6356752.3142;
    flattening = (equatorial-polar)/equatorial;
    k0 = 0.9996;
    e = sqrt(1-(polar^2)/(equatorial^2));
    A = 1-(e^2)/4-3*(e^4)/64-5*(e^6)/256;
    B = 3*(e^2)/8+3*(e^4)/32+45*(e^6)/1024;
    C = 15*(e^4)/256+45*(e^6)/1024;
    D = 35*(e^6)/3072;
    e_prime_squared = (e^2)/(1-e^2);
    n = (equatorial-polar)/(equatorial+polar);
    long0 = ((84+78)/2)*pi/180;
    
    latitude = latDeg + latMins/60;
    longitude = lonDeg + lonDeg/60;
    
    %convert to radians
    lat_rads = latitude*pi/180;
    lon_rads = longitude*pi/180;
    
    %calculate final constants
    nu = equatorial/sqrt( 1-(e^2)*sin(lat_rads)^2 );
    p = lon_rads - long0;
    
    %determine the Meridional Arc
    M = A*lat_rads - B*sin(2*lat_rads) + C*sin(4*lat_rads) - D*sin(6*lat_rads);
    M = M*equatorial;
    %calculate northings and eastings
    K1 = M*k0;
    K2 = k0*nu*sin(2*lat_rads)/4;
    K3 = (k0*nu*sin(lat_rads)*(cos(lat_rads)^3)/24)*(5-(tan(lat_rads)^2)+9*e_prime_squared*(cos(lat_rads)^2)+4*(e_prime_squared^2)*(cos(lat_rads)^4));
    K4 = k0*nu*cos(lat_rads);
    K5 = (k0*nu*(cos(lat_rads)^3)/6)*(1 - (tan(lat_rads)^2) + e_prime_squared*(cos(lat_rads)^2));
    northings = K1 + K2*p^2 + K3*p^4;
    eastings = K4*p + K5*p^3 + 500000;
    
    