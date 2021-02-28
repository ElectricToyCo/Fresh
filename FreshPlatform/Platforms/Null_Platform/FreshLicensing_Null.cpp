//
//  FreshLicensing_Null.cpp
//  Fresh
//
//  Created by Jeff Wofford on 11/20/16.
//
//

#include "FreshLicensing.h"

namespace fr
{
	namespace licensing
	{
        void addDelegate_platform( Delegate* delegate ) {}
        void removeDelegate_platform( Delegate* delegate ) {}

        bool isPurchasingSupported()
		{
			return false;
		}

		void updateAppPurchaseVersion( AppPurchaseVersionUpdateType updateType )
		{
			eachDelegate( []( Delegate& delegate ) { delegate.onLicensingPurchaseVersionFound( Version{}, {} ); } );
		}

		void restorePurchases()
		{
			eachDelegate( []( Delegate& delegate ) { delegate.onLicensingRestorePurchasesFinished( Error{ "Restoring purchases is unsupported on this platform", 1 } ); } );
		}
		
		void updateDoesUserOwnFeature( const std::string& featureName, bool forceRefresh )
		{
			// Always purchased in null implementation.
			eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingFeatureOwnershipFound( featureName, PurchaseState::Unpurchased, {} ); } );
		}
				
		void purchaseFeature( const std::string& featureName )
		{
			eachDelegate( [&]( Delegate& delegate ) { delegate.onLicensingFeaturePurchaseCompleted( featureName, PurchaseState::Unpurchased, Error{ "Purchasing is unsupported on this platform", 2 } ); } );
		}
	}
}
