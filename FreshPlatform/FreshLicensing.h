//
//  FreshLicensing.h
//  Fresh
//
//  Created by Jeff Wofford on 11/20/16.
//
//

#ifndef FreshLicensing_h
#define FreshLicensing_h

#include <string>
#include <vector>
#include <functional>

namespace fr
{
	namespace licensing
	{
		struct Version
		{
		public:
			
			Version();
			explicit Version( const std::string& versionString );

			// Comparable iff original string is a series of one or more nonnegative integers separated by dots.
			bool comparable() const;
			const std::string& string() const;
			
			bool operator==( const Version& other ) const;
			bool operator!=( const Version& other ) const;

			bool operator<( const Version& other ) const;
			bool operator<=( const Version& other ) const;

			bool operator>( const Version& other ) const;
			bool operator>=( const Version& other ) const;
			
		private:
			
			const std::string m_versionString;
			std::vector< unsigned int > m_versionNumbers;
		};
		
        //////////////////////////////////////////////////////////////////////////////////

		class Error
		{
		public:
			Error();
			explicit Error( const std::string& message, int code );
			bool isError() const;
			int code() const { return m_code; }
			const std::string& message() const { return m_message; }
			
		private:
			int m_code;
			std::string m_message;
		};
        
        //////////////////////////////////////////////////////////////////////////////////
		
        enum class PurchaseState
        {
            Unknown,
            Purchased,
            Unpurchased
        };
        
		class Delegate
		{
		public:
			virtual void onLicensingPurchaseVersionFound( Version version, fr::licensing::Error error ) {};
			virtual void onLicensingRestorePurchasesFinished( Error error ) {};
			virtual void onLicensingFeaturePurchaseCompleted( const std::string& feature, PurchaseState purchaseState, Error error ) {};
			virtual void onLicensingFeatureOwnershipFound( const std::string& feature, PurchaseState purchaseState, Error error ) {};
			virtual ~Delegate();
		};
		
		bool isPurchasingSupported();
		
		void addDelegate( Delegate* delegate );
		void removeDelegate( Delegate* delegate );
        void eachDelegate( std::function< void( Delegate& ) >&& fn );
		
		enum class AppPurchaseVersionUpdateType
		{
			RefreshRequired,
			RefreshAllowed,
			RefreshProhibited,
		};
		
		void updateAppPurchaseVersion( AppPurchaseVersionUpdateType updateType = AppPurchaseVersionUpdateType::RefreshAllowed );
		void restorePurchases();
		
		// Don't call this willy-nilly. May prompt the user to enter their iTunes credentials, which can be unnerving.
		void updateDoesUserOwnFeature( const std::string& featureName, bool forceRefresh = false );
				
		void purchaseFeature( const std::string& featureName );
        
        // INTERNALS //////////////////////////////////////////////////////////////////////
        void addDelegate_platform( Delegate* delegate );
        void removeDelegate_platform( Delegate* delegate );
	}
}

#endif /* FreshLicensing_h */
