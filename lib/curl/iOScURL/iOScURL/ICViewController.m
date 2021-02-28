//
//  ICViewController.m
//  iOScURL
//
//  Created by Nick Zitzmann on 11/3/12.
//  Copyright (c) 2012 Nick Zitzmann. All rights reserved.
//

#import "ICViewController.h"

@interface ICViewController (Private)
- (size_t)copyUpToThisManyBytes:(size_t)bytes intoThisPointer:(void *)pointer;
- (void)insertText:(NSString *)text;
- (void)receivedData:(NSData *)data;
- (void)rewind;
@end

int ICCurlDebugCallback(CURL *curl, curl_infotype infotype, char *info, size_t infoLen, void *contextInfo)
{
	ICViewController *vc = (__bridge ICViewController *)contextInfo;
	NSData *infoData = [NSData dataWithBytes:info length:infoLen];
	NSString *infoStr = [[NSString alloc] initWithData:infoData encoding:NSUTF8StringEncoding];
	
	if (infoStr)
	{
		infoStr = [infoStr stringByReplacingOccurrencesOfString:@"\r\n" withString:@"\n"];	// convert Windows CR/LF to just LF
		infoStr = [infoStr stringByReplacingOccurrencesOfString:@"\r" withString:@"\n"];	// convert remaining CRs to LFs
		switch (infotype)
		{
			case CURLINFO_DATA_IN:
				[vc insertText:infoStr];
				break;
			case CURLINFO_DATA_OUT:
				[vc insertText:[infoStr stringByAppendingString:@"\n"]];
				break;
			case CURLINFO_HEADER_IN:
				[vc insertText:[@"< " stringByAppendingString:infoStr]];
				break;
			case CURLINFO_HEADER_OUT:
				infoStr = [infoStr stringByReplacingOccurrencesOfString:@"\n" withString:@"\n> "];	// start each line with a /
				[vc insertText:[NSString stringWithFormat:@"> %@\n", infoStr]];
				break;
			case CURLINFO_TEXT:
				[vc insertText:[@"* " stringByAppendingString:infoStr]];
				break;
			default:	// ignore the other CURLINFOs
				break;
		}
	}
	return 0;
}


curlioerr ICCurlIoctlCallback(CURL *handle, int cmd, void *clientp)
{
	ICViewController *vc = (__bridge ICViewController *)clientp;
	
	if (cmd == CURLIOCMD_RESTARTREAD)	// this is our cue to rewind _dataToSendBookmark back to the start
	{
		[vc rewind];
		return CURLIOE_OK;
	}
	return CURLIOE_UNKNOWNCMD;
}


size_t ICCurlReadCallback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	const size_t sizeInBytes = size*nmemb;
	ICViewController *vc = (__bridge ICViewController *)userdata;
	
	return [vc copyUpToThisManyBytes:sizeInBytes intoThisPointer:ptr];
}


size_t ICCurlWriteCallback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
	const size_t sizeInBytes = size*nmemb;
	ICViewController *vc = (__bridge ICViewController *)userdata;
	NSData *data = [[NSData alloc] initWithBytes:ptr length:sizeInBytes];
	
	[vc receivedData:data];
	return sizeInBytes;
}


@implementation ICViewController

- (id)initWithNibName:(NSString *)nibNameOrNil bundle:(NSBundle *)nibBundleOrNil
{
	self = [super initWithNibName:nibNameOrNil bundle:nibBundleOrNil];
	if (self)
	{
		_dataReceived = [[NSMutableData alloc] init];
		_curl = curl_easy_init();
		
		// Some settings I recommend you always set:
		curl_easy_setopt(_curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);	// support basic, digest, and NTLM authentication
		curl_easy_setopt(_curl, CURLOPT_NOSIGNAL, 1L);	// try not to use signals
		curl_easy_setopt(_curl, CURLOPT_USERAGENT, curl_version());	// set a default user agent
		
		// Things specific to this app:
		curl_easy_setopt(_curl, CURLOPT_VERBOSE, 1L);	// turn on verbose logging; your app doesn't need to do this except when debugging a connection
		curl_easy_setopt(_curl, CURLOPT_DEBUGFUNCTION, ICCurlDebugCallback);
		curl_easy_setopt(_curl, CURLOPT_DEBUGDATA, self);
		curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, ICCurlWriteCallback);
		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, self);	// prevent libcurl from writing the data to stdout
	}
	return self;
}


- (void)dealloc
{
	if (_headers)
		curl_slist_free_all(_headers);
	curl_easy_cleanup(_curl);
}


- (void)viewDidLoad
{
    [super viewDidLoad];
	// Do any additional setup after loading the view, typically from a nib.
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];
    // Dispose of any resources that can be recreated.
}


#pragma mark -


