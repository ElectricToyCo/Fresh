//
//  FreshLicensing.mm
//  Fresh
//
//  Created by Jeff Wofford on 11/20/16.
//
//

#include "FreshLicensing.h"
#include "FreshDebug.h"
#include "FreshEssentials.h"
#import <StoreKit/StoreKit.h>

namespace
{
	const std::string PREFERENCE_KEY_APP_VERSION = "_LicensingAppVersion";
	const std::string PREFERENCE_PREFIX_FEATURE_PURCHASE = "_LicensingFeaturePurchased_";
	
	NSString* LICENSING_ERROR_DOMAIN = @"co.electrictoy.licensing";
	
	inline std::string convert( NSString* string )
	{
		if( string )
		{
			return [string UTF8String];
		}
		else
		{
			return {};
		}
	}
	
	__unused inline NSString* convert( const std::string& string )
	{
		return [NSString stringWithUTF8String: string.c_str() ];
	}
	
	
	fr::licensing::Error convertError( NSError* error )
	{
		if( error != nil )
		{
			return fr::licensing::Error{ convert( [error localizedDescription] ), static_cast< int >( [error code] ) };
		}
		else
		{
			return {};
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

@interface ReceiptFetcher: NSObject <SKRequestDelegate>
@property(atomic,copy) void (^pendingCompletionHandler)( NSDictionary* receiptJson, NSError* error );
- (void) updateReceipt: (void (^)( NSDictionary* receiptJson, NSError* error )) completionHandler;
- (void) requestDidFinish: (SKRequest *) request;
- (void) request: (SKRequest *) request didFailWithError:(NSError *) error;
@end
@implementation ReceiptFetcher
{
	SKReceiptRefreshRequest* receiptRequest;
}

- (void) updateReceipt: (void (^)( NSDictionary* receiptJson, NSError* error )) completionHandler
{
	self.pendingCompletionHandler = completionHandler;
	
	receiptRequest = [[SKReceiptRefreshRequest alloc] initWithReceiptProperties: nil];
	receiptRequest.delegate = self;
	[receiptRequest start];
}

- (void) callCompletionHandler: (NSDictionary*) receiptJson withError: (NSError*) error
{
	self.pendingCompletionHandler( receiptJson, error );
	self.pendingCompletionHandler = nil;
}

- (void) validateRequestWithStores: (NSArray< NSURL* >*) storeURLs with: (NSData*) requestData
{
	NSUInteger storeCount = [storeURLs count];
	ASSERT( storeCount > 0 );
	NSURL* storeURL = [storeURLs firstObject];
	ASSERT( storeURL );
	
	NSMutableURLRequest *storeRequest = [NSMutableURLRequest requestWithURL:storeURL];
	[storeRequest setHTTPMethod:@"POST"];
	[storeRequest setHTTPBody:requestData];
	
	// Make a connection to the iTunes Store on a background queue.
	[[NSURLSession sharedSession] dataTaskWithRequest:storeRequest
									completionHandler:^(NSData *data, NSURLResponse *response, NSError *connectionError)
	{
		 if (connectionError)
		 {
			 // No. Return this result as the error.
			 //
			 NSDictionary *errorInfo = @{ NSLocalizedDescriptionKey : @"Connection error." };
			 [self callCompletionHandler: nil withError: [NSError errorWithDomain:LICENSING_ERROR_DOMAIN code:3 userInfo:errorInfo]];
			 return;
		 }
		 else
		 {
			 NSError* jsonError;
			 NSDictionary* jsonResponse = [NSJSONSerialization JSONObjectWithData:data options:0 error:&jsonError];
			 
			 // Verify that this isn't a failed status code.
			 //
			 if( [jsonResponse[@"status"] integerValue] == 21007 )
			 {
				 // 21007 indicates that the receipt is from the sandbox store but we asked the production store for verification.
				 // Send it to the next app store in our list.
				 //
				 // Are there other stores to try?
				 if( storeCount > 1 )
				 {
					 // Yes. Try the rest.
					 NSArray< NSURL* >* remainingStores = [storeURLs subarrayWithRange:NSMakeRange( 1, storeCount - 1 )];
					 [self validateRequestWithStores:remainingStores with:requestData];
					 return;
				 }
			 }
			 
			 [self callCompletionHandler: jsonResponse withError: nil];
		 }
	 }];
}

- (void) requestDidFinish:(SKRequest *)request
{
	NSURL *receiptURL = [[NSBundle mainBundle] appStoreReceiptURL];
	NSData *receipt = [NSData dataWithContentsOfURL:receiptURL];
	
	if( !receipt )
	{
		NSDictionary *errorInfo = @{ NSLocalizedDescriptionKey : @"Could not load app receipt." };
		[self callCompletionHandler: nil withError: [NSError errorWithDomain:LICENSING_ERROR_DOMAIN code:1 userInfo:errorInfo]];
		return;
	}
	
	// Create the JSON object that describes the request
	NSError *error;
	NSDictionary *requestContents = @{ @"receipt-data": [receipt base64EncodedStringWithOptions:0] };
	NSData *requestData = [NSJSONSerialization dataWithJSONObject:requestContents
														  options:0
															error:&error];
	
	if( !requestData )
	{
		NSDictionary *errorInfo = @{ NSLocalizedDescriptionKey : @"App receipt is in an unrecognized format." };
		[self callCompletionHandler: nil withError: [NSError errorWithDomain:LICENSING_ERROR_DOMAIN code:2 userInfo:errorInfo]];
		return;
	}
	
	// See https://developer.apple.com/library/content/technotes/tn2413/_index.html
	// It advises to check the production store first, then the sandbox if production returns status code 21007
	[self validateRequestWithStores: @[[NSURL URLWithString:@"https://buy.itunes.apple.com/verifyReceipt"],
									   [NSURL URLWithString:@"https://sandbox.itunes.apple.com/verifyReceipt"],
									   ] with: requestData];
}
	 
 - (void) request:(SKRequest *)request didFailWithError:(NSError *)error
{
	[self callCompletionHandler: nil withError: error];
}

@end

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

@interface ProductListFetcher: NSObject <SKProductsRequestDelegate>
@property(atomic,copy) void (^pendingCompletionHandler)( NSArray<SKProduct *>* products );
- (void)updateProducts:(NSArray<NSString *> *)productIdentifiers completion: (void (^)( NSArray<SKProduct *>* products )) completionHandler;
@end

@implementation ProductListFetcher
{
	SKProductsRequest* productsRequest;
}

- (void)updateProducts:(NSArray<NSString *> *)productIdentifiers completion: (void (^)( NSArray<SKProduct *>* products )) completionHandler
{
	ASSERT( self.pendingCompletionHandler == nil );
	self.pendingCompletionHandler = completionHandler;
	
	productsRequest = [[SKProductsRequest alloc] initWithProductIdentifiers:[NSSet setWithArray:productIdentifiers]];
	productsRequest.delegate = self;
	[productsRequest start];
	
	// Prepare timeout (since productRequest sits forever in airplane mode).
	[self cancelTimeout];
	[self performSelector:@selector(requestTimedOut) withObject:self afterDelay:30.0f];
}

- (void)cancelTimeout
{
	[NSObject cancelPreviousPerformRequestsWithTarget:self selector:@selector(requestTimedOut) object:nil];
}

- (void)requestTimedOut
{
	[productsRequest cancel];
	if( self.pendingCompletionHandler != nil )
	{
		self.pendingCompletionHandler( @[] );
		self.pendingCompletionHandler = nil;
	}
}

// SKProductsRequestDelegate protocol method
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response
{
	[self cancelTimeout];
	if( self.pendingCompletionHandler != nil )
	{
		self.pendingCompletionHandler( response.products );
		self.pendingCompletionHandler = nil;
	}
}
@end

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
using namespace fr::licensing;

@interface LicensingController : NSObject <SKPaymentTransactionObserver>
- (void) updateVersion: (fr::licensing::AppPurchaseVersionUpdateType) updateType;
- (void) restorePurchases;
- (void) purchaseFeature: (NSString*) feature;
- (void) updateDoesUserOwnFeature: (NSString*) feature forceRefresh: (BOOL) forceRefresh;
@end

@implementation LicensingController

- (void) updateAppStoreReceipt: (void (^)( NSDictionary* receiptJson, NSError* error )) completionHandler
{
	dev_trace( "fetching receipt." );

	ReceiptFetcher* fetcher = [[ReceiptFetcher alloc] init];
	[fetcher updateReceipt: completionHandler];
}

- (void) updateVersion: (fr::licensing::AppPurchaseVersionUpdateType) updateType
{
	
#if DEV_MODE && 0		// TODO!!! licensing: Set to 0 to remove these lines for release.
	fr::licensing::eachDelegate( []( Delegate& delegate ) { delegate.onLicensingPurchaseVersionFound( Version( "2.0" ), {} ); } );
	return;
#endif
	
	// Has this been cached in preferences?
	//
	std::string preferenceVersion;
	if( updateType != fr::licensing::AppPurchaseVersionUpdateType::RefreshRequired
	    && fr::loadPreference( PREFERENCE_KEY_APP_VERSION, preferenceVersion ))
	{
		dev_trace( "updateVersion found preference with value: " << preferenceVersion );
		
		// Yes. Return that.
		//
		const auto version = Version{ preferenceVersion };
		fr::licensing::eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingPurchaseVersionFound( version, {} ); } );
	}
	else if( updateType != fr::licensing::AppPurchaseVersionUpdateType::RefreshProhibited )
	{
		dev_trace( "updateVersion found no preference value" );
		
		// No. Fetch it from the app store.
		//
		[self updateAppStoreReceipt: ^( NSDictionary* receiptJson, NSError* error )
		 {
			 NSString* version = nil;
			 if( receiptJson )
			 {
				 NSLog(@"receiptJson: %@", receiptJson);
				 version = receiptJson[@"receipt"][@"original_application_version"];
				 
				 NSLog( @"Found original app purchase version '%@'", version );
				 
				 // Cache the value to preferences.
				 //
				 if( version != nil && [version length] > 0 )
				 {
					 fr::savePreference( PREFERENCE_KEY_APP_VERSION, convert( version ));
				 }
			 }
			 
			 const auto cppVersion = Version{ convert( version ) };
			 const Error cppError = convertError( error );
			 fr::licensing::eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingPurchaseVersionFound( cppVersion, cppError ); } );
		 }];
	}
	else
	{
		// Can't find version locally, but not allowed to refresh it from the server. Version remains unknown.
		//
		ASSERT( updateType == fr::licensing::AppPurchaseVersionUpdateType::RefreshProhibited );
		fr::licensing::eachDelegate( [&]( Delegate& delegate )
									{
										delegate.onLicensingPurchaseVersionFound( Version{},
																				  Error{ "App purchase version unknown", 7 } ); } );
	}
}

