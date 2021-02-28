//
//  ValueControls.cpp
//  Fresh
//
//  Created by Jeff Wofford on 9/30/19.
//  Copyright (c) 2019 Jeff Wofford. All rights reserved.
//

#include "ValueControls.h"
#include "FantasyConsole.h"

namespace
{
	fr::TextField::ptr createDefaultTextField()
	{
		const auto textField = fr::createObject< fr::TextField >();
		textField->setFont( "uni05_53_8" );
		textField->fontSize( 8 );
		
		return textField;
	}
}

namespace fr
{
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( ValueControl )

	void ValueControl::create( WeakPtr< FantasyConsole > console, const ValueControlOptions& options )
	{
		m_console = console;
		m_options = options;
		
		const auto name = split( options.variablePath ).back();
		m_label = createDefaultTextField();
		m_label->text( name );
		m_label->alignment( TextMetrics::Alignment::Right );
		m_label->position( -4, -4 );
		addChild( m_label );
	}
	
	void ValueControl::console( WeakPtr< FantasyConsole > console_ )
	{
		m_console = console_;
	}
	
	bool ValueControl::valid() const
	{
		return m_console && m_console->compiled() && !hasError();
	}
	
	void ValueControl::reset()
	{
		m_error.clear();
	}
	
	std::string ValueControl::valueString() const
	{
		return createString( m_options.variablePath << " = " << valueAsString() );
	}
	
