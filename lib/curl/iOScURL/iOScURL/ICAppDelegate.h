//
//  ICAppDelegate.h
//  iOScURL
//
//  Created by Nick Zitzmann on 11/3/12.
//  Copyright (c) 2012 Nick Zitzmann. All rights reserved.
//

#import <UIKit/UIKit.h>

@class ICViewController;

@interface ICAppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;

@property (strong, nonatomic) ICViewController *viewController;

@end