- (void) restorePurchases
{
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
 }

- (void) updateDoesUserOwnFeature: (NSString*) feature forceRefresh: (BOOL) forceRefresh
{
	const auto cppFeature = convert( feature );

	// Has this been cached in preferences?
	//
	std::string featureOwnership;
	const bool hadValue = fr::loadPreference( PREFERENCE_PREFIX_FEATURE_PURCHASE + convert( feature ), featureOwnership );
	if( !forceRefresh && hadValue && featureOwnership == "true" )
	{
		// Yes. Return that.
		//
		const auto purchaseState = featureOwnership == "true" ? fr::licensing::PurchaseState::Purchased : fr::licensing::PurchaseState::Unpurchased;
		fr::licensing::eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingFeatureOwnershipFound( cppFeature, purchaseState, {} ); } );
	}
	else
	{
		// No. Fetch it from the app store.
		//
		[self updateAppStoreReceipt: ^( NSDictionary* receiptJson, NSError* error )
		 {
			 if( !receiptJson )
			 {
				 const Error cppError = convertError( error );
				 fr::licensing::eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingFeatureOwnershipFound( cppFeature, fr::licensing::PurchaseState::Unknown, cppError ); } );
				 return;
			 }
			 else
			 {
				 NSLog( @"receiptJson: %@", receiptJson );
				 NSArray* inAppPurchases = receiptJson[@"receipt"][@"in_app"];
				 
				 // Is this feature among the receipted past purchases?
				 bool hasPurchase = false;
				 for( id purchase in inAppPurchases )
				 {
					 if( [feature isEqualToString: purchase[ @"product_id" ]] )
					 {
						 hasPurchase = true;
						 break;
					 }
				 }

				 if( hasPurchase )
				 {
					 fr::savePreference( PREFERENCE_PREFIX_FEATURE_PURCHASE + convert( feature ), "true" );
				 }

				 fr::licensing::eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingFeatureOwnershipFound( cppFeature,
																									 hasPurchase ? fr::licensing::PurchaseState::Purchased : fr::licensing::PurchaseState::Unpurchased,
																									 {} ); } );
			 }
		 }];
	}
}

