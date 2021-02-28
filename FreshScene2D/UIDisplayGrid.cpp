//
//  UIDisplayGrid.cpp
//  Fresh
//
//  Created by Jeff Wofford on 6/8/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "UIDisplayGrid.h"

namespace
{
	using namespace fr;
	
	vec2 largestChildDimensions( const DisplayObjectContainer& container )
	{
		vec2 result;
		container.forEachChild< DisplayObject >( [&]( const DisplayObject& child )
							   {
								   const auto bounds = child.localBounds();
								   result.x = std::max( bounds.width(), result.x );
								   result.y = std::max( bounds.height(), result.y );
							   } );
		
		return result;
	}
}

namespace fr
{	
	FRESH_DEFINE_CLASS( UIDisplayGrid )
	DEFINE_VAR( UIDisplayGrid, vec2, m_cellSize );
	DEFINE_VAR( UIDisplayGrid, bool, m_allowScaleUpToFit );
	DEFINE_VAR( UIDisplayGrid, bool, m_allowScaleDownToFit );
	DEFINE_VAR( UIDisplayGrid, vec2, m_padding );
	DEFINE_VAR( UIDisplayGrid, bool, m_wrapRows );
	DEFINE_VAR( UIDisplayGrid, TextMetrics::Alignment, m_alignment );
	DEFINE_VAR( UIDisplayGrid, TextMetrics::VerticalAlignment, m_verticalAlignment );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( UIDisplayGrid )

	vec2 UIDisplayGrid::effectiveCellSize() const
	{
		auto cellSizeToUse = m_cellSize;
		
		if( cellSizeToUse.x <= 0 || cellSizeToUse.y <= 0 )
		{
			const auto largestChild = largestChildDimensions( *this );
			if( cellSizeToUse.x <= 0 )
			{
				cellSizeToUse.x = largestChild.x;
			}
			if( cellSizeToUse.y <= 0 )
			{
				cellSizeToUse.y = largestChild.y;
			}
		}

		return cellSizeToUse;
	}
	
	vec2 UIDisplayGrid::effectiveCellSpacing() const
	{
		return effectiveCellSize() + m_padding;
	}
	
	void UIDisplayGrid::arrangeChildren()
	{
		const auto& myFrame = frame();
	
		const auto nChildren = numChildren();
		if( nChildren > 0 )
		{
			vec2 offsetPerStep = effectiveCellSpacing();
			
			if( offsetPerStep.x == 0 || offsetPerStep.y == 0 )
			{
				dev_warning( this << " had zero-length step." );
				return;
			}
			
			// Calculate the required dimensions.
			//
			const vec2 firstCellCenter = myFrame.ulCorner() + effectiveCellSize() * 0.5f;
			
			auto myFrameSize = myFrame.dimensions();
			
			size_t gridColumns = std::max( size_t{ 1 }, static_cast< size_t >( myFrameSize.x / offsetPerStep.x ));
			size_t gridRows = nChildren / gridColumns;
			
			if( !m_wrapRows )
			{
				gridRows = std::max( size_t{ 1 }, static_cast< size_t >( myFrameSize.y / offsetPerStep.y ));
				gridColumns = nChildren / gridRows;
			}
			
			ASSERT( gridRows > 0 );
			ASSERT( gridColumns > 0 );
			
			for( size_t iChild = 0; iChild < nChildren; ++iChild )
			{
				auto child = getChildAt( iChild );
				
				vec2 gridPosition( static_cast< real >( iChild % gridColumns ), static_cast< real >( iChild / gridColumns ));
				
				if( !m_wrapRows )
				{
					gridPosition.set( static_cast< real >( iChild % gridRows ), static_cast< real >( iChild / gridRows ));
				}
				
				// Fit to cell by scaling.
				//
				if( m_allowScaleUpToFit || m_allowScaleDownToFit )
				{
					auto desiredScale = getFitRatio( child->localBounds().dimensions(), effectiveCellSize() );
					if( !m_allowScaleUpToFit )
					{
						desiredScale = std::min( desiredScale, 1.0f );
					}
					if( !m_allowScaleDownToFit )
					{
						desiredScale = std::max( desiredScale, 1.0f );
					}
					
					child->scale( desiredScale );
				}
				
				// Apply alignment.
				//
				const auto childBounds = child->localBounds().dimensions() * child->scale();
				const auto extraSpace = effectiveCellSize() - childBounds;
				vec2 alignmentOffset;
				switch( m_alignment )
				{
					case TextMetrics::Alignment::Left:
						alignmentOffset.x = extraSpace.x * -0.5f;
						break;
					case TextMetrics::Alignment::Centered:
						alignmentOffset.x = extraSpace.x * 0;
						break;
					case TextMetrics::Alignment::Right:
						alignmentOffset.x = extraSpace.x * 0.5f;
						break;
				}
				switch( m_verticalAlignment )
				{
					case TextMetrics::VerticalAlignment::Top:
						alignmentOffset.y = extraSpace.y * -0.5f;
						break;
					case TextMetrics::VerticalAlignment::Middle:
						alignmentOffset.y = extraSpace.y * 0;
						break;
					case TextMetrics::VerticalAlignment::Bottom:
						alignmentOffset.y = extraSpace.y * 0.5f;
						break;
				}
				
				child->position( firstCellCenter + offsetPerStep * gridPosition + alignmentOffset );					// TODO offset for non-centered child bounds.
			}
		}
		
		m_lastArrangementFrame = myFrame;
		
		PROMISES( !isDirtyArrangement() );
	}
	
	void UIDisplayGrid::update()
	{
		Super::update();
		
		if( isDirtyArrangement() )
		{
			arrangeChildren();
		}
	}
	
	void UIDisplayGrid::onAddedToStage()
	{
		Super::onAddedToStage();
		
		arrangeChildren();
	}

	bool UIDisplayGrid::isDirtyArrangement() const
	{
		return frame() != m_lastArrangementFrame;
	}
}

