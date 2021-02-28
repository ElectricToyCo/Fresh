//
//  OpenGLES2ContextHandler.m
//  Fresh
//
//  Created by Jeff Wofford on 5/28/10.
//  Copyright 2010 jeffwofford.com. All rights reserved.
//

#import "OpenGLES2ContextHandler.h"


@implementation OpenGLES2ContextHandler

- (id)init
{
    if ((self = [super init]))
    {
		context = [[EAGLContext alloc] initWithAPI: kEAGLRenderingAPIOpenGLES2];
		
		if (!context || ![EAGLContext setCurrentContext:context])
		{
			return nil;
		}
		
		defaultFramebuffer = 0;
		colorRenderbuffer = 0;
		[self createFrameBuffers];
		
	}
	return self;
}

- (void) createFrameBuffers
{
	assert( defaultFramebuffer == 0 );
	assert( colorRenderbuffer == 0 );

	// Create default framebuffer object. The backing will be allocated for the current layer in -resizeFromLayer
	glGenFramebuffers( 1, &defaultFramebuffer );
	glGenRenderbuffers( 1, &colorRenderbuffer );
	glBindFramebuffer( GL_FRAMEBUFFER, defaultFramebuffer );
	glBindRenderbuffer( GL_RENDERBUFFER, colorRenderbuffer );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorRenderbuffer );
}

- (void) destroyFrameBuffers
{
	if( defaultFramebuffer )
    {
        glDeleteFramebuffers( 1, &defaultFramebuffer );
        defaultFramebuffer = 0;
    }
	
    if( colorRenderbuffer )
    {
        glDeleteRenderbuffers( 1, &colorRenderbuffer );
        colorRenderbuffer = 0;
    }
}

- (BOOL) hasFrameBuffers
{
	return defaultFramebuffer != 0;
}

- (void) swapBuffers
{
    glBindRenderbuffer( GL_RENDERBUFFER, colorRenderbuffer );
    [context presentRenderbuffer:GL_RENDERBUFFER];
}

- (BOOL)resizeFromLayer:(CAEAGLLayer *)layer
{	
    // Allocate color buffer backing based on the current layer size
	//
	glBindRenderbuffer( GL_RENDERBUFFER, colorRenderbuffer );
    [context renderbufferStorage: GL_RENDERBUFFER fromDrawable: layer];
    glGetRenderbufferParameteriv( GL_RENDERBUFFER, GL_RENDERBUFFER_WIDTH, &backingWidth );
    glGetRenderbufferParameteriv( GL_RENDERBUFFER, GL_RENDERBUFFER_HEIGHT, &backingHeight );
	
#if 0	// TODO Non-packed. Rumor has it this may be required on the simulator (only)
	
	// TODO Fresh: make application-controlled.
	//
	GLuint depthRenderbuffer;
	glGenRenderbuffers( 1, &depthRenderbuffer );
	glBindRenderbuffer( GL_RENDERBUFFER, depthRenderbuffer );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH_COMPONENT16, backingWidth, backingHeight );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthRenderbuffer );
	
	// TODO Fresh: make application-controlled.
	//
	GLuint stencilRenderbuffer;
	glGenRenderbuffers( 1, &stencilRenderbuffer );
	glBindRenderbuffer( GL_RENDERBUFFER, stencilRenderbuffer );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_STENCIL_INDEX8, backingWidth, backingHeight );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, stencilRenderbuffer );
	
#elif 1		// No depth buffer, thanks.
	
	// TODO packed? Possibly required but only on the device.
	//
	GLuint depthStencilRenderbuffer;
	glGenRenderbuffers( 1, &depthStencilRenderbuffer );
	glBindRenderbuffer( GL_RENDERBUFFER, depthStencilRenderbuffer );
	glRenderbufferStorage( GL_RENDERBUFFER, GL_DEPTH24_STENCIL8_OES, backingWidth, backingHeight );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthStencilRenderbuffer );
	glFramebufferRenderbuffer( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencilRenderbuffer );
	
#endif
	
    if( glCheckFramebufferStatus( GL_FRAMEBUFFER ) != GL_FRAMEBUFFER_COMPLETE )
    {
        NSLog( @"Failed to make complete framebuffer object %x", glCheckFramebufferStatus( GL_FRAMEBUFFER ));
        return NO;
    }
	
    return YES;
}

- (void)dealloc
{
    // Tear down GL
	[self destroyFrameBuffers];
	
    // Tear down context
    if ([EAGLContext currentContext] == context)
	{
        [EAGLContext setCurrentContext:nil];
	}
	
	[super dealloc];
}

@end
