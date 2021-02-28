//
//  ApplicationStaged.cpp
//  Fresh
//
//  Created by Jeff Wofford on 11/21/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#include "ApplicationStaged.h"
#include "escaped_string.h"
#include "Stage.h"
#include "FreshFile.h"
#include "Archive.h"
#include "EventTouch.h"
#include "Renderer.h"
#include "CommandProcessor.h"

// Pre-referencing common Fresh classes.
//
#include "MovieClip.h"
#include "SimpleButton.h"
#include "Font.h"
#include "SimpleMesh.h"
#include "ShaderUniformUpdaterConcrete.h"
#include "TextGroup.h"
#include "TextField.h"

namespace
{
	using namespace fr;

#define APP_CONFIG_VAR( type, name )	\
	public: SYNTHESIZE( type, name )	\
	private: VAR( type, m_##name )
	
#define APP_CONFIG_DVAR( type, name, default )	\
	public: SYNTHESIZE( type, name )	\
	private: DVAR( type, m_##name, default )
	
	class AppStageConfig : public AppConfig
	{
		FRESH_DECLARE_CLASS( AppStageConfig, AppConfig )
	public:
		
		APP_CONFIG_VAR( path, stageManifestPath )
		APP_CONFIG_VAR( path, alternativeStageManifestPath )
		APP_CONFIG_DVAR( bool, swapAlternativeStage, false );
	};
	
	
#undef APP_CONFIG_VAR
#undef APP_CONFIG_DVAR
	
#define APP_CONFIG_VAR( type, name )	\
	DEFINE_VAR( AppStageConfig, type, m_##name );
	
#define APP_CONFIG_DVAR( type, name, default )	\
	DEFINE_VAR( AppStageConfig, type, m_##name );

	FRESH_DEFINE_CLASS( AppStageConfig )
	APP_CONFIG_VAR( path, stageManifestPath )
	APP_CONFIG_VAR( path, alternativeStageManifestPath )
	APP_CONFIG_DVAR( bool, swapAlternativeStage, false );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( AppStageConfig )
	
	template< class T >
	void use()
	{
		typename T::ptr t = fr::createObject< T >();
	}
}

namespace fr
{
	void forceInclusion()
	{
		ASSERT( false );	// Don't actually call this.
		
		// Reference Fresh static library functions and classes that might otherwise be stripped due to apparent non-use.
		use< FontLoader >();
		use< SimpleMeshLoader >();
		use< ShaderUniformUpdaterConcrete< mat4 >>();
		use< ShaderUniformUpdaterConcrete< Color >>();
		use< MovieClip >();
		use< SimpleButton >();
		use< TextGroup >();
		use< TextField >();
	}
	
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	ApplicationStaged::ApplicationStaged( const path& configFilePath )
	:	Application( configFilePath )
	{}
	
	ApplicationStaged::~ApplicationStaged()
	{
		m_stage = nullptr;
		m_stagePackage = nullptr;
	}

	void ApplicationStaged::renderStage( DisplayObject::ptr stage ) const
	{
		ASSERT( stage );		
		stage->render( 1.0 );
	}

	Vector2i ApplicationStaged::stageToWindow( const vec2& p ) const
	{ 
		return vector_cast< int >( Renderer::instance().world2DToScreen( p ));
	}

	void ApplicationStaged::onPreFirstUpdate()
	{
		Application::onPreFirstUpdate();
		
		ASSERT( !m_stage );
	
		const AppStageConfig* const myConfig = dynamic_cast< const AppStageConfig* >( &config() );
		
		if( !myConfig )
		{
			con_error( "ApplicationStaged's config object was not of type AppStageConfig. We need an AppStageConfig with a valid stageManifestPath path!" );
		}
		else
		{
			const bool useAlternative = isApplicationAlternativeStartupKeyDown() ^ myConfig->swapAlternativeStage();
			
			auto stagePath = myConfig->stageManifestPath();
			
			if( stagePath.empty() || ( useAlternative && !myConfig->alternativeStageManifestPath().empty() ))
			{
				stagePath = myConfig->alternativeStageManifestPath();
				dev_trace( "Using alternative stage path: " << stagePath );
			}
			
			if( !stagePath.empty() )
			{
				m_stage = loadStagePackage( stagePath );
			}
			else
			{
				con_error( "No stage path indicated." );
			}
		}

		if( !m_stage )
		{
			con_error( "Could not create stage. Application will fail." );
			quit( 1 );
		}
	}
	
	void ApplicationStaged::update()
	{
		// Update and render the stage.
		//
		if( m_stage )
		{
			m_stage->update();
			
			renderStage( m_stage );
		}
		
		Application::update();		
	}
	
	void ApplicationStaged::onTerminationThreat()
	{
		if( m_stage ) 
		{
			m_stage->onAppMayTerminate();
		}
		
		Application::onTerminationThreat();
	}
	
	void ApplicationStaged::onTerminating()
	{
		Application::onTerminating();
		
		if( m_stage )
		{
			m_stage->onAppMayTerminate();
			m_stage = nullptr;
			m_stagePackage = nullptr;
		}
	}
	
	void ApplicationStaged::onWaking()
	{
		if( m_stage )
		{
			m_stage->onAppWaking();
		}
		
		Application::onWaking();
	}
	
	void ApplicationStaged::onSleeping()
	{
		// Call the base class first so that termination threat handling (which tends to be milder than
		// sleep handling, e.g. saving.) occurs before sleep handling (which may stop music, purge assets,
		// etc.).
		//
		Application::onSleeping();
		
		if( m_stage )
		{
			m_stage->onAppSleeping();
		}
	}
	
	Stage::ptr ApplicationStaged::loadStagePackage( const path& packagePath )
	{
		REQUIRES( !packagePath.empty() );
		
		dev_trace( "Loading stage from '" << packagePath << "'." );
		
		// Load the stage manifest, hopefully creating the stage.
		//
		try
		{
			m_stagePackage = loadPackage< DisplayPackage >( packagePath, "~stage" );
		}
		catch( const std::exception& e )
		{
			con_error( "Failed to load stage package '" << packagePath.filename() << "': " << e.what() );
			return nullptr;
		}
		
		Stage::ptr stage = dynamic_freshptr_cast< Stage::ptr >( m_stagePackage->root() );
		
		if( stage )
		{
			completeStageLoading( *stage );
			stage->beginPlay();
		}
		else
		{
			con_error( "Failed to load stage from stage package '" << packagePath.filename() << "'." );
		}
		
		return stage;
	}
	
	void ApplicationStaged::completeStageLoading( Stage& stage ) const
	{
		stage.fixupStageDimensions( vector_cast< real >( getWindowDimensions() ));
		stage.onStageLoaded();
	}
	
	//////////////////////////////////////////////////////////////////// 
	
#define SEND_TOUCHES( func, strType, enumType )	\
	for( ; begin != end; ++begin )	\
	{	\
		m_stage->func(	\
							   EventTouch(strType,	\
										  begin->touchId,	\
										  enumType,	\
										  begin->nTaps,	\
										  begin->position,	\
										  begin->lastPosition,	\
										  begin->nTouches,	\
										  begin->iTouch,	\
										  begin->wheelDelta	\
								)	\
							   );	\
	}		

	void ApplicationStaged::onTouchesBegin( TouchIter begin, TouchIter end )
	{
		if( m_stage && begin != end )
			SEND_TOUCHES( onTouchBegin, EventTouch::TOUCH_BEGIN, EventTouch::TouchPhase::Begin )
	}
	
	void ApplicationStaged::onTouchesMove( TouchIter begin, TouchIter end )
	{
		if( m_stage && begin != end  )
			SEND_TOUCHES( onTouchMove, EventTouch::TOUCH_MOVE, EventTouch::TouchPhase::Move )
	}
	
	void ApplicationStaged::onTouchesEnd( TouchIter begin, TouchIter end )
	{
		if( m_stage && begin != end  )
			SEND_TOUCHES( onTouchEnd, EventTouch::TOUCH_END, EventTouch::TouchPhase::End )
	}
	
	void ApplicationStaged::onTouchesCancelled( TouchIter begin, TouchIter end )
	{
		if( m_stage && begin != end  )
			SEND_TOUCHES( onTouchCancelled, EventTouch::TOUCH_CANCELLED, EventTouch::TouchPhase::Cancelled )
	}
	
	void ApplicationStaged::onWheelMove( TouchIter begin, TouchIter end )
	{
		if( m_stage && begin != end  )
			SEND_TOUCHES( onWheelMove, EventTouch::WHEEL_MOVE, EventTouch::TouchPhase::WheelMove )
	}

#undef SEND_TOUCHES

	void ApplicationStaged::onKeyUp( const EventKeyboard& event )
	{
		if( m_stage )
		{
			m_stage->onKeyUp( EventKeyboard( event.type(),
											 m_stage,
											 event.charCode(),
											 event.key(),
											 event.isAltOptionDown(),
											 event.isCtrlCommandDown(),
											 event.isShiftDown(),
											event.isAHeldRepeat(),
											m_stage ));
		}
	}
	
	void ApplicationStaged::onKeyDown( const EventKeyboard& event )
	{
		if( m_stage )
		{
			m_stage->onKeyDown( EventKeyboard( event.type(),
										 m_stage,
										 event.charCode(),
										 event.key(),
										 event.isAltOptionDown(),
										 event.isCtrlCommandDown(),
										 event.isShiftDown(),
										 event.isAHeldRepeat(),
										 m_stage ));
		}
	}

	void ApplicationStaged::onGainedFocus()
	{
		if( m_stage )
		{
			m_stage->onGainedFocus();
		}
	}

	void ApplicationStaged::onLostFocus()
	{
		if( m_stage )
		{
			m_stage->onLostFocus();
		}
	}
	
	void ApplicationStaged::onWindowReshape()
	{
		if( m_stage )
		{
			m_stage->onWindowReshape( getWindowDimensions() );
		}
	}
}
