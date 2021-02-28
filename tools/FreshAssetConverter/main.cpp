
//
//  main.cpp
//  fac
//
//  Created by Jeff Wofford on 2/4/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "Object.h"
#include "Assets.h"
#include "FreshXML.h"
#include "FreshPath.h"
#include "AssetTransformTexture.h"
#include "FreshTime.h"
#include "FreshFile.h"
#include <tclap/CmdLine.h>
#include <iostream>
#include <fstream>
#include <copyfile.h>
using namespace fr;
using namespace std;


namespace fac
{
	class Transforms
	{
	public:
		
		explicit Transforms( const path& pathTransformFile, const path& rootSrcPath, const path& rootDestPath = "." )
		{
			if( !pathTransformFile.empty() )
			{
				load( pathTransformFile, rootSrcPath, rootDestPath );
			}
		}
		
		void load( const path& pathTransformFile, const path& rootSrcPath, const path& rootDestPath = "." )
		{
			ASSERT( rootSrcPath.empty() == false );
						
			XmlDocument doc;
			const XmlElement* rootElement = loadXmlDocument( pathTransformFile.string().c_str(), doc );
			if( !rootElement )
			{
				cerr << "FATAL: Could not load XML transform file '" << pathTransformFile << "'.\n";
				exit( 1 );
			}
			
			if( rootElement->ValueStr() != "asset-transforms" )
			{
				cerr << "WARNING: File '" << pathTransformFile << "' had a root element '" << rootElement->ValueStr() << "' rather than the expected 'asset-transforms'.\n";
			}
			
			m_rootSrcPath = rootSrcPath;
			m_rootDestPath = rootDestPath;
			
			iterateChildElements( *rootElement );
		}
		
		void report()
		{
			cout << "Transforms:\n";
			for( const auto& transform : m_assetTransforms )
			{
				cout << "\t" << *transform << endl;
			}
		}
		
		void verify()
		{
			for( const auto& transform : m_assetTransforms )
			{
				if( transform->isApplicable() == false )
				{
					cerr << "\ttransform for asset " << transform->assetName() << " cannot be applied (possibly missing source file \"" << transform->fullPathSrc() << "\" ).\n";
				}
			}
		}
		
		void apply( const std::string& databasePath, const std::time_t& databaseModifiedTime, bool overrideNewerDest )
		{
			std::ofstream databaseFile;
			auto tempFilePath = createTempFile( databaseFile );
			ASSERT( databaseFile.good() );
			
			// Write the XML header.
			//
			databaseFile << R"(<?xml version="1.0" encoding="UTF-8" ?>)";
			
			// Write the update time.
			//
			databaseFile << "\n<!-- FAC Converted Assets. Updated " << getStandardTimeDisplay( std::time( NULL )) << " -->\n\n";
			
			// Write the assets.
			//
			databaseFile << "<objects>\n";
			
			std::time_t oldestTransformTime = time( NULL );	// Now. Can't be newer than that, right?
			
			AssetPackage::ptr assets = getPackage< AssetPackage >( "assets" );
			ASSERT( assets );
			
			for( const auto& transform : m_assetTransforms )
			{
				if( transform->isApplicable() )
				{
					if( overrideNewerDest || transform->isSrcNewer()
	//					 || ( databaseModifiedTime == 0 || databaseModifiedTime > transform->destModifiedTime() )
						 )
					{
						cout << "\tConverting " << transform->fullPathSrc().string() << " to " << transform->fullPathDest().string() << " as " << transform->assetName() << endl;
						bool succeeded = transform->apply();
						
						if( succeeded )
						{
							oldestTransformTime = std::min( oldestTransformTime, transform->destModifiedTime() );
							
							transform->writeAssetToDatabase( databaseFile );
							continue;
						}
						else
						{
							cout << "\t" << transform->assetName() << " had problems. ";
						}
					}
					else
					{
						cout << "\t" << transform->assetName() << " up-to-date. ";
					}
				}
				else
				{
					cout << "\t" << transform->assetName() << " not applicable (missing source?). ";
				}
				
				// Write out the old asset loader information for this asset.
				//
				auto loader = assets->getLoader( transform->loaderName() );
				
				if( loader )
				{
					cout << "\tExporting from previous database.\n";
					
					Stringifier stringifier( databaseFile );
					ObjectStreamFormatterXml formatter( stringifier );
					formatter.indent( 1 );
					
					loader->serialize( formatter );
				}
				else
				{
					cerr << "Expunging due to no loader of name '" << transform->loaderName() << "' found.\n";
				}

			}
			databaseFile << "</objects>\n";
			
			// Make sure that the export (dest) database file has a file time no newer than the oldest asset file.
			//
			databaseFile.close();
			
			// Copy the completed temp file onto the actual databasePath.
			//
			::copyfile( tempFilePath.c_str(), databasePath.c_str(), nullptr, COPYFILE_ALL | COPYFILE_MOVE );
			
			setFileLastModifiedTime( databasePath, oldestTransformTime );
		}
		
