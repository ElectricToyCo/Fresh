//
//  ApiScreen.cpp
//  Fresh
//
//  Created by Jeff Wofford on 8/18/19.
//  Copyright (c) 2019 Jeff Wofford. All rights reserved.
//

#include "FantasyConsole.h"
#include "Application.h"
#include "ApiImplementation.h"
#include "Stage.h"
#include "FreshVector.h"
#include "Color.h"
#include "ScreenEffects.h"
using namespace fr;
using namespace luacpp;

namespace
{
    template< typename T >
    T enumFromString( const std::string& string, T defaultIfErroneous )
    {
        fr::Destringifier destringifier{ string };
        T result = defaultIfErroneous;
        destringifier >> result;
        return result;
    }
}

namespace fr
{
    void FantasyConsole::changeScreenState( std::function< void( ScreenEffects::ShaderState& state ) >&& mutator )
    {
        mutator( m_consoleShaderState );
    }

    LUA_FUNCTION( screen_size, 1 );
    void FantasyConsole::screen_size( int wid, int hgt )
    {
        const auto aspectRatio = hasStage() ? stage().stageAspectRatio() : Application::instance().getWindowAspectRatio();

        m_stageAspectRatioForLastSpecifiedScreenSize = aspectRatio;
		m_lastSpecifiedScreenSize.set( wid, hgt );

        // Sanitize wid and hgt in terms of the stage aspect ratio, if known.
        if( wid <= 0 || hgt <= 0 )
        {
            if( wid <= 0 && hgt <= 0)
            {
                wid = m_defaultScreenSize.x;
                hgt = m_defaultScreenSize.y;
            }
            else if( wid <= 0 )
            {
                wid = hgt * aspectRatio;
            }
            else
            {
                hgt = wid / aspectRatio;
            }
        }

        SANITIZE( wid, m_defaultScreenSize.x, 1, 4096 );
        SANITIZE( hgt, wid, 1, 4096 );

        m_screen->create( vec2i( wid, hgt ));
    }

    LUA_FUNCTION( screen_wid, 0 );
    int FantasyConsole::screen_wid()
    {
        return m_screen->size().x;
    }

    LUA_FUNCTION( screen_hgt, 0 );
    int FantasyConsole::screen_hgt()
    {
        return m_screen->size().y;
    }

    LUA_FUNCTION( filter_mode, 1 );
    void FantasyConsole::filter_mode( const std::string& mode )
    {
        m_filterMode = enumFromString( mode, m_filterMode );
    }

    LUA_FUNCTION( barrel, 1 );
    void FantasyConsole::barrel( real distortion )
    {
        SANITIZE( distortion, 0, 0.0f, 1.0f );
        changeScreenState( [&]( auto& state )
                          {
                              state.barrelDistortion = distortion;
                          });
    }

    LUA_FUNCTION( bloom, 0 );
    void FantasyConsole::bloom( real intensity, real contrast, real brightness )
    {
        SANITIZE( intensity, 0, 0.0f, 100.0f );
        SANITIZE( brightness, 0, -1.0f, 1.0f );
        SANITIZE( contrast, 0, 1.0f, 10.0f );
        changeScreenState( [&]( auto& state )
                          {
                              state.bloomIntensity = intensity;
                              state.bloomBrightness = brightness;
                              state.bloomContrast = contrast;
                          });
    }

    LUA_FUNCTION( burn_in, 1 );
    void FantasyConsole::burn_in( real amount )
    {
        SANITIZE( amount, 0, 0.0f, 1.0f );
        changeScreenState( [&]( auto& state )
                          {
                              state.burnIn = amount;
                          });
    }

    LUA_FUNCTION( chromatic_aberration, 1 );
    void FantasyConsole::chromatic_aberration( real aberration )
    {
        SANITIZE( aberration, 0, -100.0f, 100.0f );
        changeScreenState( [&]( auto& state )
                          {
                              state.chromaticAberration = aberration;
                          });
    }

    LUA_FUNCTION( noise, 1 );
    void FantasyConsole::noise( real amount, real rescan_r, real rescan_g, real rescan_b, real rescan_a  )
    {
        SANITIZE( amount, 0, 0.0f, 1.0f );
        SANITIZE( rescan_r, 0, 0.0f, 1.0f );
        SANITIZE( rescan_g, rescan_r, 0.0f, 1.0f );
        SANITIZE( rescan_b, rescan_g, 0.0f, 1.0f );
        SANITIZE( rescan_a, 1.0f, 0.0f, 1.0f );
        changeScreenState( [&]( auto& state )
                          {
                              state.noiseIntensity = amount;
                              state.rescanColor.set( rescan_r, rescan_g, rescan_b, rescan_a );
                          });
    }

    LUA_FUNCTION( saturation, 1 );
    void FantasyConsole::saturation( real sat )
    {
        SANITIZE( sat, 1, 0.0f, 100.0f );
        changeScreenState( [&]( auto& state )
                          {
                              state.saturation = sat;
                          });
    }

    LUA_FUNCTION( color_multiplied, 1 );
    void FantasyConsole::color_multiplied( real r, real g, real b, real a )
    {
        SANITIZE( r, 1, 0.0f, 1.0f );
        SANITIZE( g, r, 0.0f, 1.0f );
        SANITIZE( b, g, 0.0f, 1.0f );
        SANITIZE( a, 1.0f, 0.0f, 1.0f );
        changeScreenState( [&]( auto& state )
                          {
                              state.colorMultiplied.set( r, g, b, a );
                          });
    }

    LUA_FUNCTION( bevel, 1 );
    void FantasyConsole::bevel( real intensity, int type )
    {
		DEFAULT( type, 0 )

        SANITIZE( intensity, 0, 0.0f, 100.0f );
        changeScreenState( [&]( auto& state )
						{
							state.bevelIntensity = intensity;
						    state.bevelType = type;
						});
    }
}

