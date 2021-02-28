//
//  TextGroup.h
//  Fresh
//
//  Created by Jeff Wofford on 8/16/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_TextGroup_h
#define Fresh_TextGroup_h

#include "DisplayObjectContainer.h"

namespace fr
{
	
	class TextGroup : public fr::DisplayObjectContainer
	{
		FRESH_DECLARE_CLASS( TextGroup, DisplayObjectContainer );
	public:
		
		void text( const std::string& text_ );
		SYNTHESIZE_GET( std::string, text );

		virtual void update() override;
		
	protected:
		
		void propagateText();
		
	private:
		
		VAR( std::string, m_text );
	};
	
}

#endif
