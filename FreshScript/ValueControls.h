//
//  ValueControls.h
//  Fresh
//
//  Created by Jeff Wofford on 9/30/19.
//  Copyright (c) 2019 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_ValueControls_h
#define Fresh_ValueControls_h

#include "MovieClip.h"
#include "TextField.h"

namespace fr
{
	
	class FantasyConsole;
	
	struct ValueControlOptions
	{
		std::string variablePath;
		
        enum class Type {
			Detect,
            Bool,
            Real,
        };
		
		Type type;
		
		// If switched or equal, non-fixed.
		real min = 0;
		real max = 0;
		
		bool fixedRange() const {
			return max > min;
		}
		
		std::string toString() const
		{
			std::ostringstream str;
			str << variablePath << " ";
			str << ( type == Type::Bool ? "bool" : "real" ) << " ";
			str << min << " ";
			str << max << "\n";
			return str.str();
		}
	};

    class ValueControl : public MovieClip
    {
        FRESH_DECLARE_CLASS_ABSTRACT( ValueControl, MovieClip );
    public:
        
		virtual void create( WeakPtr< FantasyConsole > console_, const ValueControlOptions& options );
		void console( WeakPtr< FantasyConsole > console_ );
		
		virtual void update() override;
		
		SYNTHESIZE_GET( ValueControlOptions, options );
		
		virtual void readValue() = 0;
		
		SYNTHESIZE_GET( std::string, error );		// Empty if no error.
		bool hasError() const { return m_error.empty() == false; }
		void clearError() { m_error.clear(); }
		
		bool valid() const;
		virtual void reset();
		
		std::string valueString() const;
		
		virtual std::string valueAsString() const = 0;
		
    protected:
		
		WeakPtr< FantasyConsole > m_console;
		ValueControlOptions m_options;
		std::string m_error;
		TextField::ptr m_label;
    };
	
	class ValueControlBool : public ValueControl
	{
		FRESH_DECLARE_CLASS( ValueControlBool, ValueControl );
	public:
		virtual void create( WeakPtr< FantasyConsole > console, const ValueControlOptions& options ) override;
		
		virtual void readValue() override;
		
		virtual std::string valueAsString() const override;
		
	private:
		
		DVAR( real, m_size, 2 );
		
		Sprite::ptr m_check;
		bool m_value = true;
		
		FRESH_DECLARE_CALLBACK( ValueControlBool, onCheckClicked, EventTouch );
	};

	class ValueControlReal : public ValueControl
	{
		FRESH_DECLARE_CLASS( ValueControlReal, ValueControl );
	public:
		virtual void create( WeakPtr< FantasyConsole > console, const ValueControlOptions& options ) override;
		
		virtual void readValue() override;
			
		real min() const;
		real max() const;
		
		virtual void reset() override;
		
		virtual std::string valueAsString() const override;

	protected:
		
		void updateMinsMaxes( real value );
		
	private:
		
		DVAR( vec2, m_size, vec2( 20, 4 ) );
		
		bool m_haveSeenValue = false;
		real m_detectedMin = 0;
		real m_detectedMax = 0;
		
		real m_value = 0;
		
		Sprite::ptr m_handle;
		TextField::ptr m_valueText;
		TextField::ptr m_min;
		TextField::ptr m_max;
		
		FRESH_DECLARE_CALLBACK( ValueControlReal, onHandleDragged, EventTouch );
	};

	class ValueControls : public DisplayObjectContainer
	{
		FRESH_DECLARE_CLASS( ValueControls, DisplayObjectContainer );
	public:
        
        virtual void add( ValueControlOptions options, WeakPtr< FantasyConsole > console );
        virtual void reset( const std::string& path );
		virtual void reset( size_t i );
        virtual void remove( const std::string& path );
		virtual void remove( size_t i );
		virtual void removeLast();
		virtual void clear();
		
		virtual void update() override;
        
		SYNTHESIZE( bool, active );
		
		void console( WeakPtr< FantasyConsole > console );

		std::string valuesString() const;
		
		std::string saveState() const;
		void restoreState( const std::string& controlsString );		
		
		virtual void postLoad() override;
		
	protected:

		size_t find( const std::string& path ) const;
		
	private:
		
		DVAR( real, m_intervalY, 8 );
		
		bool m_active = false;
		
		DisplayObjectContainer::ptr m_controlsHost;
		TextField::ptr m_hideShowButton;
		
		FRESH_DECLARE_CALLBACK( ValueControls, onHideShow, EventTouch );
	};
	
}

#endif
