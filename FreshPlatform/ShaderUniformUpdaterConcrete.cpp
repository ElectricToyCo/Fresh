/*
 *  ShaderUniformUpdaterConcrete.cpp
 *  Fresh
 *
 *  Created by Jeff Wofford on 12/22/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#include "ShaderUniformUpdaterConcrete.h"
#include "Objects.h"

namespace fr
{
	FRESH_DEFINE_CLASS_TEMPLATE_1( ShaderUniformUpdaterConcrete, int )
	FRESH_DEFINE_CLASS_TEMPLATE_1( ShaderUniformUpdaterConcrete, float )
	FRESH_DEFINE_CLASS_TEMPLATE_1( ShaderUniformUpdaterConcrete, Color )
	FRESH_DEFINE_CLASS_TEMPLATE_1( ShaderUniformUpdaterConcrete, vec2 )
	FRESH_DEFINE_CLASS_TEMPLATE_1( ShaderUniformUpdaterConcrete, vec3 )
	FRESH_DEFINE_CLASS_TEMPLATE_1( ShaderUniformUpdaterConcrete, vec4 )
	FRESH_DEFINE_CLASS_TEMPLATE_1( ShaderUniformUpdaterConcrete, mat4 )
}
