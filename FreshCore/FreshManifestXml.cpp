//
//  FreshManifestXml.cpp
//  Fresh
//
//  Created by Jeff Wofford on 4/14/15.
//
//

#include "FreshManifest.h"
#include "FreshException.h"
#include "Object.h"

namespace
{
	using namespace fr;
	
	std::string nodeValue( const XmlElement& element )
	{
		return element.ValueStr();
	}
	
	std::string nodeText( const XmlElement& element )
	{
		const char* text = element.GetText();
		return text ? text : "";
	}
	
	std::string attributeValue( const XmlElement& element, const std::string& attributeName )
	{
		const char* attribName  = element.Attribute( attributeName.c_str() );
		return attribName ? attribName : "";
	}
	
	template< typename parent_t, typename child_t, typename iter_t >
	struct NodeIterator
	{
		explicit NodeIterator( const parent_t& node );
		
		NodeIterator& operator++();
		
		explicit operator bool() const;
		
		const child_t& operator*() const;
		
	private:
		
		iter_t m_member;
	};
	
	// XML NodeIterator
	
	template<>
	NodeIterator< XmlElement, XmlElement, ElementIterator >::NodeIterator( const XmlElement& node )
	:	m_member( node )
	{}
	
	template<>
	NodeIterator< XmlElement, XmlElement, ElementIterator >& NodeIterator< XmlElement, XmlElement, ElementIterator >::operator++()
	{
		++m_member;
		return *this;
	}
	
	template<>
	NodeIterator< XmlElement, XmlElement, ElementIterator >::operator bool() const
	{
		return m_member != ElementIterator{};
	}
	
	template<>
	const XmlElement& NodeIterator< XmlElement, XmlElement, ElementIterator >::operator*() const
	{
		return *m_member;
	}
	
	// getIterator functions
	
	NodeIterator< XmlElement, XmlElement, ElementIterator > getIterator( const XmlElement& collection )
	{
		return NodeIterator< XmlElement, XmlElement, ElementIterator >{ collection };
	}
	
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	std::shared_ptr< Manifest::Map > readProperties( const XmlElement& element );
	
	std::shared_ptr< Manifest::Object > makeObjectFromElement( const XmlElement& element )
	{
		const std::string name = attributeValue( element, "name" );
		const std::string className = attributeValue( element, "class" );
		
		auto properties = readProperties( element );
		
		return std::make_shared< Manifest::Object >(
			name,
			className,
			std::move( properties ) );
	}
	
	std::shared_ptr< Manifest::Value > parseObjects( const XmlElement& parent )
	{
		std::vector< const XmlElement* > childObjectElements;
		
		// Build the children into a vector.
		//
		for( auto child = parent.FirstChildElement( "object" ); child; child = child->NextSiblingElement( "object" ))
		{
			childObjectElements.push_back( child );
		}
		
		// Now check whether this "vector" is really just one element or multiple.
		//
		ASSERT( childObjectElements.size() > 0 );	// Don't call this function unless the parent has object children.
		if( childObjectElements.size() == 1 )
		{
			// A single object value. Make a Manifest::Object;
			//
			auto object = makeObjectFromElement( *childObjectElements.front() );
			return std::make_shared< Manifest::Value >( std::move( object ));
		}
		else
		{
			// An array of objects. Sweet.
			//
			std::shared_ptr< Manifest::Array > array{ new Manifest::Array{} };
			
			for( const auto& child : childObjectElements )
			{
				auto object = makeObjectFromElement( *child );
				auto objectValue = std::make_shared< Manifest::Value >( std::move( object ));
				array->push_back( std::move( objectValue ));
			}
			
			return std::make_shared< Manifest::Value >( std::move( array ));
		}
	}
	
