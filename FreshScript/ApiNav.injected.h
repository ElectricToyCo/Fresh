//
//  ApiNav.injected.h
//  Fresh
//
//  Created by Jeff Wofford on 10/11/19.
//  Copyright (c) 2019 Jeff Wofford. All rights reserved.
//

public:

std::tuple< bool, real, real > navdest( real desiredDestinationX, real desiredDestinationY, real radius, real originX, real originY );
std::vector< std::tuple< real, real >> nav( real fromX, real fromY, real toX, real toY, real actorRadius, bool smooth );

