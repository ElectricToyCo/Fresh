//
//  PropertyPane.cpp
//  Fresh
//
//  Created by Jeff Wofford on 1/16/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "PropertyPane.h"
#include "UIEditBox.h"
#include "UISliderKnob.h"
#include "UICheckbox.h"
#include "Editor.h"
#include "EdObjectBrowser.h"
using namespace fr;

namespace
{
	const char MULTIPLE_VALUES_STRING[] = "(different values)";
}

namespace fr
{

	///////////////////////////////////////////////////////////////////////////////////////////////
	// CONTROLS

	PropertyControl& PropertyControl::Changed::control() const
	{
		return m_control;
	}
	
	const char* PropertyControl::PROPERTY_CONTROL_CHANGING = "Property Control Changing";
	const char* PropertyControl::PROPERTY_CONTROL_FINISHED_CHANGE = "Property Control Finished Change";
	
	inline PropertyControl::PropertyControl( CreateInertObject c )
	:	Super( c )
	{
		isDragEnabled( true );
	}
	
	inline PropertyControl::PropertyControl( const ClassInfo& assignedClassInfo, NameRef name )
	:	Super( assignedClassInfo, name )
	{
		isDragEnabled( true );
	}
	
	void PropertyControl::onValueChangedByControl( bool ongoingElseFinal )
	{
		ASSERT( m_hasValidValue );
		
		onValueChanged( ongoingElseFinal );
		
		Changed event( ongoingElseFinal ? PROPERTY_CONTROL_CHANGING : PROPERTY_CONTROL_FINISHED_CHANGE, *this, getValueString() );
		dispatchEvent( &event );
	}
			
	void PropertyControl::onValueChanged( bool ongoingElseFinal ) {}
	
	void PropertyControl::onDragBegin( const EventTouch& event ) { updateFromDrag( event.location(), true ); Super::onDragBegin( event ); }
	void PropertyControl::onDragMove( const EventTouch& event )  { updateFromDrag( event.location(), true ); Super::onDragMove( event ); }
	void PropertyControl::onDragEnd( const EventTouch& event )   { updateFromDrag( event.location(), false ); Super::onDragEnd( event ); }
	
