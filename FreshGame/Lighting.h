//
//  Lighting.h
//  Fresh
//
//  Created by Jeff Wofford on 12/4/11.
//  Copyright (c) 2011 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_Lighting_h
#define Fresh_Lighting_h

#include "RenderTarget.h"
#include "Sprite.h"
#include "Segment.h"

namespace fr
{
	class LightMask : public DisplayObjectWithMesh
	{
		FRESH_DECLARE_CLASS( LightMask, DisplayObjectWithMesh )
	public:
		
		SYNTHESIZE( DisplayObjectContainer::wptr, owner );
		
	protected:
		
		virtual void preDraw( TimeType relativeFrameTime ) override;
		
	private:
		
		VAR( DisplayObjectContainer::wptr, m_owner );
	};
	
	///////////////////////////////////////////////////////

	class FreshTileGrid;
	
	class LightSource : public Sprite
	{
		FRESH_DECLARE_CLASS( LightSource, Sprite )
	public:
		
		static const unsigned int NO_DESTROY_COLOR = 0x00000001;
		
		void setDestroyStyle( TimeType duration, real destroyRadius = -1.0f, Color destroyColor = NO_DESTROY_COLOR );
		virtual void destroyWithAnimation();
		bool isDestroying() const;
		
		SYNTHESIZE_GET( real, radius )		
		void radius( real r );
		// REQUIRES( r > 0 );
		
		SYNTHESIZE( bool, doShadows );
		SYNTHESIZE( unsigned int, groups );
		
		WeakPtr< FreshTileGrid > tileGrid() const;
		WeakPtr< FreshTileGrid > tileGrid( WeakPtr< FreshTileGrid > tileGrid_ );
		
		SYNTHESIZE_GET( real, spotHalfArcAngleDegrees );
		void spotHalfArcAngleDegrees( real angle );
		
		SYNTHESIZE_GET( real, spotNearRadius );
		void spotNearRadius( real radius );
		
		bool matchesGroups( unsigned int groups ) const					{ return ( m_groups & groups ) != 0; }
		
		bool isSpotLight() const			{ return m_spotHalfArcAngleDegrees > 0 && m_spotHalfArcAngleDegrees < 90.0f; }
		
		Color getLitColorAt( const vec2& pos, unsigned int includeGroups = ~0, bool ignoreShadows = false ) const;
		
		bool mightAffect( const vec2& pos, real radius = 0 ) const;
		
		void setGradientTexture( SmartPtr< Texture > texture );
		void setGradientTextureByName( const std::string& name );
		
		virtual void update() override;
		
		virtual void onAddedToStage() override;
		
	protected:

		void setDirty()						{ m_isDirty = true; }
		
		struct State
		{
			vec2 position = vec2::ZERO;
			angle rotation = angle( 0 );
			real radius = 0;
			real spotNearRadius = 0;
			real spotHalfArcAngleDegrees = 0;
			Color color = Color::Invisible;
			bool doShadows = false;
			
			bool operator==( const State& state ) const
			{
				return position == state.position &&
				rotation == state.rotation &&
				radius == state.radius &&
				spotNearRadius == state.spotNearRadius && 
				spotHalfArcAngleDegrees == state.spotHalfArcAngleDegrees &&
				doShadows == state.doShadows;
			}
			bool operator!=( const State& state ) const 
			{ 
				return !operator==( state );
			}
		};
		
		virtual void preRender( TimeType relativeFrameTime ) override;
		
		virtual void recordState( State& state, TimeType relativeFrameTime ) const;
		void updateCachedLightingInfo( const State& currentState );
		
		void setupGradientDraw();
		
		bool isPointInShadow( const vec2& pos ) const;
		
		void updateDestroy();
		
	private:
		
		DVAR( real, m_radius, 0.0f );
		DVAR( real, m_overlayRadiusScalar, 1.0f );
		DVAR( real, m_spotNearRadius, 0.0f );
		DVAR( real, m_spotHalfArcAngleDegrees, 0.0f );	// If <= 0 or >= 90, specifies an omni light.
		DVAR( unsigned int, m_groups, 0x80000000 );		// Bitmask identifying which "groups" this light is a part of. Used for queries.

		VAR( DisplayObjectWithMesh::ptr, m_gradientHost );
		VAR( LightMask::ptr, m_lightMask );
		VAR( DisplayObjectWithMesh::ptr, m_overlay );
		
		// Destroy animation.
		//
		DVAR( Color, m_destroyedColor, NO_DESTROY_COLOR );
		DVAR( real, m_destroyedRadius, 1.0f );
		DVAR( TimeType, m_destroyDuration, 0.4 );
		
		DisplayObjectState m_destroyInitialState;
		DisplayObjectState m_destroyedState;
		TimeType m_destroyStartTime = -1.0;
				
		std::vector< vec2 > m_shadowTriangles;

		VertexStructure::ptr m_vertexStructure;
		
		WeakPtr< FreshTileGrid > m_tileGrid;

		State m_lastCachedState;
		
		DVAR( bool, m_doShadows, true );
		
		bool m_isDirty = true;
		
		friend class Lighting;
	};

	/////////////////////////////////////////////////////////////////////////////////////
	
	class Lighting : public Sprite
	{
		FRESH_DECLARE_CLASS( Lighting, Sprite )
	public:
		
		SYNTHESIZE( Color, ambientColor )
		SYNTHESIZE( real, desiredAmbientColorScalar )
		SYNTHESIZE( real, ambientColorScalarLerpAlpha )
		
		Color getLitColorAt( const vec2& pos, unsigned int includeGroups = ~0, bool ignoreShadows = false, bool includeAmbient = true ) const;
		
		std::vector< LightSource::ptr > getNearestLightSources(const vec2& pos,
															   size_t maxLights = 0,
															   unsigned int includeGroups = ~0,
															   bool ignoreShadows = false );
		
		template< typename FunctionT >
		void eachLightSource( FunctionT&& fn )
		{
			forEachChild< LightSource >( std::move( fn ));
		}

		template< typename FunctionT >
		void eachLightSource( FunctionT&& fn ) const
		{
			forEachChild< LightSource >( std::move( fn ));
		}

		void dirtyAffectedLights( const vec2& pos, real radius );
		
		virtual void update() override;
		
	protected:
		
		SYNTHESIZE( real, ambientColorScalar )
		
		virtual void draw( TimeType relativeFrameTime, RenderInjector* injector = nullptr ) override;
		
	private:
		
		VAR( Color, m_ambientColor );
		DVAR( real, m_ambientColorScalar, 1.0f );		
		DVAR( real, m_desiredAmbientColorScalar, 1.0f );
		DVAR( real, m_ambientColorScalarLerpAlpha, 0.5f );
		VAR( SmartPtr< RenderTarget >, m_pRenderTarget );
		VAR( WeakPtr< FreshTileGrid >, m_tileGrid );
	};
	
}

#endif
