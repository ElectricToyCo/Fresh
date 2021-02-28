//
//  EdPointList.h
//  Fresh
//
//  Created by Jeff Wofford on 6/15/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_EdPointList_h
#define Fresh_EdPointList_h

#include "Sprite.h"

namespace fr
{
	
	class EdPointList : public Sprite
	{
		FRESH_DECLARE_CLASS( EdPointList, Sprite );
	public:
		
		virtual ~EdPointList();
		
		typedef std::function< vec2( const DisplayObject& from, const DisplayObject& to, const vec2& point ) > ChangeOfBasisFunction;

		void fnTransformPoint( ChangeOfBasisFunction&& fn )		{ m_fnTransformPoint = std::forward< ChangeOfBasisFunction >( fn ); }
		void fnTransformScale( ChangeOfBasisFunction&& fn )		{ m_fnTransformScale = std::forward< ChangeOfBasisFunction >( fn ); }
		
		SYNTHESIZE( DisplayObject::wptr, subject );
		SYNTHESIZE( Color, lineColor );
		SYNTHESIZE( Color, fillColor );
		SYNTHESIZE( bool, isLineLoop );
		
		void addProxyEventListener( Event::TypeRef type, CallbackFunctionAbstract::ptr fnCallback );
		
		void clearPoints();
		
		void addPoint( const vec2& point );
		
		template< typename iter_t >
		void create( iter_t begin, iter_t end );
		
		template< typename fn_t >
		void forEachPoint( fn_t&& fn );

		template< typename fn_t >
		void forEachPoint( fn_t&& fn ) const;
		
		virtual void update() override;
		
		std::vector< vec2 > points() const;
		
	protected:
		
		virtual void drawLines();
		
		void setupDefaults();
		
	private:
		
		VAR( DisplayObject::wptr, m_subject );
		VAR( ClassInfo::cptr, m_classPointProxy );
		DVAR( vec2, m_proxyScale, vec2( 4.0f ));
		DVAR( Color, m_lineColor, Color::Green );
		DVAR( Color, m_fillColor, Color::Invisible );
		DVAR( bool, m_isLineLoop, true );
				
		ChangeOfBasisFunction m_fnTransformPoint;
		ChangeOfBasisFunction m_fnTransformScale;
		
		Sprite::ptr m_markup;
		
		std::vector< std::pair< Event::Type, CallbackFunctionAbstract::ptr >> m_proxyCallbacks;		
	};
	
	template< typename iter_t >
	void EdPointList::create( iter_t begin, iter_t end )
	{
		clearPoints();
		while( begin != end )
		{
			addPoint( *begin++ );
		}
	}

	template< typename fn_t >
	void EdPointList::forEachPoint( fn_t&& fn )
	{
		setupDefaults();
		int iChild = 0;
		forEachChild< DisplayObject >( [&]( DisplayObject& p )
					 {
						 if( iChild > 0 )
						 {
							 vec2 pos = m_fnTransformPoint( *this, *m_subject, p.position() );
							 fn( pos );
							 p.position( m_fnTransformPoint( *m_subject, *this, pos ));
						 }
						 ++iChild;
					 } );
	}
	
	
	template< typename fn_t >
	void EdPointList::forEachPoint( fn_t&& fn ) const
	{
		ASSERT( m_fnTransformPoint );
		int iChild = 0;
		forEachChild< DisplayObject >( [&]( const DisplayObject& p )
					 {
						 if( iChild > 0 )
						 {
							 fn( m_fnTransformPoint( *this, *m_subject, p.position() ));
						 }
						 ++iChild;
					 } );
	}
}

#endif
