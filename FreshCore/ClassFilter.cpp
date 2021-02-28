//
//  ClassFilter.cpp
//  Fresh
//
//  Created by Jeff Wofford on 10/28/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "ClassFilter.h"

namespace
{
	using namespace fr;
	
	int minDepthToPossibleAncestors( const std::vector< ClassInfo::cptr >& possibleAncestors, const ClassInfo& classInfo )
	{
		int minDepth = -1;
		for( auto possibleAncestor : possibleAncestors )
		{
			if( possibleAncestor )		// Ignore null entries.
			{
				int depth = classInfo.getSuperClassDepth( *possibleAncestor );
				if( depth >= 0 )
				{
					if( minDepth < 0 )
					{
						minDepth = depth;
					}
					else
					{
						minDepth = std::min( minDepth, depth );
					}
				}
			}
		}
		return minDepth;
	}
}

namespace fr
{	
	FRESH_DEFINE_CLASS( ClassFilter )
	DEFINE_VAR( ClassFilter, std::vector< ClassInfo::cptr >, m_included );
	DEFINE_VAR( ClassFilter, std::vector< ClassInfo::cptr >, m_excluded );
	DEFINE_VAR( ClassFilter, std::vector< ClassName >, m_includedNames );
	DEFINE_VAR( ClassFilter, std::vector< ClassName >, m_excludedNames );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( ClassFilter )
	
	bool ClassFilter::includes( const ClassInfo& classInfo ) const
	{
		// A little harmless (hopefully--hard to image why not) cheating.
		//
		const_cast< ClassFilter* >( this )->fixupRemainingNames();
		
		// Measure the shortest inheritance depths to the included and excluded classes, respectively.
		//
		int minIncludedDepth = minDepthToPossibleAncestors( m_included, classInfo );
		int minExcludedDepth = minDepthToPossibleAncestors( m_excluded, classInfo );
		
		// The class is included if it is (1) included at all and (2) it is closer in derivation to an included class than to an excluded one.
		return minIncludedDepth >= 0 && ( minExcludedDepth < 0 || minIncludedDepth < minExcludedDepth );
	}
	
	void ClassFilter::fixupRemainingNames()
	{
		auto fixup = []( std::vector< ClassName >& names, std::vector< ClassInfo::cptr >& classes ) -> void
		{
			for( auto iter = names.begin(); iter != names.end(); )
			{
				const auto& className = *iter;
				
				auto classInfo = getClass( className );
				if( classInfo )
				{
					if( classes.end() == std::find( classes.begin(), classes.end(), classInfo ))
					{
						classes.push_back( classInfo );
						iter = names.erase( iter );
						continue;
					}
				}
				
				++iter;
			}
		};
		
		fixup( m_includedNames, m_included );
		fixup( m_excludedNames, m_excluded );
	}
}

