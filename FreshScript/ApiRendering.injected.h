//
//  ApiRendering.injected.h
//  Fresh
//
//  Created by Jeff Wofford on 6/24/18.
//  Copyright (c) 2018 Jeff Wofford. All rights reserved.
//

public:

void cls( uint color );
void camera( real x, real y );
void rect( real left, real top, real right, real bottom, uint color, real lineThickness );
void rectfill( real left, real top, real right, real bottom, uint color );
void arc( real x, real y, real radius, real arcLength, real rotation, uint color, real lineThickness, int segments );
void arcfill( real x, real y, real radius, real arcLength, real rotation, uint color, int segments );
void circ( real x, real y, real radius, uint color, real lineThickness, int segments );
void circfill( real x, real y, real radius, uint color, int segments );
void line( real x0, real y0, real x1, real y1, uint color, real lineRhickness, std::string capType );
void print( std::string message, real x, real y, uint color, int font, real scale, bool noNewline );
void spr( int spriteIndex, real x, real y, int spritesWid, int spritesHgt, bool flipX, bool flipY, uint color, uint additiveColor );
void sspr( int sx, int sy, int sw, int sh, real dx, real dy, real dw, real dh, bool flipX, bool flipY, uint color, uint additiveColor );
int  fget( int spriteIndex, int mask );
void fset( int spriteIndex, int flagIndexOrBitfield, int valueOrUndefined );
void fillp( int bitpattern );

private:

SimpleMesh::ptr m_quadMesh;
vec2 m_nextDefaultPrintPosition;
