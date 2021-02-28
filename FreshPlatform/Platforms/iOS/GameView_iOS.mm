//
//  GameView_iOS.mm
//  Fresh
//
//  Created by Jeff Wofford on 5/29/09.
//  Copyright jeffwofford.com 2009. All rights reserved.
//



#import <QuartzCore/QuartzCore.h>
#import <OpenGLES/EAGLDrawable.h>

#import "GameView_iOS.h"

#include "Renderer.h"
#include "Asset.h"
#include "Application.h"
#include "FreshDebug.h"
#include "FreshTime.h"
#import "OpenGLES2ContextHandler.h"

using namespace fr;

@implementation GameView
{
	OpenGLES2ContextHandler* contextHandler;
	
	BOOL animating;
	NSTimeInterval framesPerSecond;
	
	// Use of the CADisplayLink class is the preferred method for controlling your animation timing.
	// CADisplayLink will link to the main display and fire every vsync when added to a given run-loop.
	// The NSTimer class is used only as fallback when running on a pre 3.1 device where CADisplayLink
	// isn't available.
	CADisplayLink* displayLink;
}

+ (Class) layerClass
{
    return [CAEAGLLayer class];
}

- (id)initWithFrame:(CGRect)rect frameRate:(NSTimeInterval)fps contextHandler:(OpenGLES2ContextHandler*) handler
{
	self = [super initWithFrame: rect];
	if( self )
	{	
        CAEAGLLayer* eaglLayer = (CAEAGLLayer*) self.layer;
        
        eaglLayer.opaque = YES;
        eaglLayer.drawableProperties = [NSDictionary dictionaryWithObjectsAndKeys:
                                        [NSNumber numberWithBool:NO], 
										kEAGLDrawablePropertyRetainedBacking, 
										kEAGLColorFormatRGBA8, 
										kEAGLDrawablePropertyColorFormat, 
										nil];
        
		ASSERT( handler != nil );
		contextHandler = handler;
		animating = NO;
		framesPerSecond = fps;
    }
	
    return self;
}

- (void)drawView 
{
	Application::instance().updateFrame();
	
    // Present the finished scene.
	//
	[contextHandler swapBuffers];
}


- (void)layoutSubviews 
{
	[contextHandler resizeFromLayer: (CAEAGLLayer*)self.layer];	
    [self drawView];
}

- (void)startAnimation 
{
	if( ![contextHandler hasFrameBuffers] )
	{
		[contextHandler createFrameBuffers];
		[self setNeedsLayout];
	}
	
	if( !animating )
	{
		// Setup the displaylink object to maintain the desired frame rate.
		displayLink = [CADisplayLink displayLinkWithTarget:self selector:@selector( drawView )];
		
		if( [displayLink respondsToSelector: @selector(preferredFramesPerSecond)] )
		{
			displayLink.preferredFramesPerSecond = (NSInteger) framesPerSecond;
		}
		else
		{
			// Calculate frame interval based on desired frame rate and the assumed screen refresh rate.
			const NSTimeInterval expectedRefreshRate = 60.0;
			
			NSInteger animationFrameInterval = (NSInteger)( expectedRefreshRate / framesPerSecond );
			if( animationFrameInterval < 0 )
			{
				NSLog( @"Invalid frame rate: %g", framesPerSecond );
				animationFrameInterval = 1;
			}
		}
		
		[displayLink addToRunLoop:[NSRunLoop currentRunLoop] forMode:NSDefaultRunLoopMode];
		animating = YES;
	}
}

- (void)stopAnimation 
{
    if (animating)
    {
		[displayLink invalidate];
		displayLink = nil;
		
        animating = NO;
    }
	
	if( [contextHandler hasFrameBuffers] )
	{
		[contextHandler destroyFrameBuffers];
	}
	
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TOUCH STUFF
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::vector< UITouch* > g_currentTouches;

int touchOrdinal( UITouch* touch )
{
	ASSERT( touch );
	auto iter = std::find( g_currentTouches.begin(), g_currentTouches.end(), touch );
	if( iter != g_currentTouches.end() )
	{
		return static_cast< int >( iter - g_currentTouches.begin() );
	}
	else
	{
		return -1;
	}
}

static void createAppTouches( UIView* view, CGSize viewSize, NSSet* cocoaTouches, Application::Touches& outAppTouches )
{
	for( UITouch* touch in cocoaTouches )
	{
		CGPoint touchPoint = [touch locationInView:view];
		CGPoint lastTouchPoint = [touch previousLocationInView:view];
		
		auto ordinal = touchOrdinal( touch );
		ASSERT( ordinal >= 0 );
		
		outAppTouches.push_back( Application::Touch( 
												 vec2( touchPoint.x, viewSize.height - touchPoint.y ) * view.contentScaleFactor,
												 vec2( lastTouchPoint.x, viewSize.height - lastTouchPoint.y ) * view.contentScaleFactor,
												 vec2::ZERO,	// wheel
												 ordinal,
												 static_cast< int >( g_currentTouches.size() ),
												 static_cast< int >( touch.tapCount ),
												 (__bridge void *)( touch )));
	}		
}

- (void)touchesBegan:(NSSet *)touches withEvent:(UIEvent *)event 
{
	// Update the touch tracker.
	//
	for( UITouch* touch in touches )
	{
		ASSERT( touchOrdinal( touch ) < 0 );
		g_currentTouches.push_back( touch );
	}
	
	if( !Application::doesExist() )
	{
		return;
	}
	
	Application::Touches appTouches;
	
	createAppTouches( self, self.bounds.size, touches, appTouches );
	
	Application::instance().onTouchesBegin( appTouches.begin(), appTouches.end() );
}

- (void)touchesMoved:(NSSet *)touches withEvent:(UIEvent *)event 
{	
	if( !Application::doesExist() )
	{
		return;
	}
	
	Application::Touches appTouches;
	
	createAppTouches( self, self.bounds.size, touches, appTouches );

	Application::instance().onTouchesMove( appTouches.begin(), appTouches.end() );
}

- (void)touchesEnded:(NSSet *)touches withEvent:(UIEvent *)event 
{	
	if( !Application::doesExist() )
	{
		return;
	}
	
	//	NSLog( @"touchesEnded" );
	
	Application::Touches appTouches;
	
	createAppTouches( self, self.bounds.size, touches, appTouches );
	
	Application::instance().onTouchesEnd( appTouches.begin(), appTouches.end() );

	// Update the touch tracker.
	//
	for( UITouch* touch in touches )
	{
		auto ordinal = touchOrdinal( touch );
		ASSERT( ordinal >= 0 );
		g_currentTouches.erase( g_currentTouches.begin() + ordinal );
	}
}

- (void)touchesCancelled:(NSSet *)touches withEvent:(UIEvent *)event
{
	if( !Application::doesExist() )
	{
		return;
	}
	
	Application::Touches appTouches;
	
	createAppTouches( self, self.bounds.size, touches, appTouches );
	
	Application::instance().onTouchesCancelled( appTouches.begin(), appTouches.end() );
	
	// Update the touch tracker.
	//
	for( UITouch* touch in touches )
	{
		auto ordinal = touchOrdinal( touch );
		ASSERT( ordinal >= 0 );
		g_currentTouches.erase( g_currentTouches.begin() + ordinal );
	}
}

- (void)setFramesPerSecond:(NSTimeInterval)fps
{
	framesPerSecond = fps;
	
	// Apply the new rate.
	if( animating )
	{
		[self stopAnimation];
		[self startAnimation];
	}
}

- (void)applicationWillTerminate:(UIApplication*) application
{
	[self stopAnimation];
}

@end
