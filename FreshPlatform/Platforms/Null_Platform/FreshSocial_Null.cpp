//
//  FreshSocial_Null.cpp
//  Fresh
//
//  Created by Jeff Wofford on 10/29/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "../FreshSocial.h"

namespace fr
{
	
	class SocialImpl {};

	Social::Social()
	{}
	
	Social::~Social()
	{}
	
	bool Social::isAvailable( Service service ) const
	{
		return false;
	}

	void Social::proposePost( Service service, const std::string& proposedText )
	{}

	void Social::proposePost( Service service, const std::string& proposedText,
							 const std::vector< unsigned int >& imagePixels,
							 unsigned int imageWidth,
							 unsigned int imageHeight )
	{}

}

