//
//  ApplicationStaged.h
//  Fresh
//
//  Created by Jeff Wofford on 11/21/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_ApplicationStaged_h
#define Fresh_ApplicationStaged_h

#include "Application.h"
#include "Stage.h"
#include "Color.h"
#include "DisplayPackage.h"

namespace fr
{
	
	class ApplicationStaged : public Application
	{
	public:
		
		explicit ApplicationStaged( const path& configFilePath );
		// REQUIRES( !configFilePath.empty() );
		
		virtual ~ApplicationStaged();
		
		virtual void onWaking() override;
		virtual void onSleeping() override;

		Vector2i stageToWindow( const vec2& p ) const;
		
		SmartPtr< Stage > getStage() const		{ return m_stage; }
		
		virtual void completeStageLoading( Stage& stage ) const;
		
		// Overrides from Application

		virtual void onPreFirstUpdate() override;
		virtual void update() override;
		virtual void onTouchesBegin( TouchIter begin, TouchIter end ) override;
		virtual void onTouchesMove( TouchIter begin, TouchIter end ) override;
		virtual void onTouchesEnd( TouchIter begin, TouchIter end ) override;
		virtual void onTouchesCancelled( TouchIter begin, TouchIter end ) override;
		virtual void onWheelMove( TouchIter begin, TouchIter end ) override;

		virtual void onKeyUp( const EventKeyboard& event ) override;
		virtual void onKeyDown( const EventKeyboard& event ) override;
		
		virtual void onGainedFocus() override;
		virtual void onLostFocus() override;
		virtual void onWindowReshape() override;

		virtual void onTerminationThreat() override;
		virtual void onTerminating() override;
		
	protected:
		
		virtual void renderStage( DisplayObject::ptr stage ) const;
		
		virtual Stage::ptr loadStagePackage( const path& packagePath );
		
	private:
		
		DisplayPackage::ptr m_stagePackage;
		SmartPtr< Stage > m_stage;
	};
	
}

#endif
