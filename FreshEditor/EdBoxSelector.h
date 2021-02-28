//
//  EdBoxSelector.h
//  Fresh
//
//  Created by Jeff Wofford on 6/3/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_EdBoxSelector_h
#define Fresh_EdBoxSelector_h

#include "Sprite.h"

namespace fr
{
	
	class EdBoxSelector : public Sprite
	{
		FRESH_DECLARE_CLASS( EdBoxSelector, Sprite );
	public:
		
		typedef std::set< DisplayObject::wptr > SelectedObjects;

		SYNTHESIZE_GET( vec2, startPoint )
		void startPoint( const vec2& p );
		SYNTHESIZE_GET( vec2, endPoint )
		void endPoint( const vec2& p );

		rect selectionRectangle() const;
		
	protected:
		
		virtual void onPointChanged();
		
	private:
		
		vec2 m_startPoint;
		vec2 m_endPoint;

		SelectedObjects m_selectedObjects;
	};
	
}

#endif