	std::shared_ptr< Manifest::Array > readKeyframes( const XmlElement& passthroughElement )
	{
		std::shared_ptr< Manifest::Array > keyframes;
		
		for( auto iterKeyframe = getIterator( passthroughElement ); iterKeyframe; ++iterKeyframe )
		{
			const XmlElement& keyframeElement = *iterKeyframe;
			const std::string& elementName = nodeValue( keyframeElement );
			
			if( elementName == "keyframe" )
			{
				auto keyframeObject = std::make_shared< Manifest::Object >( "", "keyframe", std::make_shared< Manifest::Map >() );
				
				auto& keyframeProperties = *keyframeObject->map;

				const auto echoProperty = [&]( const char* propertyName )
				{
					if( const XmlElement* propertyElement = keyframeElement.FirstChildElement( propertyName ))
					{
						keyframeProperties[ propertyName ] = std::make_pair( std::make_shared< Manifest::Value >( propertyElement->GetText() ),
																			Manifest::PropertyAttributes{}
																			);
					}
				};

				echoProperty( "texture" );
				echoProperty( "textureWindow" );
				echoProperty( "position" );
				echoProperty( "pivot" );
				
				// What about child elements?
				//
				auto children = std::make_shared< Manifest::Array >();
				
				for( const XmlElement* childElement = keyframeElement.FirstChildElement( "child" );
					childElement;
					childElement = childElement->NextSiblingElement( "child" ))
				{
					const char* szChildName = childElement->Attribute( "name" );
					
					if( !szChildName )
					{
						dev_warning( "While loading Manifest from XML, found a keyframe with a nameless child. Ignoring this child." );
					}
					else
					{
						auto child = std::make_shared< Manifest::Object >( szChildName, "child", std::make_shared< Manifest::Map >() );
						auto& childProperties = *child->map;
						
						const auto echoChildProperty = [&]( const char* propertyName )
						{
							if( const XmlElement* propertyElement = childElement->FirstChildElement( propertyName ))
							{
								childProperties[ propertyName ] = std::make_pair( std::make_shared< Manifest::Value >( propertyElement->GetText() ),
																				 Manifest::PropertyAttributes{}
																				 );
							}
						};
						
						echoChildProperty( "position" );
						echoChildProperty( "scale" );
						echoChildProperty( "rotation" );
						echoChildProperty( "color" );
						echoChildProperty( "colorAdditive" );
						echoChildProperty( "pivot" );
						echoChildProperty( "frame" );
						
						children->push_back( std::make_shared< Manifest::Value >( child ));
					}
				}
				
				if( !children->empty() )
				{
					keyframeProperties[ "children" ] = std::make_pair( std::make_shared< Manifest::Value >( std::move( children )),
																			Manifest::PropertyAttributes{} );
				}

				children.reset();
				
				// Get the label and/or frame index (t or s).
				//
				const auto echoAttributeProperty = [&]( const char* attributeName, const char* newAttributeName = nullptr )
				{
					ASSERT( attributeName );
					if( const char* attributeValue = keyframeElement.Attribute( attributeName ))
					{
						if( !newAttributeName )
						{
							newAttributeName = attributeName;
						}
						
						keyframeProperties[ newAttributeName ] = std::make_pair( std::make_shared< Manifest::Value >( std::string{ attributeValue }),
																				Manifest::PropertyAttributes{} );
					}
				};
				
				echoAttributeProperty( "t" );
				echoAttributeProperty( "s" );
				echoAttributeProperty( "rel-t", "rel_t" );
				echoAttributeProperty( "rel-s", "rel_s" );
				
				echoAttributeProperty( "label" );
				echoAttributeProperty( "replay-children", "replay_children" );

				echoProperty( "action" );

				// Do we like a tween with this keyframe?
				//
				if( const XmlElement* tweenElement = keyframeElement.FirstChildElement( "tween" ) )
				{
					// Yes. What kind?
					const char* szTweenType = tweenElement->Attribute( "type" );
					szTweenType = szTweenType ? szTweenType : "QuadEaseInOut";

					keyframeProperties[ "tween" ] =
						std::make_pair( std::make_shared< Manifest::Value >(
															std::make_shared< Manifest::Object >( "",
																								 szTweenType,
																								 nullptr )),
									   Manifest::PropertyAttributes{} );
				}
				
				if( !keyframes )
				{
					keyframes = std::make_shared< Manifest::Array >();
				}
				
				keyframes->push_back( std::make_shared< Manifest::Value >( std::move( keyframeObject )));
			}
			else
			{
				dev_warning( "Passthrough element with <keyframe> subelements had non-keyframe element '" << elementName << "'. Ignoring." );
			}
		}
		
		return keyframes;
	}
	
