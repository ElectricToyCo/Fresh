//
//  GameView_iOS.h
//  Fresh
//
//  Created by Jeff Wofford on 5/29/09.
//  Copyright jeffwofford.com 2009. All rights reserved.
//


#import <UIKit/UIKit.h>

@class OpenGLES2ContextHandler;

@interface GameView : UIView 
- (id)initWithFrame:(CGRect)rect frameRate:(NSTimeInterval)fps contextHandler:(OpenGLES2ContextHandler*) handler;
- (void)startAnimation;
- (void)stopAnimation;
- (void)drawView;
- (void)setFramesPerSecond:(NSTimeInterval)fps;
- (void)applicationWillTerminate:(UIApplication*) application;
@end
