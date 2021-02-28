/*
 *  DevStatsDisplay.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 11/20/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "DevStatsDisplay.h"
#include "TextField.h"
#include "Objects.h"
#include "Stage.h"
#include <iomanip>


namespace
{
	using namespace fr;
	bool compareFPSBands( const std::pair< TimeType, Color >& a, const std::pair< TimeType, Color >& b )
	{
		return a.first < b.first;
	}	
}

namespace fr
{
	FRESH_DEFINE_CLASS( DevStatsDisplay )
	

	DEFINE_VAR( DevStatsDisplay, SmartPtr< TextField >, m_textField );
	DEFINE_VAR( DevStatsDisplay, Sprite::ptr, m_historyDisplay );
	DEFINE_VAR( DevStatsDisplay, size_t, m_maxHistoryEntries );
	DEFINE_VAR( DevStatsDisplay, rect, m_historyRect );
	DEFINE_VAR( DevStatsDisplay, TimeType, m_targetFPS );
	DEFINE_VAR( DevStatsDisplay, TimeType, m_maxDisplayedFPS );
	DEFINE_VAR( DevStatsDisplay, bool, m_doDrawHistory );
	DEFINE_VAR( DevStatsDisplay, ColorTimes, m_fpsColorBands );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( DevStatsDisplay )
	
	DevStatsDisplay::DevStatsDisplay( const ClassInfo& assignedClassInfo, NameRef objectName )
	:	Sprite( assignedClassInfo, objectName )
	,	m_textField( createObject< TextField >( name() + "text" ))
	,	m_historyDisplay( createObject< Sprite >( name() + "history" ))
	,	m_historyFPS( m_maxHistoryEntries )
	{
		m_textField->setFont( "Monaco26" );
		addChild( m_historyDisplay );
		addChild( m_textField );
	}
	
	void DevStatsDisplay::postLoad()
	{
		Super::postLoad();
		
		if( m_fpsColorBands.empty() )
		{
			addFPSColorBand( m_targetFPS * 1.05, Color::White );
			addFPSColorBand( m_targetFPS, Color::Green );
			addFPSColorBand( m_targetFPS * 0.75, Color::Orange );
			addFPSColorBand( m_targetFPS * 0.50, Color::Red );
		}
	}

	void DevStatsDisplay::maxHistoryEntries( size_t newSize )
	{
		m_maxHistoryEntries = newSize;
		
		m_historyFPS.resize( m_maxHistoryEntries );
		m_oldestHistoryEntry = 0;
	}
	
	void DevStatsDisplay::update()
	{		
		// Calculate and display frame timing information.
		//
		const TimeType deltaTimeSeconds = stage().lastFrameDurationReal();
		const TimeType fps = 1.0 / deltaTimeSeconds;

		std::stringstream statsMessage;		
		statsMessage << "fps: " << std::fixed << std::setfill( ' ' ) << std::setprecision( 1 ) << std::setw( 6 ) << fps << " ms: " << (deltaTimeSeconds * 1000.0);
		
		m_historyFPS[ m_oldestHistoryEntry ] = fps;
		++m_oldestHistoryEntry;
		
		// Wrap iterator.
		if( m_oldestHistoryEntry >= m_historyFPS.size() )
		{
			m_oldestHistoryEntry = 0;
		}
		
		m_textField->text( statsMessage.str() );
		
		if( m_doDrawHistory )
		{
			m_historyDisplay->position( m_historyRect.ulCorner() );
			m_historyDisplay->scale( m_historyRect.dimensions() );
			drawHistoryGraph();
		}
		m_historyDisplay->visible( m_doDrawHistory );
	}
	
	void DevStatsDisplay::clearFPSColorBands()
	{
		m_fpsColorBands.clear();
	}
	
	void DevStatsDisplay::addFPSColorBand( TimeType minFPSForBand, Color bandColor )
	{
		m_fpsColorBands.push_back( std::make_pair( minFPSForBand, bandColor ));
		std::sort( m_fpsColorBands.begin(), m_fpsColorBands.end(), compareFPSBands );		
	}
	
	void DevStatsDisplay::drawHistoryGraph()
	{
		Graphics& graphics = m_historyDisplay->graphics();
		graphics.clear();
		
		const real normalizedTargetFPS = static_cast< real >( m_targetFPS / m_maxDisplayedFPS );

		if( !m_historyFPS.empty() )
		{
			const float fRecordingSize = static_cast< float >( m_historyFPS.size() );
			
			real lastX = -1;
			real lastHeight = -1;
			
			for( size_t i = 0; i < m_historyFPS.size(); ++i )
			{
				float x = i / fRecordingSize;				
				size_t iNormalized = ( i + m_oldestHistoryEntry ) % m_historyFPS.size();
				
				const double fps = m_historyFPS[ iNormalized ];
				
				// What color band does this fps value fall under?
				//
				Color bandColor;
				getKeyframedValue( m_fpsColorBands.begin(), m_fpsColorBands.end(), fps, bandColor, true, false );
				
				
				const real height = std::max( 0.0f, 1.0f - static_cast< real >( fps / m_targetFPS * normalizedTargetFPS ));
				
				if( lastX >= 0 )
				{
					graphics.lineStyle( bandColor );
					graphics.moveTo( lastX, lastHeight );
					graphics.lineTo( x, height );
				}
				
				lastX = x;
				lastHeight = height;
			}
		}
							
		// Draw the standard 60fps line.
		//
		graphics.lineStyle( Color::DarkGreen );
		graphics.moveTo( 0, 1.0f - normalizedTargetFPS );
		graphics.lineTo( 1.0f, 1.0f - normalizedTargetFPS );
		
		// Draw the border.
		//
		graphics.lineStyle( 0.6f, 0.6f, 0.6f );
		graphics.drawRect( 0, 0, 1, 1 );
	}

	
}
