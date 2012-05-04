function [newPosition] = location_gradient_descent( receiversX, receiversY, distance_data, currentPosition, stepsize )
    K = zeros(3,1);
    y_deviation = zeros(3,1);
    x_deviation = zeros(3,1);
    for i = [1:3];
        K(i) = sqrt( (currentPosition(1) - receiversX(i)).^2 + (currentPosition(2) - receiversY(i)).^2); 
        x_deviation(i) = (distance_data(i) - K(i))*(-1/(2*K(i)))*(-2*currentPosition(1)+2*receiversX(i));
        y_deviation(i) = (distance_data(i) - K(i))*(-1/(2*K(i)))*(-2*currentPosition(2)+2*receiversY(i));
    end
    newPosition = currentPosition + 2*stepsize*[sum(x_deviation),sum(y_deviation)];
    
    