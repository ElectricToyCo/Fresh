//
//  OpenGLES2ContextHandler.h
//  Fresh
//
//  Created by Jeff Wofford on 5/28/10.
//  Copyright 2010 jeffwofford.com. All rights reserved.
//

#import <QuartzCore/QuartzCore.h>

#import <OpenGLES/EAGL.h>
#import <OpenGLES/EAGLDrawable.h>

#import <OpenGLES/ES2/gl.h>
#import <OpenGLES/ES2/glext.h>

@interface OpenGLES2ContextHandler : NSObject
{
    EAGLContext *context;
	
    // The pixel dimensions of the CAEAGLLayer
    GLint backingWidth;
    GLint backingHeight;
	
    // The OpenGL ES names for the framebuffer and renderbuffer used to render to this view
    GLuint defaultFramebuffer, colorRenderbuffer;
}

- (void) createFrameBuffers;
- (void) destroyFrameBuffers;
- (BOOL) hasFrameBuffers;

- (void) swapBuffers;
- (BOOL) resizeFromLayer:(CAEAGLLayer *)layer;

@end