	void PropertyControl::updateFromDrag( const vec2& dragPositionLocal, bool ongoingElseFinal )
	{
		onValueChangedByControl( ongoingElseFinal );
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////////

	template< typename property_t >
	class PropertyControlTyped : public PropertyControl
	{
	public:
		
		virtual void setValueString( const std::string& strValue ) override
		{
			TIMER_AUTO( PropertyControlTyped::setValueString )
			
			m_hasValidValue = strValue != MULTIPLE_VALUES_STRING;	// Assume the string will prove valid, unless it is explicitly the "multiple value" string.
			
			if( m_hasValidValue )
			{
				try
				{
					Destringifier destringifier( strValue );
					destringifier >> m_value;
				}
				catch( ... )
				{
					m_hasValidValue = false;
				}
			}
			onValueChanged( false /*final change*/ );
		}
		
		virtual std::string getValueString() const override
		{
			if( m_hasValidValue )
			{
				std::ostringstream ss;
				ss << m_value;
				return ss.str();
			}
			else
			{
				return std::string();
			}
		}
		
	protected:
		
		property_t m_value;
		
		FRESH_DECLARE_CLASS_ABSTRACT( PropertyControlTyped, PropertyControl );
		
	};

	template< typename property_t >
	inline PropertyControlTyped< property_t >::PropertyControlTyped( CreateInertObject c )
	:	Super( c )
	{}
	
	template< typename property_t >
	inline PropertyControlTyped< property_t >::PropertyControlTyped( const ClassInfo& assignedClassInfo, fr::Object::NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	{}

	/////////////////////////////////////////////////////////////////////////////////////////////////
	
	class PropertyControlString : public PropertyControl
	{
		FRESH_DECLARE_CLASS( PropertyControlString, PropertyControl );
		
	public:
		
		virtual void setValueString( const std::string& strValue ) override
		{
			TIMER_AUTO( PropertyControlString::setValueString )

			ASSERT( m_editBox );
			m_editBox->text( strValue );
			m_hasValidValue = true;
		}
		
		virtual std::string getValueString() const override
		{
			ASSERT( m_editBox );
			return m_editBox->text();
		}
		
		virtual void maxWidth( real width ) override
		{
			Super::maxWidth( width );
			ASSERT( m_editBox );
			m_editBox->dimensions( vec2( width - m_editBox->position().x, m_editBox->dimensions().y ));
		}
		
		virtual real enforcedHeight() const override
		{
			return m_editBox->dimensions().y;
		}
		
		virtual void postLoad() override
		{
			Super::postLoad();
			
			m_editBox->dimensions( vec2( m_editBox->dimensions().x, m_editBox->textField()->fontSize() ));
			
			m_editBox->addEventListener( UIEditBox::END_EDITING, FRESH_CALLBACK( onDoneEditing ));
		}
		
	protected:
		
		SYNTHESIZE_GET( UIEditBox::ptr, editBox );
				
	private:
		
		VAR( UIEditBox::ptr, m_editBox );
		
		FRESH_DECLARE_CALLBACK( PropertyControlString, onDoneEditing, Event );
	};
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( PropertyControlString )
	DEFINE_VAR( PropertyControlString, UIEditBox::ptr, m_editBox );	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( PropertyControlString )
	
	FRESH_DEFINE_CALLBACK( PropertyControlString, onDoneEditing, Event )
	{
		onValueChangedByControl( false /* final */ );
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	
	class PropertyControlAngle : public PropertyControlTyped< angle >
	{
	protected:
		
		virtual void updateFromDrag( const vec2& dragPositionLocal, bool ongoingElseFinal  ) override
		{
			// No support for direct dragging of the whole control per se.
		}
		
		virtual void onValueChanged( bool ongoingElseFinal ) override
		{
			Graphics& myGraphics = graphics();
			
			myGraphics.clear();
			
			const vec2 center = getCenter();
			
			myGraphics.lineStyle( isDragging() ? Color::White : Color::LightGray );
			myGraphics.beginFill( isDragging() ? Color::Gray : Color::DarkGray );
			myGraphics.drawCircle( center, m_radius );
			myGraphics.endFill();
			
			if( m_hasValidValue )
			{
				myGraphics.moveTo( center );
				myGraphics.lineTo( center + vec2::makeAngleNormal( m_value ) * ( m_radius * 0.8f ));
			}
		}
		
		vec2 getCenter() const
		{
			return vec2( m_radius, enforcedHeight() * 0.5f );
		}
		
		virtual real enforcedHeight() const override
		{
			return m_radius * 2.0f;
		}
		
	private:
		
		DVAR( real, m_radius, 10.0f );
		
		FRESH_DECLARE_CLASS( PropertyControlAngle, PropertyControlTyped< angle > );
	};
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( PropertyControlAngle )
	DEFINE_VAR( PropertyControlAngle, real, m_radius );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( PropertyControlAngle )
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	
	class PropertyControlColor : public PropertyControl
	{
		FRESH_DECLARE_CLASS( PropertyControlColor, PropertyControl );
	public:
		
		virtual void setValueString( const std::string& strValue ) override
		{
			TIMER_AUTO( PropertyControlColor::setValueString )
			
			// Load the value into the color.
			//
			try
			{
				Destringifier destringifier( strValue );
				Color color;
				destringifier >> color;
				
				vec4 newHSVA = color.toHSVA();
				
				if( newHSVA.y == 0 )	// Totally desaturated. Hue will have been set to 0. But avoid that:
				{
					newHSVA.x = m_hsva.x;
				}
				m_hsva = newHSVA;
				
				m_hasValidValue = true;
			}
			catch( ... )
			{
				m_hasValidValue = false;
			}
			
			onValueChanged( false /* final change */ );
		}
		
		virtual std::string getValueString() const override
		{
			if( m_hasValidValue )
			{
				Color color;
				color.fromHSVA( m_hsva );
				
				std::ostringstream ss;
				ss << color;
				return ss.str();
			}
			else
			{
				return std::string();
			}
		}
		
		virtual void postLoad() override
		{
			Super::postLoad();
			
			// Find named children.
			//			
			m_componentEditBoxes[ 0 ] = getDescendantByName< UIEditBox >( "_hue_editbox", NameSearchPolicy::Substring );
			m_componentEditBoxes[ 1 ] = getDescendantByName< UIEditBox >( "_saturation_editbox", NameSearchPolicy::Substring );
			m_componentEditBoxes[ 2 ] = getDescendantByName< UIEditBox >( "_value_editbox", NameSearchPolicy::Substring );
			m_componentEditBoxes[ 3 ] = getDescendantByName< UIEditBox >( "_alpha_editbox", NameSearchPolicy::Substring );

			m_componentSliderKnobs[ 0 ] = getDescendantByName< UISliderKnob >( "_hue_knob_host_knob", NameSearchPolicy::Substring );
			m_componentSliderKnobs[ 1 ] = getDescendantByName< UISliderKnob >( "_saturation_knob_host_knob", NameSearchPolicy::Substring );
			m_componentSliderKnobs[ 2 ] = getDescendantByName< UISliderKnob >( "_value_knob_host_knob", NameSearchPolicy::Substring );
			m_componentSliderKnobs[ 3 ] = getDescendantByName< UISliderKnob >( "_alpha_slider_host_knob_host_knob", NameSearchPolicy::Substring );

			m_editBoxMainColor->addEventListener( UIEditBox::END_EDITING, FRESH_CALLBACK( onDoneEditingMain ));

			for( int i = 0; i < 4; ++i )
			{
				UIEditBox::ptr editBox = m_componentEditBoxes[ i ];
				ASSERT( editBox );
				
				editBox->addEventListener( UIEditBox::END_EDITING, FRESH_CALLBACK( onDoneEditingComponent ));

				UISliderKnob::ptr sliderKnob = m_componentSliderKnobs[ i ];
				ASSERT( sliderKnob );
				
				sliderKnob->addEventListener( DisplayObject::DRAG_MOVE, FRESH_CALLBACK( onSliderMovedComponent ));
				sliderKnob->addEventListener( DisplayObject::DRAG_END, FRESH_CALLBACK( onSliderEndComponent ));
			}
		}
		
	protected:
		
		virtual void updateFromDrag( const vec2& dragPositionLocal, bool ongoingElseFinal ) override
		{
			// No direct updating from drag.
		}
		
		virtual void onValueChanged( bool ongoingElseFinal ) override
		{
			Color color;
			color.fromHSVA( m_hsva );
			
			// Set the edit box value.
			//
			ASSERT( m_editBoxMainColor );
			m_editBoxMainColor->text( getValueString() );
			
			// Set the sample color.
			//
			m_sample->color( color );
			
			for( int i = 0; i < 4; ++i )
			{
				// Set the slider textbox values.
				//
				UIEditBox::ptr editBox = m_componentEditBoxes[ i ];
				ASSERT( editBox );
				
				const unsigned int componentValueForText = m_hsva[ i ] / ( i == 0 ? 360.0f : 1.0f ) * 255.0f;
				
				std::ostringstream componentSliderTextValue;
				componentSliderTextValue << componentValueForText;
				
				editBox->text( componentSliderTextValue.str() );
				
				// Set the slider values.
				//
				UISliderKnob::ptr sliderKnob = m_componentSliderKnobs[ i ];
				ASSERT( sliderKnob );
				
				sliderKnob->value( componentValueForText );
			}
		}
				
		virtual real enforcedHeight() const override
		{
			return m_height;
		}
		
		void setComponentValue( unsigned int componentValue, int iComponent, bool ongoingElseFinal )
		{
			REQUIRES( componentValue < 256 );
			REQUIRES( iComponent >= 0 && iComponent < 4 );
			
			m_hsva[ iComponent ] = componentValue / 255.0f * ( iComponent == 0 ? 360.0f : 1.0f );
			
			onValueChangedByControl( ongoingElseFinal );
		}

		int getComponentForSubControl( const DisplayObject& target ) const
		{
			// Was this hue, saturation, or value?
			//
			const auto& targetName = target.name();
			int iComponent = 0;
			if( targetName.find( "_hue" ) != std::string::npos )
			{
				iComponent = 0;
			}
			else if( targetName.find( "_saturation" ) != std::string::npos )
			{
				iComponent = 1;
			}
			else if( targetName.find( "_value" ) != std::string::npos )
			{
				iComponent = 2;
			}
			else
			{
				iComponent = 3;
			}
			
			return iComponent;
		}
		
	private:
		
		vec4 m_hsva;
		
		DVAR( real, m_height, 16.0f );
		VAR( UIEditBox::ptr, m_editBoxMainColor );
		VAR( DisplayObject::ptr, m_sample );
		
		// 4 for hue, saturation, value, alpha.
		//
		UIEditBox::ptr m_componentEditBoxes[ 4 ];
		UISliderKnob::ptr m_componentSliderKnobs[ 4 ];
		
		FRESH_DECLARE_CALLBACK( PropertyControlColor, onDoneEditingMain, Event );
		FRESH_DECLARE_CALLBACK( PropertyControlColor, onDoneEditingComponent, Event );
		FRESH_DECLARE_CALLBACK( PropertyControlColor, onSliderMovedComponent, Event );
		FRESH_DECLARE_CALLBACK( PropertyControlColor, onSliderEndComponent, Event );
	};
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( PropertyControlColor )
	DEFINE_VAR( PropertyControlColor, real, m_height );
	DEFINE_VAR( PropertyControlColor, UIEditBox::ptr, m_editBoxMainColor );
	DEFINE_VAR( PropertyControlColor, DisplayObject::ptr, m_sample );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( PropertyControlColor )
	
	FRESH_DEFINE_CALLBACK( PropertyControlColor, onDoneEditingMain, Event )
	{
		setValueString( m_editBoxMainColor->text() );
		onValueChangedByControl( false /* final */ );
	}

	FRESH_DEFINE_CALLBACK( PropertyControlColor, onDoneEditingComponent, Event )
	{
		auto target = dynamic_freshptr_cast< UIEditBox::ptr >( event.target() );
		ASSERT( target );
		
		//
		// Calculate the new color.
		//
		
		// What's the component value?
		//
		const auto& targetText = target->text();
		std::istringstream streamValue( targetText );

		// Hex?
		const size_t nHexCharacters = std::count_if( targetText.begin(), targetText.end(), [] ( char c )
											  {
												  return	( c >= 'A' && c <= 'F' ) ||
															( c >= 'a' && c <= 'f' );
											  } );
		
		unsigned int componentValue;
		
		if( nHexCharacters > 0 )
		{
			streamValue >> std::hex;
		}
		streamValue >> componentValue;
		
		if( streamValue.fail() || componentValue > 255 )
		{
			// Reset the value.
			//
			onValueChangedByControl( true /* ongoing */ );
		}
		else
		{
			const int iComponent = getComponentForSubControl( *target );
			setComponentValue( componentValue, iComponent, false /* final */ );
		}
	}
	
	FRESH_DEFINE_CALLBACK( PropertyControlColor, onSliderMovedComponent, Event )
	{
		auto target = dynamic_freshptr_cast< UISliderKnob::ptr >( event.currentTarget() );
		ASSERT( target );
		
		const auto componentValue = target->value();
		
		const int iComponent = getComponentForSubControl( *target );
		setComponentValue( static_cast< unsigned int >( componentValue ), iComponent, true /* ongoing */ );
	}
	
	FRESH_DEFINE_CALLBACK( PropertyControlColor, onSliderEndComponent, Event )
	{
		auto target = dynamic_freshptr_cast< UISliderKnob::ptr >( event.currentTarget() );
		ASSERT( target );
		
		const auto componentValue = target->value();
		
		const int iComponent = getComponentForSubControl( *target );
		setComponentValue( static_cast< unsigned int >( componentValue ), iComponent, false /* final */ );
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	
	class PropertyControlObject : public PropertyControlString
	{
		FRESH_DECLARE_CLASS( PropertyControlObject, PropertyControlString );
		
	public:
		
		const ClassInfo& selectableBaseClass() const
		{
			ClassInfo::cptr base = attachedProperty()->getReferencedClass();
			ASSERT( base );
			return *base;
		}
		
		virtual void postLoad()
		{
			Super::postLoad();
			
			if( m_buttonOpenBrowser )
			{
				m_buttonOpenBrowser->addEventListener( SimpleButton::TAPPED, FRESH_CALLBACK( onButtonOpenBrowser ));
			}
		}
		
	private:
		
		VAR( SimpleButton::ptr, m_buttonOpenBrowser );
		
		FRESH_DECLARE_CALLBACK( PropertyControlObject, onButtonOpenBrowser, Event );
	};
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( PropertyControlObject )
	DEFINE_VAR( PropertyControlObject, SimpleButton::ptr, m_buttonOpenBrowser );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( PropertyControlObject )
	
	FRESH_DEFINE_CALLBACK( PropertyControlObject, onButtonOpenBrowser, Event )
	{
		Editor& editor = *stage().as< Editor >();
		ASSERT( &editor );
		
		editor.objectBrowserSelectionDestination( editBox() );

		editor.openObjectBrowser( selectableBaseClass() );
	}
	
	/////////////////////////////////////////////////////////////////////////////////////////////
	
	class PropertyControlBool : public PropertyControl
	{
		FRESH_DECLARE_CLASS( PropertyControlBool, PropertyControl );
		
	public:
		
		virtual void setValueString( const std::string& strValue ) override
		{
			try
			{
				Destringifier destringifier( strValue );
				destringifier >> m_value;
				if( m_checkbox )
				{
					m_checkbox->checked( m_value );
				}
				m_hasValidValue = true;
			}
			catch( ... )
			{
				m_hasValidValue = false;
				m_checkbox->checked( false );
			}
			
			if( m_ambiguityMarker )
			{
				m_ambiguityMarker->visible( !m_hasValidValue );
			}

			onValueChanged( false /* final change */ );
		}
		
		virtual std::string getValueString() const override
		{
			return m_value ? "true" : "false";
		}
		
		virtual void postLoad()
		{
			Super::postLoad();
			
			if( m_checkbox )
			{
				m_checkbox->addEventListener( SimpleButton::TAPPED, FRESH_CALLBACK( onCheckboxChanged ));
			}
		}
		
	private:
		
		bool m_value = false;
		
		VAR( UICheckbox::ptr, m_checkbox );
		VAR( Sprite::ptr, m_ambiguityMarker );
		
		FRESH_DECLARE_CALLBACK( PropertyControlBool, onCheckboxChanged, Event );
	};
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( PropertyControlBool )
	DEFINE_VAR( PropertyControlBool, UICheckbox::ptr, m_checkbox );
	DEFINE_VAR( PropertyControlBool, Sprite::ptr, m_ambiguityMarker );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( PropertyControlBool )
	
	FRESH_DEFINE_CALLBACK( PropertyControlBool, onCheckboxChanged, Event )
	{
		m_value = m_checkbox->checked();
		m_hasValidValue = true;
		
		if( m_ambiguityMarker )
		{
			m_ambiguityMarker->visible( false );
		}
		
		onValueChangedByControl( false /* final */ );
	}

	///////////////////////////////////////////////////////////////////////////////////////////////
	// Class PropertyItem
	
	class PropertyItem : public Sprite
	{
	public:
		
		void control( PropertyControl::ptr control )
		{
			REQUIRES( control );
			
			if( m_control )
			{
				removeChild( m_control );
			}
			m_control = control;
			addChild( m_control );
		}
		
		PropertyControl::ptr control() const
		{
			return m_control;
		}
		
		void propertyName( const std::string& name )
		{
			ASSERT( m_itemName );
			m_itemName->text( name );
		}
		
		std::string propertyName() const
		{
			ASSERT( m_itemName );
			return m_itemName->text();
		}
		
		real getNameWidth() const
		{
			return m_itemName->localBounds().width();
		}
		
	private:
		
		VAR( TextField::ptr, m_itemName );
		
		PropertyControl::ptr m_control;

		FRESH_DECLARE_CLASS( PropertyItem, Sprite );
	};
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( PropertyItem )
	
	DEFINE_VAR( PropertyItem, TextField::ptr, m_itemName );
	
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( PropertyItem )
	
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	// Class PropertyPane
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( PropertyPane )
	
	const char* PropertyPane::SUBJECTS_CHANGED = "PropertyPaneSubjectsChanged";

	DEFINE_VAR( PropertyPane, ClassInfo::cptr, m_propertyItemClass );
	DEFINE_VAR( PropertyPane, MapControlTypesToClassNames, m_controlClasses );
	DEFINE_VAR( PropertyPane, real, m_itemPaddingVertical );
	DEFINE_VAR( PropertyPane, real, m_gutterWidth );
	DEFINE_VAR( PropertyPane, real, m_paneMaxWidth );
	DEFINE_VAR( PropertyPane, TextField::ptr, m_caption );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTOR_INERT( PropertyPane )
	
	PropertyPane::PropertyPane( const ClassInfo& assignedClassInfo, NameRef name )
	:	Super( assignedClassInfo, name )
	{
		m_itemHost = createObject< DisplayObjectContainer >( "propertyItemHost" );
		addChild( m_itemHost );
	}
	
	void PropertyPane::beginEditing( const Subjects& subjects )
	{
		TIMER_AUTO( PropertyPane::beginEditing )

		m_subjects = subjects;
		
		// Find the highest base class that is the base of all subjects.
		//
		m_commonBase = nullptr;
		
		for( auto subject : m_subjects )
		{
			const ClassInfo& subjectClass = subject->classInfo();
			
			if( !m_commonBase )
			{
				m_commonBase = &subjectClass;
			}
			else
			{
				m_commonBase = &m_commonBase->getCommonBase( subjectClass );
			}
		}
		
		ASSERT( m_commonBase || m_subjects.empty() );
		
		setupPane();
	}
	
	void PropertyPane::clearPane()
	{
		m_itemHost->removeChildren();
	}
	
	void PropertyPane::setupPane()
	{
		TIMER_AUTO( PropertyPane::setupPane )

		ASSERT( m_itemHost );
		
		// Show object name(s).
		//
		if( m_caption )
		{
			TIMER_AUTO( setup caption )

			if( !m_caption->parent() )
			{
				addChild( m_caption );
			}
			m_caption->enforcedBounds( vec2( m_paneMaxWidth, 1.0f ));
			
			// Determine the caption text.
			//
			std::ostringstream out;
			
			if( m_commonBase )
			{
				out << m_commonBase->className() << ": ";
				
				bool first = true;
				for( auto subject : m_subjects )
				{
					if( !first ) out << ", ";
					out << subject->name();

					first = false;
				}
			}
			else
			{
				out << "(No Selection)";
			}
			m_caption->text( out.str() );
			
			m_itemHost->position( 0, m_caption->position().y + m_caption->localBounds().height() );
		}
		else
		{
			m_itemHost->position( vec2::ZERO );
		}
		
		{
			TIMER_AUTO( create controls )

			// Hide all the property items in the item host.
			//
			for( auto item : *m_itemHost )
			{
				item->visible( false );
			}
			
			// Find or create controls for each property.
			//
			real maxPropertyNameWidth = 0;
		
			PropertyItem::ptr lastPropertyItem = nullptr;
		
			forEachProperty(
				[&]( const PropertyAbstract& property, size_t iProperty )
				{
	//				dev_trace( "setting up property " << property.propName() << ( property.isEditable() ? "" : " (NO EDIT)" ) );
				
					// Do we already have a property item for this property?
					//
					PropertyItem::ptr propertyItem = itemForProperty( property );
					
					if( !propertyItem )
					{
						// No we do not. Create a new property item.
						//
						if( m_propertyItemClass )
						{
							propertyItem = createObject< PropertyItem >( *m_propertyItemClass );
						}
					
						if( !propertyItem )
						{
							propertyItem = createObject< PropertyItem >();
						}

						m_itemHost->addChildAt( propertyItem, iProperty );
						
						setupPropertyItem( *propertyItem, property );
					}
					else
					{
						if( iProperty < m_itemHost->numChildren() )
						{
							m_itemHost->swapChildren( propertyItem, m_itemHost->getChildAt( iProperty ));
						}
					}
					
					propertyItem->visible( true );
					
					// Position the item.
					//
					vec2 pos;
					if( lastPropertyItem )
					{
						pos.y = lastPropertyItem->position().y + lastPropertyItem->control()->enforcedHeight() + m_itemPaddingVertical;						
					}
					propertyItem->position( pos );

					maxPropertyNameWidth = std::max( maxPropertyNameWidth, propertyItem->getNameWidth());
				
					lastPropertyItem = propertyItem;
				}
			);
		
			// Go through and reposition all property controls based on the name widths.
			//
			adjustItemWidths( maxPropertyNameWidth + m_gutterWidth, std::max( 1.0f, m_paneMaxWidth - ( maxPropertyNameWidth + m_gutterWidth ) ));
		}

		updatePane();
	}

	void PropertyPane::setupPropertyItem( PropertyItem& propertyItem, const PropertyAbstract& property ) const
	{
		TIMER_AUTO( PropertyPane::setupPropertyItem )

		propertyItem.propertyName( property.propName() );
		
		// Create the proper control for the property type.
		//
		const PropertyAbstract::ControlType controlType = property.getControlType();
		
		PropertyControl::ptr control = createControl( controlType );
		ASSERT( control );
		control->attachToProperty( &property );
		propertyItem.control( control );
	}
	
	void PropertyPane::forEachProperty( const std::function< void ( const PropertyAbstract&, size_t ) >& fnPerProperty )
	{
		if( m_commonBase )
		{
			// Get all properties into alphabetical order.
			//
			std::vector< const PropertyAbstract* > properties;
			
			auto end = m_commonBase->getPropertyIteratorEnd();
			for( auto begin = m_commonBase->getPropertyIteratorBegin(); begin != end; ++begin )
			{
				if( begin->isEditable() )
				{
					properties.push_back( &*begin );
				}
			}
			
			std::sort( properties.begin(), properties.end(), []( const PropertyAbstract* a, const PropertyAbstract* b )
					  {
						  return a->propName() < b->propName();
					  } );
			
			for( size_t iProperty = 0; iProperty < properties.size(); ++iProperty )
			{
				fnPerProperty( *properties[ iProperty ], iProperty );
			}
		}
	}
	
	void PropertyPane::adjustItemWidths( real controlOffsetsX, real controlMaxWidth )
	{
		TIMER_AUTO( PropertyPane::adjustItemWidths )
		
		forEachProperty(
			[&] ( const PropertyAbstract& property, size_t iProperty )
			{
				PropertyItem::ptr item = dynamic_freshptr_cast< PropertyItem::ptr >( m_itemHost->getChildAt( iProperty ));
				ASSERT( item );
				
				if( item->visible() )
				{
					item->control()->position( controlOffsetsX, 0 );
					item->control()->maxWidth( controlMaxWidth );
				}
			}
		);
	}
	
	void PropertyPane::updatePane()
	{
		TIMER_AUTO( PropertyPane::updatePane )

		forEachProperty(
			[&] ( const PropertyAbstract& property, size_t iProperty )
			{
				updatePropertyFromSubjects( property, iProperty );
			}
		);
	}
	
	void PropertyPane::updatePropertyFromSubjects( const PropertyAbstract& property, size_t iProperty )
	{
		bool isValueInitialized = false;
		std::string strPropertyValue;

		for( auto subject : m_subjects )
		{
			std::string thisSubjectPropertyValue = property.getValueByString( subject.get() );
			
			if( !isValueInitialized )
			{
				isValueInitialized = true;
				strPropertyValue = thisSubjectPropertyValue;
			}
			else if( strPropertyValue != thisSubjectPropertyValue )
			{
				strPropertyValue = MULTIPLE_VALUES_STRING;
				break;
			}
		}
		
		// Write out the property value.
		//
		PropertyItem::ptr item = dynamic_freshptr_cast< PropertyItem::ptr >( m_itemHost->getChildAt( iProperty ));
		ASSERT( item );
		PropertyControl::ptr control = item->control();
		ASSERT( control );
		
		control->setValueString( strPropertyValue );
	}
	
	void PropertyPane::updateSubjectsProperty( const PropertyAbstract& property, const std::string& value, bool ongoingElseFinal )
	{
		Editor& editor = *stage().as< Editor >();
		ASSERT( &editor );
		
		pushActivePackage( &editor.editedPackage() );		// Push the active package so that objects may be found.
		for( auto subject : m_subjects )
		{
			subject->setPropertyValue( property.propName(), value );
		}
		popActivePackage();
		
		if( !ongoingElseFinal )
		{
			// Since the act of calling property.setValueByString() may very well have modified the string values,
			// or indeed normalized the textual version of the value, send the value back up to the
			// controls.
			//
			updatePane();
			
			// Report this event.
			//
			Event e( SUBJECTS_CHANGED, this );
			dispatchEvent( &e );
		}
	}
	
	SmartPtr< PropertyItem > PropertyPane::itemForProperty( const PropertyAbstract& property ) const
	{
		ASSERT( m_itemHost );
		
		for( size_t i = 0; i < m_itemHost->numChildren(); ++i )
		{
			PropertyItem::ptr item = dynamic_freshptr_cast< PropertyItem::ptr >( m_itemHost->getChildAt( i ));
			
			if( item && item->propertyName() == property.propName() )
			{
				return item;
			}
		}
		return nullptr;
	}
	
	SmartPtr< PropertyControl > PropertyPane::createControl( PropertyAbstract::ControlType controlType ) const
	{
		TIMER_AUTO( PropertyPane::createControl )

		TIMER_BEGIN( find control class )

		PropertyControl::ptr control;
		
		auto iter = m_controlClasses.find( controlType );
		
		if( iter == m_controlClasses.end() )
		{
			//			dev_warning( this << " could not find control class for type " << controlType );
			iter = m_controlClasses.begin();
		}
		ASSERT( iter != m_controlClasses.end() );
		
		TIMER_END( find control class )
		
//		dev_trace( "Creating control of class " << iter->second << " for type " << controlType );
		
		TIMER_BEGIN( create object )

		control = createObject< PropertyControl >( *getClass( iter->second ), DEFAULT_OBJECT_NAME );

		TIMER_END( create object )

		control->addEventListener( PropertyControl::PROPERTY_CONTROL_CHANGING, FRESH_CALLBACK( onControlChanging ) );
		control->addEventListener( PropertyControl::PROPERTY_CONTROL_FINISHED_CHANGE, FRESH_CALLBACK( onControlFinishedChange ) );
		
		PROMISES( control );
		return control;
	}
								   
	FRESH_DEFINE_CALLBACK( PropertyPane, onControlChanging, PropertyControl::Changed )
	{
		updateSubjectsProperty( *event.control().attachedProperty(), event.newValue(), true /* ongoing */ );
	}
	
	FRESH_DEFINE_CALLBACK( PropertyPane, onControlFinishedChange, PropertyControl::Changed )
	{
		updateSubjectsProperty( *event.control().attachedProperty(), event.newValue(), false /* final */ );
	}	
}

