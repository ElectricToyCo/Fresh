//
//  Essentials.h
//  freshshortname
//
//  Created by Jeff Wofford on 2013/11/19.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#ifndef freshshortname_Essentials_h
#define freshshortname_Essentials_h

#include "FreshVector.h"
#include "Color.h"
#include "Object.h"
#include "ObjectPtr.h"
#include "CommandProcessor.h"
#include "MovieClip.h"
#include "ClassFilter.h"
#include "FreshFile.h"
#include "DisplayPackage.h"
#include "AudioSystem.h"
#include "StructSerialization.h"

#if FRESH_TELEMETRY_ENABLED
#	include "UserTelemetry.h"
#endif

namespace fr
{
	class Sound;
}

namespace fresh_namespace
{
	using fr::ClassInfo;
	using fr::ClassName;
	using fr::path;
	using fr::real;
	using fr::uint;
	using fr::vec2;
	using fr::vec3;
	using fr::vec4;
	typedef fr::Vector2i vec2i;
	typedef fr::Vector2ui vec2ui;
	using fr::mat4;
	using fr::Color;
	using fr::SmartPtr;
	using fr::WeakPtr;
	using fr::TimeType;
	using fr::Range;
	using fr::rect;
	using fr::Rectanglei;
	using fr::angle;
	using fr::clamp;
	using fr::lerp;
	using fr::ObjectId;
	using fr::ClassFilter;
	using fr::dynamic_freshptr_cast;
}

#endif
