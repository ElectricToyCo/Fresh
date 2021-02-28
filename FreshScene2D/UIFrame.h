//
//  UIFrame.h
//  Fresh
//
//  Created by Jeff Wofford on 2/13/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_UIFrame_h
#define Fresh_UIFrame_h

#include "Sprite.h"

namespace fr
{
	/*	
	 
	 A UIFrame helps to automate the creation of "frames"
	 that surround and border a set of display objects.
	 
	 A frame uses a special texture that is divided into
	 9 sections, and transforms its mesh vertices in order
	 to maintain the correct inner dimensions without stretching
	 its border art.
	 
	 A frame's inner dimensions may be set directly from its
	 children or by setting a specific value. When resizing
	 with a specific value, the frame places the upper left
	 corner of its inner area at its own origin. When resizing
	 to fit its children, the frame places its mesh points wherever
	 necessary to encompass those children.
	 
	*/
	
	class UIFrame : public DisplayObjectWithMesh
	{
	public:

		rect frame() const;
		// PROMISES( result.isWellFormed() );
		virtual void frame( const rect& r ) override;

		virtual void reshape( const vec2& innerDimensions );
		virtual void reshape( const rect& innerArea );
		virtual void reshapeToEncompassChildren();
		virtual void reshapeToEncompassSiblings();

		virtual void update() override;
		
	protected:
		
		virtual void onAddedToStage() override;
		
	private:
		
		DVAR( bool, m_creationReshapeToEncompassSiblings, false );						// If true, overrides m_frame.
		DVAR( bool, m_creationReshapeToEncompassChildren, false );						// If true, overrides other creation reshape directives.
		DVAR( rect, m_pixelCoordinatesInnerArea, rect( 16.0f, 16.0f, 48.0f, 48.0f ));	// These default values would work well for a 64x64 texture with 16 pixel-wide borders surrounding a 32x32 inner area.
		
		rect m_lastReshapeFrame;
		
		FRESH_DECLARE_CLASS( UIFrame, DisplayObjectWithMesh );
	};
	
}

#endif
