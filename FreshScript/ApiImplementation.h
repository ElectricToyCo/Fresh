//
//  ApiImplementation.h
//  Fresh
//
//  Created by Jeff Wofford on 7/22/18.
//

#ifndef ApiImplementation_h
#define ApiImplementation_h

#define LUACPP_HOST_TYPE fr::FantasyConsole
#define LUACPP_HOST_TYPE_PTR LUACPP_HOST_TYPE::wptr
#include "LuaCppInterop.hpp"

#define SANITIZE( arg, defaultValue, min, max ) DEFAULT( arg, (defaultValue)); arg = fr::clamp( arg, (min), (max) );
#define SANITIZE_WRAP( arg, defaultValue, min, max ) DEFAULT( arg, (defaultValue)); arg = fr::wrap( arg, (min), (max) );

#endif /* ApiImplementation_h */
