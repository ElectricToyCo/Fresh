//
//  ICViewController.h
//  iOScURL
//
//  Created by Nick Zitzmann on 11/3/12.
//  Copyright (c) 2012 Nick Zitzmann. All rights reserved.
//

#import <UIKit/UIKit.h>
#include "curl/curl.h"

@interface ICViewController : UIViewController
{
	IBOutlet UITextField *_urlTxt;
	IBOutlet UITextField *_usernameTxt;
	IBOutlet UITextField *_passwordTxt;
	IBOutlet UISegmentedControl *_httpCommandSegment;
	IBOutlet UISwitch *_insecureModeSwitch;
	IBOutlet UITextView *_resultTxt;
	
	CURL *_curl;
	struct curl_slist *_headers;
	NSData *_dataToSend;
	size_t _dataToSendBookmark;
	NSMutableData *_dataReceived;	// we don't actually use this; it's just an example to show you how to read data from a server
}
- (IBAction)go:(id)sender;

@end
