function dB = rssi_to_db( rssi_value )
    dB = rssi_value*55/255 - 90;
    