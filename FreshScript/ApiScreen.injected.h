//
//  ApiScreen.injected.h
//  Fresh
//
//  Created by Jeff Wofford on 8/18/19.
//  Copyright (c) 2019 Jeff Wofford. All rights reserved.
//

public:

void screen_size( int wid, int hgt );
int screen_wid();
int screen_hgt();
void filter_mode( const std::string& mode );
void barrel( real distortion );
void bloom( real intensity, real contrast, real brightness );
void burn_in( real amount );
void chromatic_aberration( real aberration );
void noise( real amount, real rescan_r, real rescan_g, real rescan_b, real rescan_a );
void saturation( real sat );
void color_multiplied( real r, real g, real b, real a );
void bevel( real intensity, int type );

private:

TimeType m_screenTransitionTime = 1.0;

void changeScreenState( std::function< void( ScreenEffects::ShaderState& state ) >&& mutator );
