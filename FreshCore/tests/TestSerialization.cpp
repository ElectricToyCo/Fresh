#include "ObjectManager.h"
#include "ObjectStreamFormatter.h"
#include "FreshFile.h"
#include "FreshTest.h"
#include "StructSerialization.h"
using namespace Fresh;
using namespace std;

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Test classes and functions
////////////////////////////////////////////////////////////////////////////////////////////////////////////

class Simple : public Object
{
	FRESH_DECLARE_CLASS( Simple, Object )
public:
	
	DVAR( Simple, int, m_i, 0 );
	DVAR( Simple, float, m_f, 0 );
	VAR( Simple, std::string, m_s );
};

FRESH_DEFINE_CLASS( Simple )
DEFINE_VAR( Simple, int, m_i )
DEFINE_VAR( Simple, float, m_f );
DEFINE_VAR( Simple, std::string, m_s );
FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( Simple )

class Containing : public Object
{
	FRESH_DECLARE_CLASS( Containing, Object )
public:
	
	typedef std::map< int, std::string > MapIntsToStrings;
	
	VAR( Containing, std::vector< float >, m_vecFloats );
	VAR( Containing, std::list< std::string >, m_listStrings );
	VAR( Containing, std::list< std::vector< int > >, m_listVecInts );
	VAR( Containing, MapIntsToStrings, m_mapIntsToStrings );
};

FRESH_DEFINE_CLASS( Containing )
DEFINE_VAR( Containing, std::vector< float >, m_vecFloats );
DEFINE_VAR( Containing, std::list< std::string >, m_listStrings );
DEFINE_VAR( Containing, std::list< std::vector< int > >, m_listVecInts );
DEFINE_VAR( Containing, MapIntsToStrings, m_mapIntsToStrings );
FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( Containing )

class ContainingStructs : public Object
{
	FRESH_DECLARE_CLASS( ContainingStructs, Object )
public:
	
	struct Inner : public SerializableStruct< Inner >
	{
		int x;
		std::string s;
		
		Inner()
		{
			STRUCT_BEGIN_PROPERTIES
			STRUCT_ADD_PROPERTY( x );
			STRUCT_ADD_PROPERTY( s );
			STRUCT_END_PROPERTIES
		}
	};
	
	typedef std::map< float, Inner > Inners;
	
	VAR( ContainingStructs, Inners, m_mapFloatsToInners );
	
};

STRUCT_DEFINE_SERIALIZATION_OPERATORS( ContainingStructs::Inner )

FRESH_DEFINE_CLASS( ContainingStructs )
DEFINE_VAR( ContainingStructs, Inners, m_mapFloatsToInners );
FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( ContainingStructs )

class Referential : public Object
{
	FRESH_DECLARE_CLASS( Referential, Object )
public:
	
	VAR( Referential, Referential::ptr, m_strong );
	VAR( Referential, Referential::wptr, m_weak );
	DVAR( Referential, Referential*, m_raw, nullptr );
	
};

FRESH_DEFINE_CLASS( Referential )
DEFINE_VAR( Referential, Referential::ptr, m_strong );
DEFINE_VAR( Referential, Referential::wptr, m_weak );
DEFINE_VAR( Referential, Referential*, m_raw );
FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( Referential )