	protected:
		
		static ClassName getTransformClassForElementName( const std::string& name )
		{
			if( name == "texture" )
			{
				return "AssetTransformTexture";
			}
			else if( name == "audio" )
			{
				return "AssetTransform";		// TODO
			}
			else
			{
				return "";
			}
		}
		
		void iterateChildElements( const XmlElement& baseElement )
		{
			for( ElementIterator iter( baseElement ); iter != ElementIterator(); ++iter )
			{
				const auto& child = *iter;
				const std::string childName = child.ValueStr();
				if( childName == "texture" || childName == "audio" )
				{
					ClassName transformClass( getTransformClassForElementName( childName ));
					
					// Allow the node to specify its asset name
					//
					Object::Name assetName;
					const char* szAssetName = child.Attribute( "name" );
					if( szAssetName )
					{
						assetName = szAssetName;
					}
					
					// Allow the node to specify its type.
					//
					const char* szClassName = child.Attribute( "class" );
					if( szClassName )
					{
						transformClass = szClassName;
					}
					
					try
					{
						auto transform = createOrGetObject< AssetTransform >( ObjectId( transformClass, DEFAULT_OBJECT_NAME ), &child, false /* do not get if already exists */ );
						
						transform->assetName( assetName );
						
						transform->adjustPaths( m_rootSrcPath, m_rootDestPath, m_relativeSrcPath, m_relativeDestPath );
						m_assetTransforms.push_back( transform );
					}
					catch( FreshException& e )
					{
						std::cerr << e.what() << std::endl;
					}
				}
				else if( childName == "class" )
				{
					// Add a pseudoclass.
					//
					const char* attribName  = child.Attribute( "name" );
					if( !attribName )
					{
						dev_warning( "Pseudoclass definition lacked required 'name' attribute. Skipping." );
						continue;
					}
					
					const char* attribBase  = child.Attribute( "extends" );
					if( !attribBase )
					{
						dev_warning( "Pseudoclass definition for '" << childName << "' lacked required 'extends' attribute. Skipping." );
						continue;
					}
					
					try
					{
						createClass( attribName, attribBase, child );
					}
					catch( const FreshException& e )
					{
						dev_warning( e.what() );
					}
				}
				else if( childName == "cd" )	// Change path.
				{
					const char* szPath = child.Attribute( "path" );
					const char* szSrcPath = child.Attribute( "src-path" );
					const char* szDestPath = child.Attribute( "dest-path" );
					
					if( !szSrcPath ) szSrcPath = szPath;
					if( !szDestPath ) szDestPath = szPath;
					
					const path savedSrcPath = m_relativeSrcPath;
					const path savedDestPath = m_relativeDestPath;
					
					if( !szSrcPath && !szDestPath )
					{
						cerr << "WARNING: 'cd' node had no path attributes.\n";
					}
					else
					{
						if( szSrcPath )
						{
							m_relativeSrcPath /= szSrcPath;
						}
						if( szDestPath )
						{
							m_relativeDestPath /= szDestPath;
						}
					}
					
					iterateChildElements( child );
					
					// Restore paths.
					//
					m_relativeSrcPath = savedSrcPath;
					m_relativeDestPath = savedDestPath;
				}
			}
		}

	private:
		
		path m_rootSrcPath;
		path m_rootDestPath;
		path m_relativeSrcPath;
		path m_relativeDestPath;
		
		std::vector< AssetTransform::ptr > m_assetTransforms;
		
	};

