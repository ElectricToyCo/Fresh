/*
 *  ShaderUniformUpdaterConcrete.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 12/22/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_SHADER_UNIFORM_UPDATER_CONCRETE_H_INCLUDED
#define FRESH_SHADER_UNIFORM_UPDATER_CONCRETE_H_INCLUDED

#include "ShaderUniformUpdater.h"
#include "ShaderProgram.h"
#include "ObjectMethod.h"
#include "CommandProcessor.h"

namespace fr
{
	template< typename PropValueType, typename RefT = PropValueType&, typename ConstRefT = const PropValueType& >
	class ShaderUniformUpdaterConcrete : public ShaderUniformUpdater
	{
	public:
		
		virtual void updateUniform( Object::cptr host = nullptr ) override
		{
			REQUIRES( isBoundToProgram() );
			
			Object::cptr bindingHost = m_boundObject;
			if( !bindingHost )
			{
				bindingHost = host;
			}
			
			// Attempt late binding.
			//
			if( m_uniformId < 0 )
			{
				m_uniformId = m_shaderProgram->getUniformId( m_uniformName );
			}
			
			// Did binding fail?
			if( m_uniformId >= 0 )
			{
				ASSERT( bindingHost );
				const ClassInfo& classInfo = bindingHost->classInfo();
				
				const auto& memberName = !m_objectMemberName.empty() ? m_objectMemberName : m_uniformName;
				
				// Cache object access data. Believe it or not these few lines are major performance bottlenecks if left uncached.
				//
				if( m_isObjectMemberProperty )
				{
					if( !m_property || ( m_lastBindingHostClass != &classInfo ))
					{
						m_property = dynamic_cast< const Property< PropValueType >* >( classInfo.getPropertyByName( memberName ));
					}
				}
				else if( !m_accessorSpecific || ( m_lastBindingHostClass != &classInfo ))
				{
					// Adapted from callSimpleAccessor< ConstRefT >( bindingHost, memberName ), for efficiency.
					//
					const SimpleAccessorAbstract* accessor = classInfo.getAccessorByName( memberName );
					if( !accessor )
					{
						con_error( " could not find accessor '" << memberName << "' on binding host " << bindingHost );
					}
					else
					{
						m_accessorSpecific = dynamic_cast< AccessorType* >( accessor );
					}
				}

				m_lastBindingHostClass = &( bindingHost->classInfo());

				// Get the current value.
				//
				PropValueType currentValue;
				
				if( m_isObjectMemberProperty )
				{			
					if( !m_property )
					{
						con_error( this << " could not find property '" << memberName << "' on binding host " << bindingHost );
						return;
					}
					else
					{
						currentValue = m_property->getValue( bindingHost.get() );
					}
				}
				else
				{
					if( !m_accessorSpecific )
					{
						con_error( this << " found accessor '" << memberName << "' but it returns the incorrect type." );
						return;
					}
					else
					{
						currentValue = (*m_accessorSpecific)( bindingHost.get() );
					}
				}

				// Upload the new value, if it's actually new.
				//
				if( !m_hasBeenAssignedAtLeastOnce || ( currentValue != m_lastAssignedValue ))
				{
					m_shaderProgram->setUniform( m_uniformId, currentValue );
					m_lastAssignedValue = currentValue;
					m_hasBeenAssignedAtLeastOnce = true;
				}
			}
		}
		
	protected:
		
		virtual void clearCachedValues() override
		{
			m_hasBeenAssignedAtLeastOnce = false;
			m_property = nullptr;
			m_accessorSpecific = nullptr;
		}
		
	private:
		
		typedef const SimpleAccessor< ConstRefT > AccessorType;
		
		bool m_hasBeenAssignedAtLeastOnce = false;
		PropValueType m_lastAssignedValue;
		ClassInfo::cptr m_lastBindingHostClass = nullptr;
		
		// Cached values for performance.
		//
		const Property< PropValueType >* m_property = nullptr;
		AccessorType* m_accessorSpecific = nullptr;
		
		FRESH_DECLARE_CLASS( ShaderUniformUpdaterConcrete, ShaderUniformUpdater )
	};		

	template< typename PropValueType, typename RefT, typename ConstRefT >
	inline ShaderUniformUpdaterConcrete< PropValueType, RefT, ConstRefT >::ShaderUniformUpdaterConcrete( CreateInertObject c )
	:	Super( c )
	{}

	template< typename PropValueType, typename RefT, typename ConstRefT >
	inline ShaderUniformUpdaterConcrete< PropValueType, RefT, ConstRefT >::ShaderUniformUpdaterConcrete( const ClassInfo& assignedClassInfo, NameRef objectName )
	:	Super( assignedClassInfo, objectName )
	{}
	

}

#endif
