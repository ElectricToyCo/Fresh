//
//  ObjectSingleton.h
//  Fresh
//
//  Created by Jeff Wofford on 2/25/13.
//
//

#ifndef Fresh_ObjectSingleton_h
#define Fresh_ObjectSingleton_h

#include "Singleton.h"
#include "SmartPtr.h"

namespace fr
{
	template< class ObjectT >
	class ObjectSingleton : public Singleton< ObjectT, SmartPtr< ObjectT > >
	{
	public:
		
		virtual ~ObjectSingleton()
		{
			// Do nothing.
			
			// EXPLANATION:
			// A singleton normally assigns nullptr to s_instance on
			// destruction. But an ObjectSingleton's pointer must already
			// be nullptr if the destructor is being called. Assigning nullptr here
			// will cause the s_instance SmartPtr to be dereferenced twice,
			// causing potentially assertion failures and double-deletions.
		}
		
		static void destroy()
		{
			Singleton< ObjectT, SmartPtr< ObjectT > >::s_instance = nullptr;
		}
		
	protected:
		ALWAYS_INLINE ObjectSingleton( ObjectT* instance )
		:	Singleton< ObjectT, SmartPtr< ObjectT > >( instance )
		{}
		
	};
	
}

#endif