- (void) purchaseFeature: (NSString*) feature
{
	const auto cppFeature = convert( feature );

	dev_trace( "purchaseFeature: " << cppFeature );
	ProductListFetcher* fetcher = [[ProductListFetcher alloc] init];
	[fetcher updateProducts: @[feature] completion: ^(NSArray<SKProduct *>* products)
	 {
		 if( products == nil )
		 {
			 fr::licensing::eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingFeatureOwnershipFound( cppFeature,
																								 fr::licensing::PurchaseState::Unknown,
																								 Error{ "App Store contact timed out.", 5 } ); } );
		 }
		 else if( [products count] < 1 )
		 {
			 fr::licensing::eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingFeatureOwnershipFound( cppFeature,
																								 fr::licensing::PurchaseState::Unknown,
																								 Error{ "Could not find this product.", 6 } ); } );
		 }
		 else
		 {
			 SKProduct *product = products[ 0 ];
			 SKMutablePayment *payment = [SKMutablePayment paymentWithProduct:product];
			 payment.quantity = 1;
			 
			 [[SKPaymentQueue defaultQueue] addPayment:payment];
		 }
	 } ];
}

- (void)paymentQueue:(SKPaymentQueue *)queue updatedTransactions:(NSArray *)transactions
{
	for (SKPaymentTransaction* transaction in transactions)
	{
		switch( transaction.transactionState )
		{
			case SKPaymentTransactionStatePurchasing:
				// Do nothing?
				break;
			case SKPaymentTransactionStateDeferred:
				// Do nothing?
				// TODO!!! Just to clear out my own queue, dang it!
//				[queue finishTransaction:transaction];
				break;
			case SKPaymentTransactionStateFailed:
				[self failedTransaction:transaction];
				[queue finishTransaction:transaction];
				break;
			case SKPaymentTransactionStatePurchased:
				[self completeTransaction:transaction];
				[queue finishTransaction:transaction];
				break;
			case SKPaymentTransactionStateRestored:
				[self restoreTransaction:transaction];
				[queue finishTransaction:transaction];
				break;
			default:
				NSLog( @"Unexpected transaction state %@", @( transaction.transactionState ));
				break;
		}
	}
}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue *)queue
{
	fr::licensing::eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingRestorePurchasesFinished( Error{} ); } );
}