	void ValueControl::update()
	{
		Super::update();
		
		if( valid() )
		{
			try
			{
				readValue();
			}
			catch( const std::exception& e )
			{
				m_error = e.what();
				release_error( m_error );
			}
			
			m_label->color( hasError() ? Color::Red : Color::White );
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( ValueControlBool )
	DEFINE_VAR( ValueControlBool, real, m_size );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( ValueControlBool )

	void ValueControlBool::create( WeakPtr< FantasyConsole > console, const ValueControlOptions& options )
	{
		Super::create( console, options );
	
		const auto background = createObject< Sprite >();
		addChild( background );
		background->setTextureByName( "white_simple" );
		background->scale(( m_size + 2 ) / 2.0f);
		background->color( 0xFF333333 );
		background->position( m_size / 2, 0 );
		addEventListener( TAPPED, FRESH_CALLBACK( onCheckClicked ));

		m_check = createObject< Sprite >();
		addChild( m_check );
		
		m_check->setTextureByName( "white_simple" );
		m_check->color( Color::DarkGreen );
		background->position( m_size / 2, 0 );
		m_check->scale( m_size / 2.0f );
	}
	
	void ValueControlBool::readValue()
	{
		ASSERT( m_console );
		
		m_value = m_console->getValueBool( m_options.variablePath );
		m_check->visible( m_value );
	}
	
	std::string ValueControlBool::valueAsString() const
	{
		return createString( std::boolalpha << m_value );
	}
	
	FRESH_DEFINE_CALLBACK( ValueControlBool, onCheckClicked, EventTouch )
	{
		m_check->visible( !m_check->visible() );

		if( valid() )
		{
			try
			{
				m_console->setValueBool( m_options.variablePath, m_check->visible() );
			}
			catch( const std::exception& e )
			{
				m_error = e.what();
				release_error( m_error );
			}
		}
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( ValueControlReal )
	DEFINE_VAR( ValueControlReal, vec2, m_size );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( ValueControlReal )

	void ValueControlReal::create( WeakPtr< FantasyConsole > console, const ValueControlOptions& options )
	{
		Super::create( console, options );

		const auto background = createObject< Sprite >();
		addChild( background );
		background->setTextureByName( "white_simple" );
		background->scale( m_size.x / 2.0f, 1 / 2.0f );
		background->color( 0xFF333333 );
		background->position( m_size.x / 2.0f, 0 );
		
		
		m_handle = createObject<Sprite>();
		addChild( m_handle );
		m_handle->isDragEnabled( true );
		m_handle->doMoveWithDrag( true );
		m_handle->minStageDistanceForDrag( 0 );
		m_handle->setTextureByName( "white_simple" );
		m_handle->scale( m_size.y / 2.0f );
		m_handle->color( Color::Blue );
		m_handle->addEventListener( DRAG_MOVE, FRESH_CALLBACK( onHandleDragged ));

		m_valueText = createDefaultTextField();
		addChild( m_valueText );
		m_valueText->position( m_size.x + 2, -4 );

		m_min = createDefaultTextField();
		addChild( m_min );
		m_min->fontSize( 6 );
		m_min->position( 0, 1 );
		m_min->alignment( TextMetrics::Alignment::Right );
		m_min->text( createString( std::fixed << std::setprecision( 2 ) << min() ));

		m_max = createDefaultTextField();
		addChild( m_max );
		m_max->fontSize( 6 );
		m_max->position( m_size.x, 1 );
		m_max->text( createString( std::fixed << std::setprecision( 2 ) << max() ));
	}
	
	void ValueControlReal::updateMinsMaxes( real value )
	{
		if( m_haveSeenValue )
		{
			m_detectedMin = std::min( value, m_detectedMin );
			m_detectedMax = std::max( value, m_detectedMax );
		}
		else
		{
			m_haveSeenValue = true;
			m_detectedMin = m_detectedMax = value;
			
			if( m_detectedMin < 0 )
			{
				m_detectedMax = 0;
			}
			else if( m_detectedMax >= 0 )
			{
				m_detectedMin = 0;
			}
		}
	
		m_detectedMax = std::max( m_detectedMax, m_detectedMin + 1.0f );

		m_valueText->text( createString( std::fixed << std::setprecision( 2 ) << value ));
		m_min->text( createString( std::fixed << std::setprecision( 2 ) << min() ));
		m_max->text( createString( std::fixed << std::setprecision( 2 ) << max() ));
	}
	
	std::string ValueControlReal::valueAsString() const
	{
		return createString( m_value );
	}

	void ValueControlReal::readValue()
	{
		ASSERT( m_console );
		
		m_value = m_console->getValueNumber( m_options.variablePath );
		
		updateMinsMaxes( m_value );

		if( !m_handle->isDragging() )
		{
			// Don't update handle while dragging.
		
			const auto proportion = fr::proportion( m_value, min(), max() );
			const auto x = fr::clamp( proportion, 0.0f, 1.0f );
			m_handle->position( fr::lerp( 0.0f, m_size.x, x ), 0 );
		}
	}
	
	real ValueControlReal::min() const
	{
		if( m_options.fixedRange() )
		{
			return m_options.min;
		}
		else if( m_haveSeenValue )
		{
			const auto roundScale = 1.0f; // TODO ( m_detectedMax - m_detectedMin )

			return roundToNearest( m_detectedMin, roundScale );
		}
		else
		{
			return 0.0f;
		}
	}
	
	real ValueControlReal::max() const
	{
		if( m_options.fixedRange() )
		{
			return m_options.max;
		}
		else if( m_haveSeenValue )
		{
			const auto roundScale = 1.0f; // TODO ( m_detectedMax - m_detectedMin )

			return roundToNearest( m_detectedMax, roundScale );
		}
		else
		{
			return 1.0f;
		}
	}

	void ValueControlReal::reset()
	{
		m_haveSeenValue = false;
	}
				
	FRESH_DEFINE_CALLBACK( ValueControlReal, onHandleDragged, EventTouch )
	{
		const auto initialX = m_handle->position().x;
		const auto x = fr::clamp( initialX, 0.0f, m_size.x );
		m_handle->position( x, 0 );
		
		if( valid() )
		{
			try
			{
				auto proportion = fr::proportion( initialX, 0.0f, m_size.x );
				
				const real outageDrag = 0.65f;
				if( proportion < 0 )
				{
					proportion = lerp( proportion, 0.0f, outageDrag );
				}
				else if( proportion > 1.0f )
				{
					proportion = lerp( proportion, 1.0f, outageDrag );
				}
				
				auto value = fr::lerp( min(), max(), proportion );
				if( m_options.fixedRange() )
				{
					value = fr::clamp( value, min(), max() );
				}
				m_console->setValueNumber( m_options.variablePath, value );
			}
			catch( const std::exception& e )
			{
				m_error = e.what();
				release_error( m_error );
			}
		}
	}

	///////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS( ValueControls )
	DEFINE_VAR( ValueControls, real, m_intervalY );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( ValueControls )

	void ValueControls::postLoad()
	{
		Super::postLoad();
		
		m_controlsHost = createObject< DisplayObjectContainer >();
		addChild( m_controlsHost );
		m_controlsHost->visible( false );
		
		m_hideShowButton = createDefaultTextField();
		addChild( m_hideShowButton );
		m_hideShowButton->isTouchEnabled( true );
		m_hideShowButton->text( "+" );
		m_hideShowButton->position( 34, -10 );
		ASSERT( m_hideShowButton->isTouchable() );
		m_hideShowButton->addEventListener( TAPPED, FRESH_CALLBACK( onHideShow ));
	}
	
    void ValueControls::add( ValueControlOptions options, WeakPtr< FantasyConsole > console )
    {
		ValueControl::ptr control;

		if( options.type == ValueControlOptions::Type::Detect )
		{
			if( console )
			{
				const int luaType = console->getLuaType( options.variablePath );
				switch( luaType )
				{
					case LUA_TBOOLEAN:
						options.type = ValueControlOptions::Type::Bool;
						break;
						
					default:
						options.type = ValueControlOptions::Type::Real;
						break;
				}
			}
		}


		switch( options.type )
		{
			case ValueControlOptions::Type::Bool:
				control = createObject< ValueControlBool >();
				break;
			
			case ValueControlOptions::Type::Real:
			case ValueControlOptions::Type::Detect:
				control = createObject< ValueControlReal >();
				break;
		}
		
		m_controlsHost->addChild( control );
		
		control->create( console, options );
		
		saveState();
    }

	void ValueControls::reset( const std::string& path )
	{
		reset( find( path ));
	}
	
	void ValueControls::reset( size_t i )
	{
		if( i < m_controlsHost->numChildren() )
		{
			m_controlsHost->getChildAt( i )->as< ValueControl >()->reset();
		}
	}

	size_t ValueControls::find( const std::string& path ) const
	{
		size_t i = 0;
		size_t found = -1;
		m_controlsHost->forEachChild< ValueControl >( [&]( const ValueControl& control )
		{
			if( control.options().variablePath == path )
			{
				found = i;
			}
			
			++i;
		});

		return found;
	}
	
    void ValueControls::remove( const std::string& path )
    {
		remove( find( path ));
    }
	
	void ValueControls::remove( size_t i )
	{
		if( i < m_controlsHost->numChildren() )
		{
			m_controlsHost->removeChildAt( i );
			saveState();
		}
	}
	
	void ValueControls::removeLast()
	{
		remove( m_controlsHost->numChildren() - 1 );
	}
	
	void ValueControls::clear()
	{
		m_controlsHost->removeChildren();
	}
	
	void ValueControls::update()
	{
		if( !m_controlsHost->visible() ) { return; }
		
		const ValueControl* prior = nullptr;
		m_controlsHost->forEachChild< ValueControl >( [&]( ValueControl& control )
		{
			control.wantsUpdate( m_active );
			
			if( prior )
			{
				control.position( lerp( control.position(), prior->position() + vec2( 0, m_intervalY ), 0.2f ));
			}
			else
			{
				control.position( lerp( control.position(), vec2{}, 0.2f ));
			}
			
			prior = &control;
		});
		
		Super::update();
	}
	
	void ValueControls::restoreState( const std::string& controlsString )
	{
		clear();
		
		std::istringstream str( controlsString );
		
		while( str.peek() != str.eof() && str.good() )
		{
			ValueControlOptions options;
			str >> options.variablePath;
			
			if( options.variablePath.empty() )
			{
				break;
			}
			
			std::string typeStr;
			str >> typeStr;
			options.type = typeStr == "bool" ? ValueControlOptions::Type::Bool : ValueControlOptions::Type::Real;
			
			str >> options.min;
			str >> options.max >> std::ws;
			
			add( options, nullptr );
		}
	}
	
	void ValueControls::console( WeakPtr< FantasyConsole > console )
	{
		m_controlsHost->forEachChild< ValueControl >( [&]( ValueControl& control )
		{
			control.console( console );
		});
	}
	
	std::string ValueControls::valuesString() const
	{
		std::ostringstream str;
		m_controlsHost->forEachChild< ValueControl >( [&]( const ValueControl& control )
		{
			str << control.valueString() << "\n";
		});
		
		return str.str();
	}
	
	std::string ValueControls::saveState() const
	{
		std::ostringstream str;
		m_controlsHost->forEachChild< ValueControl >( [&]( const ValueControl& control )
		{
			str << control.options().toString();
		});
		
		return str.str();
	}
	
	FRESH_DEFINE_CALLBACK( ValueControls, onHideShow, EventTouch )
	{
		m_controlsHost->visible( !m_controlsHost->visible() );
		
		m_hideShowButton->text( m_controlsHost->visible() ? "X" : "+" );
	}
}
