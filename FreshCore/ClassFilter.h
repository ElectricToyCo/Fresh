//
//  ClassFilter.h
//  Fresh
//
//  Created by Jeff Wofford on 10/28/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_ClassFilter_h
#define Fresh_ClassFilter_h

#include "Object.h"
#include "Property.h"
#include "ClassInfo.h"

namespace fr
{
	
	class ClassFilter : public Object
	{
		FRESH_DECLARE_CLASS( ClassFilter, Object );
	public:
		
		bool includes( const ClassInfo& classInfo ) const;

	protected:
		
		void fixupRemainingNames();
		
	private:
		
		VAR( std::vector< ClassInfo::cptr >, m_included );
		VAR( std::vector< ClassInfo::cptr >, m_excluded );
		VAR( std::vector< ClassName >, m_includedNames );		// Supports lazy-fixup of names, for example when the filter is loaded before the referenced class is.
		VAR( std::vector< ClassName >, m_excludedNames );
		
	};
	
}

#endif
