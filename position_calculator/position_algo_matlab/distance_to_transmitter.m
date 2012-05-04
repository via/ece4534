function [distance] = distance_to_transmitter( rx_power, tx_power, rx_gain, tx_gain, freq )
    lambda = 299792458/freq;
    distance = (10^((rx_power-tx_power-rx_gain-tx_gain)/20))*4*pi/lambda;
    distance = 1/distance;
    