	///////////////////////////////////////////////////////////////////////////

	void printUsage()
	{
		cout << "USAGE: fac <transform-file>\n";
	}
}

///////////////////////////////////////////////////////////////////////////

int main( int argc, const char* argv[] )
{
	fr::initReflection();
	
	// Parse options.
	//
	path transformFilePath;
	path rootSrcPath;
	path rootDestPath( "." );
	path destDatabaseFilePath( "assets/assets-imported.xml" );
	path srcDatabaseFilePath( destDatabaseFilePath );
	path classPath( "assets/asset-classes.xml" );
	bool forceOverrideNewer = false;
	
	TCLAP::CmdLine cmd( "Batch-converts Fresh assets including sound and texture files." );
	
	try
	{
		TCLAP::UnlabeledValueArg< string > argTransformFilePath( "transform-file", "The path to an XML file containing asset transform elements.", true, "", "path" );
		cmd.add( argTransformFilePath );
		
		TCLAP::ValueArg< string > argRootSrc( "s", "srcroot", "The path to the root of the source asset files.", false, "", "path" );
		cmd.add( argRootSrc );
		
		TCLAP::ValueArg< string > argRootDest( "d", "destroot", "The path to the root of the destination files.", false, rootDestPath.string(), "path" );
		cmd.add( argRootDest );
		
		TCLAP::ValueArg< string > argExportDatabase( "e", "export", "The relative path within <destroot> to asset database file to be exported.", false, destDatabaseFilePath.string(), "path" );
		cmd.add( argExportDatabase );
		
		TCLAP::ValueArg< string > argImportDatabase( "i", "import", "The relative path within <destroot> to a previous asset database file. Used to recall metadata for assets that don't need to be re-converted.", false, srcDatabaseFilePath.string(), "path" );
		cmd.add( argImportDatabase );
		
		TCLAP::ValueArg< string > argClasses( "c", "classes", "The relative path within <destroot> to the asset class file", false, classPath.string(), "path" );
		cmd.add( argClasses );
		
		TCLAP::SwitchArg argForceOverrideNew( "f", "force", "Force overriding of destination files, even if they are newer", cmd, false );
		
		cmd.parse( argc, argv );
		
		transformFilePath = argTransformFilePath.getValue();
		rootDestPath = argRootDest.getValue();
		rootSrcPath = argRootSrc.getValue();
		
		if( rootSrcPath.empty() )
		{
			rootSrcPath = transformFilePath.parent_path();
		}
		
		destDatabaseFilePath = rootDestPath / argExportDatabase.getValue();
		
		srcDatabaseFilePath = rootDestPath / argImportDatabase.getValue();
		
		classPath = rootDestPath / argClasses.getValue();
		
		forceOverrideNewer = argForceOverrideNew.getValue();
	}
	catch( const TCLAP::ArgException& e )
	{
		std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
		std::cerr << cmd.getMessage() << endl;
		exit( 1 );
	}

	ObjectLinker::create();
	
	if( !classPath.empty() )
	{
		if( exists( classPath ))
		{
			Package::ptr classPackage = loadPackage( classPath, "classes" );
		}
		else
		{
			std::cerr << "WARNING: Asset class file '" << classPath << "' did not exist.\n";
		}
	}
	
	AssetPackage::ptr assetPackage = createPackage< AssetPackage >( "assets" );
	if( !srcDatabaseFilePath.empty() && exists( srcDatabaseFilePath ))
	{
		try
		{
			assetPackage->loadDatabase( srcDatabaseFilePath );
		}
		catch( const FreshException& e )
		{
			std::cerr << e.what() << std::endl;
			exit( 1 );
		}
	}
	else
	{
		std::cerr << "WARNING: Source database file '" << srcDatabaseFilePath << "' not specified or did not exist.\n";
	}
	   
	//
	// Load transforms.
	//
	fac::Transforms transforms( transformFilePath, rootSrcPath, rootDestPath );
	
	transforms.verify();

	const std::time_t databaseFileTime = getFileLastModifiedTime( destDatabaseFilePath.string() );
	transforms.apply( destDatabaseFilePath.string(), databaseFileTime, forceOverrideNewer );
	
	ObjectLinker::destroy();
	
	cout << "Done.\n";
	
    return 0;
}

