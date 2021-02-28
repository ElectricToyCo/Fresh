//
//  ApiMap.h
//  Fresh
//
//  Created by Jeff Wofford on 8/05/18.
//  Copyright (c) 2018 Jeff Wofford. All rights reserved.
//

public:

void map( int cel_x, int cel_y, int sx, int sy, int cel_w, int cel_h, int layer );
void mapdraw( int cel_x, int cel_y, int sx, int sy, int cel_w, int cel_h, int layer );
int mget( real x, real y, int layer );
void mset( real x, real y, int v, int layer );
