//
//  FreshSocial.h
//  Fresh
//
//  Created by Jeff Wofford on 10/29/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef Fresh_FreshSocial_h
#define Fresh_FreshSocial_h

#include <string>
#include <memory>
#include "Property.h"

namespace fr
{
	class SocialImpl;
	class Social
	{
	public:
		
		enum Service
		{
			Facebook,
			Twitter
		};
		
		Social();
		~Social();
		
		bool isAvailable( Service service ) const;
		void proposePost( Service service, const std::string& proposedText );
		void proposePost( Service service, const std::string& proposedText,
						  const std::vector< unsigned int >& imagePixels,
						  unsigned int imageWidth,
						  unsigned int imageHeight );
		
	private:
		
		std::unique_ptr< SocialImpl > m_impl;
		
	};

	FRESH_ENUM_STREAM_IN_BEGIN( Social, Service )
	FRESH_ENUM_STREAM_IN_CASE( Social::Service, Facebook )
	FRESH_ENUM_STREAM_IN_CASE( Social::Service, Twitter )
	FRESH_ENUM_STREAM_IN_END()
	
	FRESH_ENUM_STREAM_OUT_BEGIN( Social, Service )
	FRESH_ENUM_STREAM_OUT_CASE( Social::Service, Facebook )
	FRESH_ENUM_STREAM_OUT_CASE( Social::Service, Twitter )
	FRESH_ENUM_STREAM_OUT_END()

}

#endif