- (IBAction)go:(id)sender
{
	if (_urlTxt.text && ![_urlTxt.text isEqualToString:@""])
	{
		CURLcode theResult;
		NSURL *url = [NSURL URLWithString:_urlTxt.text];
		NSMutableString *cookies = [NSMutableString string];
		NSString *proxyHost = @"";
		NSNumber *proxyPort = @0L;
		NSDictionary *proxySettings = (__bridge_transfer NSDictionary *)CFNetworkCopySystemProxySettings();
		
		if (_headers)
		{
			curl_slist_free_all(_headers);
			_headers = NULL;
		}
		[_dataReceived setLength:0U];
		_dataToSendBookmark = 0U;
		
		curl_easy_setopt(_curl, CURLOPT_URL, url.absoluteString.UTF8String);	// little warning: curl_easy_setopt() doesn't retain the memory passed into it, so if the memory used by calling url.absoluteString.UTF8String is freed before curl_easy_perform() is called, then it will crash. IOW, don't drain the autorelease pool before making the call
		curl_easy_setopt(_curl, CURLOPT_USERNAME, _usernameTxt.text ? _usernameTxt.text.UTF8String : "");
		curl_easy_setopt(_curl, CURLOPT_PASSWORD, _passwordTxt.text ? _passwordTxt.text.UTF8String : "");
		
		// Set up proxies:
		if ([proxySettings objectForKey:(NSString *)kCFNetworkProxiesHTTPEnable] && [[proxySettings objectForKey:(NSString *)kCFNetworkProxiesHTTPEnable] boolValue])
		{
			if ([proxySettings objectForKey:(NSString *)kCFNetworkProxiesHTTPProxy])
				proxyHost = [proxySettings objectForKey:(NSString *)kCFNetworkProxiesHTTPProxy];
			if ([proxySettings objectForKey:(NSString *)kCFNetworkProxiesHTTPPort])
				proxyPort = [proxySettings objectForKey:(NSString *)kCFNetworkProxiesHTTPPort];
		}
		curl_easy_setopt(_curl, CURLOPT_PROXY, proxyHost.UTF8String);
		curl_easy_setopt(_curl, CURLOPT_PROXYPORT, proxyPort.longValue);
		
		// Set up cookies:
		[[[NSHTTPCookieStorage sharedHTTPCookieStorage] cookiesForURL:[NSURL URLWithString:_urlTxt.text]] enumerateObjectsUsingBlock:^(NSHTTPCookie *cookie, NSUInteger i, BOOL *stop) {
			if ([cookie.path isEqualToString:@"/"] || [cookie.path isEqualToString:url.path])
				[cookies appendFormat:@"%@=%@; ", cookie.name, cookie.value];	// name1=value1; name2=value2; etc.
		}];
		curl_easy_setopt(_curl, CURLOPT_COOKIE, cookies.UTF8String);
		
		// Set up the HTTP operation:
		curl_easy_setopt(_curl, CURLOPT_UPLOAD, 0L);
		curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, NULL);
		switch (_httpCommandSegment.selectedSegmentIndex)
		{
			case 0L: default:
				curl_easy_setopt(_curl, CURLOPT_HTTPGET, 1L);
				break;
			case 1L:
				curl_easy_setopt(_curl, CURLOPT_NOBODY, 1L);
				break;
			case 2L:
				curl_easy_setopt(_curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
				break;
			case 3L:
				_dataToSend = [@"<?xml version=\"1.0\" encoding=\"utf-8\" ?><D:propfind xmlns:D=\"DAV:\"><D:allprop/></D:propfind>" dataUsingEncoding:NSUTF8StringEncoding];
				curl_easy_setopt(_curl, CURLOPT_UPLOAD, 1L);
				curl_easy_setopt(_curl, CURLOPT_READFUNCTION, ICCurlReadCallback);
				curl_easy_setopt(_curl, CURLOPT_READDATA, self);
				curl_easy_setopt(_curl, CURLOPT_INFILESIZE, _dataToSend.length);
				curl_easy_setopt(_curl, CURLOPT_IOCTLFUNCTION, ICCurlIoctlCallback);
				curl_easy_setopt(_curl, CURLOPT_IOCTLDATA, self);
				curl_easy_setopt(_curl, CURLOPT_CUSTOMREQUEST, "PROPFIND");
				_headers = curl_slist_append(_headers, "Content-Type: application/xml; charset=\"utf-8\"");
				_headers = curl_slist_append(_headers, "Depth: 1");	// the Depth: header is required for the PROPFIND command
				curl_easy_setopt(_curl, CURLOPT_HTTPHEADER, _headers);
		}
		
		// Don't mess with CURLOPT_SSL_VERIFYHOST or CURLOPT_SSL_VERIFYPEER unless you **really** know what you're doing! libcurl is secure by default; we just switch off security in this sample if the user flipped the switch to enable insecure mode (which should never be on by default, and in fact, I'd recommend against adding such a switch in your apps).
		curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYHOST, _insecureModeSwitch.on ? 0L : 2L);
		curl_easy_setopt(_curl, CURLOPT_SSL_VERIFYPEER, _insecureModeSwitch.on ? 0L : 1L);
		
		_resultTxt.text = @"";
		theResult = curl_easy_perform(_curl);
		if (theResult == CURLE_OK)
			_resultTxt.text = [_resultTxt.text stringByAppendingString:NSLocalizedString(@"Mission accomplished!", @"cURL success string")];
		else
			_resultTxt.text = [_resultTxt.text stringByAppendingFormat:NSLocalizedString(@"Mission failed: Error %d", @"cURL failure string"), theResult];
	}
}

@end

@implementation ICViewController (Private)

- (size_t)copyUpToThisManyBytes:(size_t)bytes intoThisPointer:(void *)pointer
{
	size_t bytesToGo = _dataToSend.length-_dataToSendBookmark;
	size_t bytesToGet = MIN(bytes, bytesToGo);
	
	if (bytesToGo)
	{
		[_dataToSend getBytes:pointer range:NSMakeRange(_dataToSendBookmark, bytesToGet)];
		_dataToSendBookmark += bytesToGet;
		return bytesToGet;
	}
	return 0U;
}

- (void)insertText:(NSString *)text
{
	@autoreleasepool
	{
		_resultTxt.text = [_resultTxt.text stringByAppendingString:text];
		[[NSRunLoop mainRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow:0.1]];	// force the run loop to run and do drawing while curl_easy_perform() hasn't returned yet
	}
}


- (void)receivedData:(NSData *)data
{
	[_dataReceived appendData:data];
}


- (void)rewind
{
	_dataToSendBookmark = 0U;
}

@end
