/*
 *  ObjectMethodAbstract.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 12/28/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_OBJECT_METHOD_ABSTRACT_H_INCLUDED
#define FRESH_OBJECT_METHOD_ABSTRACT_H_INCLUDED

#include <iostream>
#include <string>

namespace fr
{
	class Object;
	
	class StreamedMethodAbstract
	{
	public:
		virtual ~StreamedMethodAbstract() {}
		virtual std::string operator()( Object* object, std::istream& in ) const = 0;
	};
	
	class SimpleAccessorAbstract
	{
	public:
		virtual ~SimpleAccessorAbstract() {}
	};
	
	class SimpleMutatorAbstract
	{
	public:
		virtual ~SimpleMutatorAbstract() {}
	};
}

#endif

