function do_various_simulations
    %enable pausing
    pause on
    
    %Setup our 'typical' node setup in an equillateral triangle
    receiverX = [0,-6.5,6.5];
    receiverY = [0,10.83,10.83];
    
    %Initial starting position
    current_position = [0.1,0.1];
    
    %stepsize
    stepsize = 0.1;
    
    %max number of iterations
    max_iterations = 15;
    
    position_data = zeros(max_iterations+1,2);
    position_data(1,:) = current_position;
    
    distance_data = [20,5,15];
    
    for i = [2:max_iterations+1]
        position_data(i,:) = location_gradient_descent( receiverX, receiverY, distance_data, position_data(i-1,:), stepsize );
    end
    
    figure(1);
    hold on;
    
    %Plot the three receiver nodes
    plot(receiverX,receiverY,'og');
    %Plot the iteration steps
    plot(position_data(:,1),position_data(:,2),'xb');
    %Plot circles
    for i = [1:3]
        xp = distance_data(i)*cos(0:0.1:2*pi);
        yp = distance_data(i)*sin(0:0.1:2*pi);
        plot( receiverX(i) + xp, receiverY(i) + yp, 'k');
    end
    
    pause
    
    
    position_data(1,:) = position_data(max_iterations+1,:);
    
    distance_data = [8,17,3];
    
    for i = [2:max_iterations+1]
        position_data(i,:) = location_gradient_descent( receiverX, receiverY, distance_data, position_data(i-1,:), stepsize );
    end
    
    figure(2);
    hold on;
    
    %Plot the three receiver nodes
    plot(receiverX,receiverY,'og');
    %Plot the iteration steps
    plot(position_data(:,1),position_data(:,2),'xb');
    %Plot circles
    for i = [1:3]
        xp = distance_data(i)*cos(0:0.1:2*pi);
        yp = distance_data(i)*sin(0:0.1:2*pi);
        plot( receiverX(i) + xp, receiverY(i) + yp, 'k');
    end
    
    pause
    
    position_data(1,:) = position_data(max_iterations+1,:);
    
    distance_data = [20,20,20];
    
    for i = [2:max_iterations+1]
        position_data(i,:) = location_gradient_descent( receiverX, receiverY, distance_data, position_data(i-1,:), stepsize );
    end
    
    figure(3);
    hold on;
    
    %Plot the three receiver nodes
    plot(receiverX,receiverY,'og');
    %Plot the iteration steps
    plot(position_data(:,1),position_data(:,2),'xb');
    %Plot circles
    for i = [1:3]
        xp = distance_data(i)*cos(0:0.1:2*pi);
        yp = distance_data(i)*sin(0:0.1:2*pi);
        plot( receiverX(i) + xp, receiverY(i) + yp, 'k');
    end
    
    pause
    
    position_data(1,:) = [0,6.25];
    
    distance_data = [20,20,20];
    
    for i = [2:max_iterations+1]
        position_data(i,:) = location_gradient_descent( receiverX, receiverY, distance_data, position_data(i-1,:), stepsize );
    end
    
    figure(4);
    hold on;
    
    %Plot the three receiver nodes
    plot(receiverX,receiverY,'og');
    %Plot the iteration steps
    plot(position_data(:,1),position_data(:,2),'xb');
    %Plot circles
    for i = [1:3]
        xp = distance_data(i)*cos(0:0.1:2*pi);
        yp = distance_data(i)*sin(0:0.1:2*pi);
        plot( receiverX(i) + xp, receiverY(i) + yp, 'k');
    end