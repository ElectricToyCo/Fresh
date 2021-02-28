#ifndef FRESH_CORE_CPP_COMPATABILITY_H_INCLUDED
#define FRESH_CORE_CPP_COMPATABILITY_H_INCLUDED

#ifdef _MSC_VER
//#	define constexpr
//#	define noexcept

#	define FRESH_CURRENT_FUNCTION __FUNCTION__

#else
#	define FRESH_CURRENT_FUNCTION __func__
#endif

#endif