- (void)paymentQueue:(SKPaymentQueue *)queue restoreCompletedTransactionsFailedWithError:(NSError *)error
{
	const auto cppError = convertError( error );
	fr::licensing::eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingRestorePurchasesFinished( cppError ); } );
}

-(void) failedTransaction:(SKPaymentTransaction*) transaction
{
	const auto cppFeature = convert( transaction.payment.productIdentifier );
	const auto cppError = convertError( transaction.error );
	NSLog( @"Failed transaction for feature %@", transaction.payment.productIdentifier );
	
	fr::licensing::eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingFeatureOwnershipFound( cppFeature,
																						 fr::licensing::PurchaseState::Unknown,
																						 cppError ); } );
}

-(void) completeTransaction:(SKPaymentTransaction*) transaction
{
	const auto cppFeature = convert( transaction.payment.productIdentifier );
	NSLog( @"Completed transaction for feature %@", transaction.payment.productIdentifier );

	fr::savePreference( PREFERENCE_PREFIX_FEATURE_PURCHASE + cppFeature, "true" );
	fr::licensing::eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingFeatureOwnershipFound( cppFeature,
																						fr::licensing::PurchaseState::Purchased,
																						Error{} ); } );
}

-(void) restoreTransaction:(SKPaymentTransaction*) transaction
{
	NSLog( @"Completed transaction for feature %@", transaction.payment.productIdentifier );
	[self completeTransaction:transaction];
}

@end

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

namespace
{
	LicensingController* g_licensingController;
}

namespace fr
{
	namespace licensing
	{
		bool isPurchasingSupported()
		{
			return true;
		}
		
		void addDelegate_platform( Delegate* delegate )
		{
			REQUIRES( delegate );
            
			// Do other initialization (one time only).
			//
			if( g_licensingController == nil )
			{
				g_licensingController = [[LicensingController alloc] init];
				
				[[SKPaymentQueue defaultQueue] addTransactionObserver: g_licensingController];
			}
		}
        
        void removeDelegate_platform( Delegate* delegate )
        {}
		
		void updateAppPurchaseVersion( AppPurchaseVersionUpdateType updateType )
		{
			dev_trace( "updating app purchase version." );
			[g_licensingController updateVersion: updateType];
		}
		
		void restorePurchases()
		{
			dev_trace( "Restoring purchases" );
			[g_licensingController restorePurchases];
		}
		
		void updateDoesUserOwnFeature( const std::string& featureName, bool forceRefresh )
		{
			dev_trace( "updateDoesUserOwnFeature: " << featureName );
			[g_licensingController updateDoesUserOwnFeature: convert( featureName ) forceRefresh: forceRefresh];
		}
		
		void purchaseFeature( const std::string& featureName )
		{
			dev_trace( "Purchasing feature: " << featureName );
			[g_licensingController purchaseFeature: convert( featureName )];
		}
	}
}