	std::shared_ptr< Manifest::Map > readProperties( const XmlElement& element )
	{
		std::shared_ptr< Manifest::Map > properties = std::make_shared< Manifest::Map >();
		
		for( auto iterProperty = getIterator( element ); iterProperty; ++iterProperty )
		{
			const XmlElement& propertyElement = *iterProperty;
			const std::string& propertyName = nodeValue( propertyElement );
			std::string propertyValue = nodeText( propertyElement );
			
			// Parse propertyValue to map, array, or object if possible.
			//
			std::shared_ptr< Manifest::Value > value;
			
			// Is this the special passthrough element?
			//
			if( propertyName == "passthrough" )
			{
				// Handle special class-specific cases.
				//
				if( propertyElement.FirstChildElement( "keyframe" ))
				{
					// MovieClip (we hope) keyframes.
					auto keyframeProperties = readKeyframes( propertyElement );
					value = std::make_shared< Manifest::Value >( std::move( keyframeProperties ));
				}
				else
				{
					// Parse it as a map of sub-elements.
					//
					auto passthroughProperties = readProperties( propertyElement );
					value = std::make_shared< Manifest::Value >( std::move( passthroughProperties ));
				}
			}
			else
			{
				// Does this element contain sub object elements?
				//
				if( propertyElement.FirstChildElement( "object" ))
				{
					value = parseObjects( propertyElement );
				}
				else
				{
					// Guess it's a string.
					value = std::make_shared< Manifest::Value >( std::move( propertyValue ));
				}
			}
			
			// Gather property attributes.
			//
			Manifest::PropertyAttributes attributes;
			for( auto attribute = propertyElement.FirstAttribute(); attribute; attribute = attribute->Next() )
			{
				// Property attributes are expected to be boolean.
				//
				if( attribute->ValueStr() == "true" )
				{
					attributes.push_back( attribute->Name() );
				}
			}
			
			(*properties)[ propertyName ] = std::make_pair( value, attributes );
		}
		
		return properties;
	}
}

namespace fr
{
	void Manifest::load( const XmlElement& rootElement, Directives& directives )
	{
		for( auto iter = getIterator( rootElement ); iter; ++iter )
		{
			const auto& child = *iter;
			
			const auto nodeType = nodeValue( child );
			
			if( nodeType == "object" )
			{
				const char* sz = child.Attribute( "class" );
				
				if( !sz )
				{
					FRESH_THROW( FreshException, "Object element lacked class attribute." );
				}
				else
				{
					const std::string className = sz;
					
					sz = child.Attribute( "name" );
					
					std::string objectName;
					if( sz )
					{
						objectName = parseObjectName( sz );
					}
					else
					{
						objectName = DEFAULT_OBJECT_NAME;
					}

					auto objectProperties = readProperties( child );
					
					directives.emplace_back( new Manifest::Object( objectName,
																  className,
																  std::move( objectProperties )));
				}
			}
			else if( nodeType == "class" )
			{
				// Add a pseudoclass.
				//
				const auto className = attributeValue( child, "name" );
				const auto baseClassName = attributeValue( child, "extends" );

				std::shared_ptr< Manifest::Map > properties = std::make_shared< Manifest::Map >();
				
				auto classProperties = readProperties( child );

				directives.emplace_back( new Manifest::Class( className,
															 std::vector< std::string >{ baseClassName },
															 std::move( classProperties )));
			}
			else if( nodeType == "const" )
			{
				const auto name = attributeValue( child, "name" );
				const auto type = attributeValue( child, "type" );
				
				directives.emplace_back( new Manifest::Const( name, type, nodeText( child )));
			}
			else
			{
				dev_warning( "Manifest XML file had unrecognized node type '" << nodeType << "'. Ignoring." );
			}
		}
	}
}
