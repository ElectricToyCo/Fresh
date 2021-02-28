//
//  DisplayPackage.h
//  Fresh
//
//  Created by Jeff Wofford on 3/4/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_DisplayPackage_h
#define Fresh_DisplayPackage_h

#include "Package.h"

namespace fr
{
	
	class DisplayObject;
	
	class DisplayPackage : public Package
	{
		FRESH_DECLARE_CLASS( DisplayPackage, Package )
		
	public:
		
		SYNTHESIZE_GET( SmartPtr< DisplayObject >, root );
		void root( SmartPtr< DisplayObject > root );
		
		virtual std::vector< Object::ptr > loadFromManifest( const Manifest& manifest ) override;
		
	protected:
		
		virtual void writeRootElement( std::ostream& out ) const override;

	private:
		
		SmartPtr< DisplayObject > m_root;		
	};
	
}

#endif
