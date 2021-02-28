//
//  UIDisplayGrid.h
//  Fresh
//
//  Created by Jeff Wofford on 6/8/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_UIDisplayGrid_h
#define Fresh_UIDisplayGrid_h

#include "DisplayObjectContainer.h"
#include "TextMetrics.h"

namespace fr
{
	
	// Arranges its children into rows and columns so that they fit
	// within the object's frame.
	// Each child is rescaled to fit within its cell.
	
	class UIDisplayGrid : public DisplayObjectContainer
	{
		FRESH_DECLARE_CLASS( UIDisplayGrid, DisplayObjectContainer );
	public:
		
		SYNTHESIZE_GET( vec2, cellSize );

		vec2 effectiveCellSize() const;
		vec2 effectiveCellSpacing() const;
		
		virtual void arrangeChildren();
				
		virtual void update() override;
		virtual void onAddedToStage() override;

	protected:
		
		virtual bool isDirtyArrangement() const;
		
	private:
		
		DVAR( vec2, m_cellSize, vec2( 100, 100 ));
		DVAR( bool, m_allowScaleUpToFit, true );
		DVAR( bool, m_allowScaleDownToFit, true );
		DVAR( vec2, m_padding, vec2( 5, 5 ));		// The width of the gutter between cells.
		DVAR( bool, m_wrapRows, true );				// Else wraps columns and extends horizontally.
		DVAR( TextMetrics::Alignment, m_alignment, TextMetrics::Alignment::Centered );
		DVAR( TextMetrics::VerticalAlignment, m_verticalAlignment, TextMetrics::VerticalAlignment::Middle );
		
		rect m_lastArrangementFrame;
		
	};
	
}

#endif