bool loadObjects()
{
	ObjectManager& objectManager = ObjectManager::instance();
	
	// Test loading simple types.
	{
		int iValue = 15316;
		float fValue = 16.584f;
		std::string sValue = "  Hello\n   World  ";
		
		std::ostringstream xmlText;
		xmlText << "<object class='Simple' name='simple1'>"
				<< "	<i>" << iValue << "</i>"
				<< "	<f>" << fValue << "</f>"
				<< "	<s><![CDATA[" << sValue << "]]></s>"	// CDATA needed to prevent XML reader from condensing whitespace and such.
				<< "</object>";
		
		
		auto xmlElement = stringToXmlElement( xmlText.str() );
		VERIFY_BOOL( xmlElement );
		
		Simple::ptr object = dynamic_freshptr_cast< Simple::ptr >( objectManager.createObject( *xmlElement ));
		
		VERIFY_BOOL( object );
		VERIFY_BOOL( object->hasName( "simple1" ));
		VERIFY_BOOL( object->m_i == iValue );
		VERIFY_BOOL( object->m_f == fValue );
		VERIFY_BOOL( object->m_s == sValue );
	}

	// Test malformed object XML: text child nodes.
	{
		std::ostringstream xmlText;
		xmlText << "<object class='Simple' name='simple1'>Me bum.<i>4</i></object>";		// Object element contained substantive non-element children.
		
		auto xmlElement = stringToXmlElement( xmlText.str() );
		VERIFY_BOOL( xmlElement );

		DevLog::resetErrorAndWarningCount();
		dev_trace( "TEST: The following warning is expected and correct:" );
		Simple::ptr object = dynamic_freshptr_cast< Simple::ptr >( objectManager.createObject( *xmlElement ));
		
		VERIFY_BOOL_MSG( DevLog::numWarnings() == 1, "Log failed to report XML formation problem." );
	}
	
	// Test malformed object XML: spurious attributes.
	{
		std::ostringstream xmlText;
		xmlText << "<object class='Simple' name='simple1' blame='me'></object>";		// Extra attribute
		
		auto xmlElement = stringToXmlElement( xmlText.str() );
		VERIFY_BOOL( xmlElement );

		DevLog::resetErrorAndWarningCount();
		dev_trace( "TEST: The following warning is expected and correct:" );
		Simple::ptr object = dynamic_freshptr_cast< Simple::ptr >( objectManager.createObject( *xmlElement ));

		VERIFY_BOOL_MSG( DevLog::numWarnings() == 1, "Log failed to report XML formation problem." );
	}

	// Test ERRONEOUS loading of simple types.
	{
		std::ostringstream xmlText;
		xmlText << "<object class='Simple' name='simple1'><i>Herbert</i></object>";		// Not an int.
		
		auto xmlElement = stringToXmlElement( xmlText.str() );
		VERIFY_BOOL( xmlElement );
		
		DevLog::resetErrorAndWarningCount();
		dev_trace( "TEST: The following error is expected and correct:" );
		Simple::ptr object = dynamic_freshptr_cast< Simple::ptr >( objectManager.createObject( *xmlElement ));
		
		VERIFY_BOOL_MSG( DevLog::numErrors() == 1, "Log failed to report XML formation problem." );
	}

	// Test loading containers.
	{		
		std::ostringstream xmlText;
		xmlText << "<object class='Containing' name='containing1'>"
		<< "	<vecFloats>4, 5.1, 6,    2.4</vecFloats>"
		<< "	<listStrings></listStrings>"
		<< "	<listVecInts>[[ 1, 2 ],	\
							  [ 3, 4, 5 ],	\
							  ]</listVecInts>"
		<< "	<mapIntsToStrings>[	\
							 {100 = \"Buttercup\"}	\
							 {   200=\"Yabba Dabba Do!!!\"}]	\
				</mapIntsToStrings>"
		<< "</object>";
		
		auto xmlElement = stringToXmlElement( xmlText.str() );
		VERIFY_BOOL( xmlElement );
		
		Containing::ptr object = dynamic_freshptr_cast< Containing::ptr >( objectManager.createObject( *xmlElement ));
		
		VERIFY_BOOL( object );
		VERIFY_BOOL( object->hasName( "containing1" ));
		VERIFY_BOOL( object->m_vecFloats.size() == 4 );
		VERIFY_BOOL( object->m_vecFloats[ 0 ] == 4.0f );
		VERIFY_BOOL( object->m_vecFloats[ 1 ] == 5.1f );
		VERIFY_BOOL( object->m_vecFloats[ 2 ] == 6.0f );
		VERIFY_BOOL( object->m_vecFloats[ 3 ] == 2.4f );
		VERIFY_BOOL( object->m_listStrings.empty() );
		VERIFY_BOOL( object->m_listVecInts.size() == 2 );
		VERIFY_BOOL( object->m_listVecInts.front().size() == 2 );
		VERIFY_BOOL( object->m_listVecInts.front()[ 0 ] == 1 );
		VERIFY_BOOL( object->m_listVecInts.front()[ 1 ] == 2 );
		VERIFY_BOOL( object->m_listVecInts.back().size() == 3 );
		VERIFY_BOOL( object->m_listVecInts.back()[ 0 ] == 3 );
		VERIFY_BOOL( object->m_listVecInts.back()[ 1 ] == 4 );
		VERIFY_BOOL( object->m_listVecInts.back()[ 2 ] == 5 );
		VERIFY_BOOL( object->m_mapIntsToStrings.size() == 2 );
		VERIFY_BOOL( object->m_mapIntsToStrings[ 100 ] == "Buttercup" );
		VERIFY_BOOL( object->m_mapIntsToStrings[ 200 ] == "Yabba Dabba Do!!!" );
	}
	
	// Test simple structs.
	{
		std::string structText( "{ x = 141, s = \"hello world\" }" );
		Destringifier destringifier( structText );
		
		ContainingStructs::Inner inner;
		inner.read( destringifier );
		
		VERIFY_BOOL( inner.x == 141 );
		VERIFY_BOOL( inner.s == "hello world" );
	}
	
	// Test loading structs.
	{
		std::ostringstream xmlText;
		xmlText << "<object class='ContainingStructs' name='structs1'>	\
			<mapFloatsToInners>	\
				[	\
					{32.25 = {x= -1999, 	\
							  s= \"My ape pretender\" }	\
					}, \
					{-16.5=  { x= 5 , 	\
							  s = blammo }	\
					}, \
				]	\
			</mapFloatsToInners>\
		</object>";
		
		auto xmlElement = stringToXmlElement( xmlText.str() );
		VERIFY_BOOL( xmlElement );
		
		ContainingStructs::ptr object = dynamic_freshptr_cast< ContainingStructs::ptr >( objectManager.createObject( *xmlElement ));

		VERIFY_BOOL( object );
		VERIFY_BOOL( object->hasName( "structs1" ));
		VERIFY_BOOL( object->m_mapFloatsToInners.size() == 2 );
		
		{
			const ContainingStructs::Inner& inner = object->m_mapFloatsToInners[ -16.5f ];
			VERIFY_BOOL( inner.x == 5 );
			VERIFY_BOOL( inner.s == "blammo" );
		}
		{
			const ContainingStructs::Inner& inner = object->m_mapFloatsToInners[ 32.25f ];
			VERIFY_BOOL( inner.x == -1999 );
			VERIFY_BOOL( inner.s == "My ape pretender" );
		}
	}
	
	// Test loading pointers.
	{
		std::vector< Referential::ptr > referentials;
		
		for( int i = 0; i < 1000; ++i )
		{
			std::ostringstream xmlText;
			xmlText << "<object class='Referential'>";
			
			int pointerType = -1;
			ObjectId referent;
			
			if( !referentials.empty() )
			{
				size_t iRandReferent = rand() % referentials.size();
				referent = referentials[ iRandReferent ]->getObjectId();
				
				pointerType = rand() % 3;
				switch( pointerType )
				{
					default:
					case 0:
						xmlText << "<strong>" << referent << "</strong>";
						break;
					case 1:
						xmlText << "<weak>" << referent << "</weak>";
						break;
					case 2:
						xmlText << "<raw>" << referent << "</raw>";
						break;
				}
			}
			
			xmlText << "</object>";

			auto xmlElement = stringToXmlElement( xmlText.str() );
			VERIFY_BOOL( xmlElement );
			
			Referential::ptr referential = dynamic_freshptr_cast< Referential::ptr >( objectManager.createObject( *xmlElement ));
			
			VERIFY_BOOL( referential );
			VERIFY_BOOL( referential->m_strong->getObjectId() == ( pointerType == 0 ? referent : ObjectId::NULL_OBJECT ));
			VERIFY_BOOL( referential->m_weak->getObjectId() == ( pointerType == 1 ? referent : ObjectId::NULL_OBJECT ) );
			VERIFY_BOOL( referential->m_raw->getObjectId() == ( pointerType == 2 ? referent : ObjectId::NULL_OBJECT ) );
			
			referentials.push_back( referential );
		}
	}
	
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

int main( int argc, char * const argv[] )
{
	{
		try
		{
			// Initial setup.
			//
			Object::useTimeCodedDefaultNames( false );		// Use predictable object names so that test results are
															// consistent from run to run.
			ObjectManager objectManager;
			
			// Run tests.
			//
			VERIFY_TEST( loadObjects() );
		}
		catch( ... )
		{
			cerr << "Caught unknown exception.\n";
			return 1;
		}
	}

	cout << "ALL TESTS PASSED.\n";

	return 0;
}
