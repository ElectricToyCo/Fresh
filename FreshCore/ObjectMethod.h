/*
 *  ObjectMethod.h
 *  Fresh
 *
 *  Created by Jeff Wofford on 12/28/10.
 *  Copyright 2010 jeffwofford.com. All rights reserved.
 *
 */

#ifndef FRESH_OBJECT_METHOD_H_INCLUDED
#define FRESH_OBJECT_METHOD_H_INCLUDED

#include "ObjectMethodAbstract.h"
#include "StreamedFunctionCaller.h"
#include "FreshDebug.h"
#include <functional>
#include <memory>

namespace fr
{
	
	template< typename ObjectT, typename return_t, typename... Args >
	class StreamedMethod : public StreamedMethodAbstract
	{
	public:
		
		typedef return_t (func_type)( Args... );
		
		template< typename fn_t >
		explicit StreamedMethod( fn_t&& fn )
		:	m_method( std::forward< fn_t >( fn ))
		{}
		
		virtual std::string operator()( Object* object, std::istream& in ) const override
		{
			ObjectT* castObject = dynamic_cast< ObjectT* >( object );
			if( castObject )
			{
				return m_method( castObject, in );
			}
			else
			{
				dev_warning( "StreamedMethod could not upcast object to the desired type." );
				return std::string();
			}
		}

	private:
		
		stream_method< ObjectT, return_t( Args... ) > m_method;
	};
	
	template< typename ObjectT, typename return_t, typename... Args >
	std::unique_ptr< StreamedMethodAbstract > makeStreamedMethod( return_t (ObjectT::*fn)( Args... ))
	{
		return std::unique_ptr< StreamedMethodAbstract >( new StreamedMethod< ObjectT, return_t, Args... >( std::mem_fn( fn )));
	}
	
	/////////////////////////////////////////////////////
	
	template< typename ReturnT >
	class SimpleAccessor : public SimpleAccessorAbstract, std::function< ReturnT( Object* ) >
	{
	public:
		virtual ReturnT operator()( const Object* object ) const = 0;
	};
	
	template< typename ObjectT, typename ReturnT >
	class SimpleAccessorClassSpecific : public SimpleAccessor< ReturnT >
	{
	public:
		
		typedef ReturnT (ObjectT::*MethodType)() const;
		
		SimpleAccessorClassSpecific( MethodType method )
		:	m_method( method )
		{
			// The REQUIRES check below is commented out for a reason.
			// The spirit of the law is that the argument (method) passed to 
			// this constructor should be the address of a real class method.
			// It should not, in that sense, be "null."
			// However--interesting story--it turns out that C++ (or at least
			// Clang/LLVM) will sometimes use a value of 0 as a legitimate
			// function address. Specifically, virtual functions are sometimes
			// referenced via their index in the virtual function table. Since
			// this index can be 0, method can legally have the numeric
			// value 0. Unfortunately this means that accidentally passing
			// "null" here cannot be trapped via a REQUIRES check.
			//
			// REQUIRES( method );
		}
		
		virtual ReturnT operator()( const Object* object ) const override
		{
			REQUIRES( object );
			const ObjectT* objectOfRequiredClass = dynamic_cast< const ObjectT* >( object );
			ASSERT( objectOfRequiredClass );
			
			return (*objectOfRequiredClass.*m_method)();
		}
		
	private:
		
		MethodType m_method;
	};
	
}

#define DEFINE_METHOD( varHost, methodName ) 	\
	struct MethodInit_##varHost##_##methodName	\
	{	\
		MethodInit_##varHost##_##methodName()	\
		{	\
			varHost::StaticGetClassInfo().addMethod( #methodName, fr::makeStreamedMethod< varHost >( &varHost::methodName ));	\
		}	\
	} g_methodInit_##varHost##_##methodName##_;

#define DEFINE_METHOD_OVERLOAD( varHost, methodName, override ) 	\
	struct MethodInit_##varHost##_##methodName	\
	{	\
		MethodInit_##varHost##_##methodName()	\
		{	\
			varHost::StaticGetClassInfo().addMethod( #methodName, fr::makeStreamedMethod< varHost >( override ));	\
		}	\
	} g_methodInit_##varHost##_##methodName##_;


#define DECLARE_ACCESSOR( varHost, accessorReturnType, accessorName ) 	\
	static struct AccessorInit_##varHost##_##accessorName	\
	{	\
		AccessorInit_##varHost##_##accessorName()	\
		{	\
			varHost::StaticGetClassInfo().addAccessor( #accessorName, std::unique_ptr< fr::SimpleAccessorAbstract >( new fr::SimpleAccessorClassSpecific< varHost, accessorReturnType >( &varHost::accessorName ) ));	\
		}	\
	} accessorInit_##varHost##_##accessorName##_;

#define DEFINE_ACCESSOR( varHost, accessorReturnType, accessorName ) 	\
	struct varHost::AccessorInit_##varHost##_##accessorName varHost::accessorInit_##varHost##_##accessorName##_;

#endif
