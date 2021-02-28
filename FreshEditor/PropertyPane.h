//
//  PropertyPane.h
//  Fresh
//
//  Created by Jeff Wofford on 1/16/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_PropertyPane_h
#define Fresh_PropertyPane_h

#include "MovieClip.h"
#include "TextField.h"

namespace fr
{
	
	class PropertyControl : public Sprite
	{
	public:
		
		static const char* PROPERTY_CONTROL_CHANGING;
		static const char* PROPERTY_CONTROL_FINISHED_CHANGE;
		
		class Changed : public Event
		{
		public:
			
			Changed( Event::TypeRef type_, PropertyControl& control, const std::string& value )
			:	Event( type_, &control )
			,	m_control( control )
			,	m_newValue( value )
			{}
			
			PropertyControl& control() const;
			SYNTHESIZE_GET( std::string, newValue );
			
		private:
			PropertyControl&	m_control;
			std::string			m_newValue;
		};
		
		virtual void setValueString( const std::string& strValue ) = 0;
		virtual std::string getValueString() const = 0;
		virtual void maxWidth( real width )
		{
			m_maxWidth = width;
		}
		
		virtual real enforcedHeight() const	{ return 12.0f; }
		
		void attachToProperty( const PropertyAbstract* property ) { m_attachedProperty = property; }
		const PropertyAbstract* attachedProperty() const { return m_attachedProperty; }
		
	protected:
		
		real m_maxWidth = 0;
		bool m_hasValidValue = false;
		
		// Called when the control value has been changed *specifically* by the direct
		// manipulation of the control (as opposed to some outside change).
		//
		virtual void onValueChangedByControl( bool ongoingElseFinal );
		
		// Called whenever the value changes (or finishes changing) for whatever reason.
		// Subclasses may override this to respond.
		//
		virtual void onValueChanged( bool ongoingElseFinal );
		
		// Dragging support.
		//
		virtual void onDragBegin( const EventTouch& event );
		virtual void onDragMove( const EventTouch& event );
		virtual void onDragEnd( const EventTouch& event );
		
		// Common processing for all drag movements, though they may be overridden.
		//
		virtual void updateFromDrag( const vec2& dragPositionLocal, bool ongoingElseFinal );
		
	private:
		
		const PropertyAbstract* m_attachedProperty = nullptr;
		
		FRESH_DECLARE_CLASS_ABSTRACT( PropertyControl, Sprite );
	};

	/////////////////////////////////////////////////////////////////////////
	
	
	class PropertyItem;
	class PropertyPane : public MovieClip
	{
	public:
		
		static const char* SUBJECTS_CHANGED;
		
		
		typedef std::vector< DisplayObject::wptr > Subjects;
		
		virtual void beginEditing( const Subjects& subjects );
		virtual void updatePane();
		
	protected:
		
		virtual void clearPane();
		virtual void setupPane();
		virtual void setupPropertyItem( PropertyItem& propertyItem, const PropertyAbstract& property ) const;
		virtual void adjustItemWidths( real controlOffsetsX, real controlMaxWidth = 0 );
		
		void forEachProperty( const std::function< void ( const PropertyAbstract&, size_t ) >& fnPerProperty );
		
		virtual void updatePropertyFromSubjects( const PropertyAbstract& property, size_t iProperty );
		virtual void updateSubjectsProperty( const PropertyAbstract& property, const std::string& value, bool ongoingElseFinal );
		
		SmartPtr< PropertyItem > itemForProperty( const PropertyAbstract& property ) const;
		
		SmartPtr< PropertyControl > createControl( PropertyAbstract::ControlType controlType ) const;
		
	private:
		
		typedef std::map< PropertyAbstract::ControlType, ClassInfo::Name > MapControlTypesToClassNames;
		
		VAR( ClassInfo::cptr, m_propertyItemClass );
		VAR( MapControlTypesToClassNames, m_controlClasses );
		DVAR( real, m_itemPaddingVertical, 6.0f );
		DVAR( real, m_gutterWidth, 6.0f );
		DVAR( real, m_paneMaxWidth, 400.0f );
		VAR( TextField::ptr, m_caption );
		
		DisplayObjectContainer::ptr m_itemHost;
		
		Subjects m_subjects;
		ClassInfo::cptr m_commonBase = nullptr;
		
		FRESH_DECLARE_CALLBACK( PropertyPane, onControlChanging, PropertyControl::Changed );
		FRESH_DECLARE_CALLBACK( PropertyPane, onControlFinishedChange, PropertyControl::Changed );
		
		FRESH_DECLARE_CLASS( PropertyPane, MovieClip );
	};

	
}

#endif
