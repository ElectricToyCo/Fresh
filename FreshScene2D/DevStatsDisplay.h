/*
 *  DevStatsDisplay.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 11/20/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_DEV_STATS_DISPLAY
#define FRESH_DEV_STATS_DISPLAY

#include "Sprite.h"
#include "FreshTime.h"

namespace fr
{
	class TextField;
	
	class DevStatsDisplay : public Sprite
	{
	public:
		
		SYNTHESIZE_GET( SmartPtr< TextField >, textField );
		
		SYNTHESIZE_GET( size_t, maxHistoryEntries );
		void maxHistoryEntries( size_t newSize );
		SYNTHESIZE( rect, historyRect );

		SYNTHESIZE( TimeType, targetFPS );
		SYNTHESIZE( TimeType, maxDisplayedFPS );
		
		SYNTHESIZE( bool, doDrawHistory );
		
		virtual void update() override;
		
		virtual void clearFPSColorBands();
		virtual void addFPSColorBand( TimeType minFPSForBand, Color bandColor );
		
		virtual void postLoad() override;
		
	protected:
		
		virtual void drawHistoryGraph();
		
	private:
		
		typedef std::vector< std::pair< TimeType, Color > > ColorTimes;
		typedef std::vector< TimeType > HistoryDeltaTime;
		
		VAR( SmartPtr< TextField >, m_textField );
		VAR( Sprite::ptr, m_historyDisplay );
		DVAR( size_t, m_maxHistoryEntries, 200 );
		DVAR( rect, m_historyRect, rect( 0, 20, 200, 20+50 ));
		DVAR( TimeType, m_targetFPS, 30.0 );
		DVAR( TimeType, m_maxDisplayedFPS, 40.0 );
		DVAR( bool, m_doDrawHistory, true );
		VAR( ColorTimes, m_fpsColorBands );
		
		HistoryDeltaTime m_historyFPS;
		size_t m_oldestHistoryEntry = 0;
		
		FRESH_DECLARE_CLASS( DevStatsDisplay, Sprite );
		
	};
	
}


#endif
