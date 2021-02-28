//
//  AssetTransformTexture.cpp
//  fac
//
//  Created by Jeff Wofford on 2/6/13.
//  Copyright (c) 2013 Jeff Wofford. All rights reserved.
//

#include "AssetTransformTexture.h"
#include "Objects.h"
#include "FreshFile.h"
using namespace fr;



namespace
{
	const char* scriptPreamble = R"(	
	
	function isInt(n)
	{
		return n % 1 === 0;
	}
	
	function getNearestPowTwoAtLeast( size )
	{
		return Math.pow( 2, ( Math.floor( Math.log( size - 1 ) / Math.log( 2 )) + 1 ));
	}
	
	function getNearestPowTwoAtMost( size )
	{
		return Math.pow( 2, ( Math.ceil( Math.log( size + 1 ) / Math.log( 2 )) - 1 ));
	}
	
	function getBestPowTwo( size )
	{
		var up = getNearestPowTwoAtLeast( size );
		var upChange = Math.abs( 1.0 - up / size );
		
		var dn = getNearestPowTwoAtMost( size );
		var dnChange = Math.abs( 1.0 - dn / size );
		
		if( upChange <= dnChange )
		{
			return up;
		}
		else
		{
			return dn;
		}
	}
	
	function getBestPowTwoForSize( width, height )
	{
		var bestWidth = getBestPowTwo( width );
		var bestHeight = getBestPowTwo( height );

		return { width: bestWidth, height: bestHeight };
	}
	
	function findLayer( parent, layerName )
	{
		for( var i = 0; i < parent.length; ++i )
		{
			var layer = parent[ i ];
			
			if( layer.name == layerName )
			{
				return layer;
			}
			else if( typeof layer.layers !== 'undefined' )
			{
				var result = findLayer( layer.layers, layerName );
				if( result )
				{
					return result;
				}
			}
		}
		
		return null;
	}
	
	function showLayer( layerName, enable )
	{
		var doc = app.activeDocument;
		
		var layer = findLayer( doc.layers, layerName );
		
		var curLayer = layer;
		
		while( curLayer && typeof curLayer !== 'undefined' )
		{
			curLayer.visible = enable;
			
			if( enable )
			{
				curLayer = curLayer.parent;
			}
			else
			{
				break;
			}
		}
	}
	
	function isLayerNamed( layer, layerNames )
	{
		var layerName = layer.name;
		for( var i = 0; i < layerNames.length; ++i )
		{
			if( layerNames[ i ] == layerName )
			{
				return true;
			}
		}
		return false;
	}
	
	function isolateLayers( layerNames )
	{
		isolateLayersWithin( app.activeDocument, layerNames );
	}
	
	function isolateLayersWithin( layer, layerNames )
	{
		if( isLayerNamed( layer, layerNames ))
		{
			layer.visible = true;
			return true;
		}
		
		var foundRelevantDescendant = false;
		if( typeof layer.layers !== 'undefined' )
		{
			var childLayers = layer.layers;
			for( var i = 0; i < childLayers.length; ++i )
			{
				var child = childLayers[ i ];
				foundRelevantDescendant = isolateLayersWithin( child, layerNames ) || foundRelevantDescendant;
			}
		}
		
		layer.visible = foundRelevantDescendant;
		
		return foundRelevantDescendant;
	}
	
	function showAllLayersRecursive( parent, show )
	{
		for( var i = 0; i < parent.length; ++i )
		{
			var layer = parent[ i ];
			layer.visible = show;
			
			if( typeof layer.layers !== 'undefined' )
			{
				showAllLayersRecursive( layer.layers, show );
			}
		}
	}
	
	function showEachAndEveryLayer( show )
	{
		showAllLayersRecursive(  app.activeDocument.layers, show );
	}
	
	function showAllBaseLayers( show )
	{
		for( var i = 0; i < app.activeDocument.layers.length; ++i )
		{
			var layer = app.activeDocument.layers[ i ];
			layer.visible = show;
		}
	}
	
	function trimEdges()
	{
		var doc = app.activeDocument;
		doc.trim( TrimType.TRANSPARENT );
	}
	
	function expandCanvas( pixels )
	{
		var doc = app.activeDocument;
		
		doc.resizeCanvas( doc.width + pixels * 2, doc.height + pixels * 2 );
	}
	
	function getImageSizeText()
	{
		var doc = app.activeDocument;
		var text = "( " + doc.width.toString() + ", " + doc.height.toString() + " ) ";
		return text;
	}
	
	function resizeToPowTwo( reshapeCanvasElseResizeImage )
	{
		var doc = app.activeDocument;
		
		var powTwoSize = getBestPowTwoForSize( doc.width, doc.height );
		
		if( reshapeCanvasElseResizeImage )
		{
			doc.resizeCanvas( powTwoSize.width, powTwoSize.height );
		}
		else
		{
			doc.resizeImage( powTwoSize.width, powTwoSize.height );
		}
	}
	
	function getDocumentByPath( filePath )
	{
		for( var i = 0; i < app.documents.length; ++i )
		{
			var doc = app.documents[ i ];
			if( decodeURI(doc.fullName) == decodeURI(filePath))
			{
				return doc;
			}
		}
		return null;
	}
	
	function openImage( filePath, forceReopen )
	{
		// Is this file already open?
		//
		var file = new File( filePath );    // Convert filePath to Photoshop form.
		
		var openDocument = getDocumentByPath( file )
		
		if( openDocument )
		{
			if( forceReopen )
			{
				openDocument.close( SaveOptions.DONOTSAVECHANGES );
				openDocument = null;
			}
			else
			{
				app.activeDocument = openDocument;
			}
		}
		
		if( !openDocument )
		{
			app.open( file );
		}
	}
	
	function scaleImage( xScale, yScale )
	{
		var doc = app.activeDocument;
		doc.resizeImage( doc.width * xScale, doc.height * yScale );
	}
	
	function saveAsJpeg( filepath, quality )
	{
		var doc = app.activeDocument;
		var file = new File( filepath );
		var saveOptions = new JPEGSaveOptions();
		saveOptions.embedColorProfile = true;
		saveOptions.formatOptions = FormatOptions.STANDARDBASELINE;
		saveOptions.matte = MatteType.NONE;
		saveOptions.quality = quality;
		doc.saveAs( file, saveOptions, true );
	}
	
	function saveAsPNG( filepath )
	{
		var doc = app.activeDocument;
		var file = new File( filepath );
		var saveOptions = new PNGSaveOptions();
		saveOptions.interlaced = false;
		doc.saveAs( file, saveOptions, true );
	}
	
	function closeDocument()
	{
		var doc = app.activeDocument;
		doc.close( SaveOptions.DONOTSAVECHANGES );
	}
	
	function cleanup()
	{
		// Restore the document state to when it was opened.
		try
		{
			var doc = app.activeDocument;
			doc.activeHistoryState = doc.historyStates[ 0 ];
			app.purge( PurgeTarget.HISTORYCACHES );
		}
		catch( e )
		{}
	}
	
	
	var startRulerUnits = app.preferences.rulerUnits;
	var startTypeUnits = app.preferences.typeUnits;
	var startDisplayDialogs = app.displayDialogs;
	
	app.preferences.rulerUnits = Units.PIXELS;
	app.preferences.typeUnits = TypeUnits.PIXELS;
	app.displayDialogs = DialogModes.NO;
	app.visible = false;		// Hide in the background.
	
	var resultText = "";
	
	)";
	
	//////////////////////////////////////////////////////////////////////////////////////////////////
	
	const char* scriptPostamble = R"(
	
	cleanup();
	
	app.preferences.rulerUnits = startRulerUnits;
	app.preferences.typeUnits = startTypeUnits;
	app.displayDialogs = startDisplayDialogs;
	
	resultText;
	
	)";
	
	//////////////////////////////////////////////////////////////////////////////////////////////////

}

namespace fac
{
	
	FRESH_DEFINE_CLASS( AssetTransformTexture )
	DEFINE_DVAR( AssetTransformTexture, bool, m_trimTransparency );
	DEFINE_DVAR( AssetTransformTexture, bool, m_resizeToPowerOf2 );
	DEFINE_DVAR( AssetTransformTexture, bool, m_reshapeToPowerOf2 );
	DEFINE_DVAR( AssetTransformTexture, vec2, m_postScale );
	DEFINE_DVAR( AssetTransformTexture, bool, m_layersInitiallyAllHide );
	DEFINE_DVAR( AssetTransformTexture, bool, m_layersInitiallyAllShow );
	DEFINE_VAR( AssetTransformTexture, std::vector< std::string >, m_layersToShow );
	DEFINE_VAR( AssetTransformTexture, std::vector< std::string >, m_layersToHide );
	DEFINE_DVAR( AssetTransformTexture, int, m_jpegQuality );
	DEFINE_DVAR( AssetTransformTexture, bool, m_forceReopenSrcFile );
	DEFINE_DVAR( AssetTransformTexture, bool, m_closeSrcWhenDone );
	DEFINE_DVAR( AssetTransformTexture, bool, m_doMipMap );
	DEFINE_DVAR( AssetTransformTexture, fr::Texture::ClampMode, m_clampModeU );
	DEFINE_DVAR( AssetTransformTexture, fr::Texture::ClampMode, m_clampModeV );
	DEFINE_DVAR( AssetTransformTexture, unsigned int, m_alphaBorderFlags );

	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( AssetTransformTexture )
	
	bool AssetTransformTexture::apply()
	{
		Super::apply();
		
		// Construct a Photoshop JavaScript file to apply the desired processing to this file.
		//
		std::ofstream tempJavaScriptFile;
		path tempJavaScriptFilePath = createTempFile( tempJavaScriptFile );
		ASSERT( tempJavaScriptFilePath.empty() == false );

		configurePhotoshopScript( tempJavaScriptFile );

		// Execute AppleScript (Mac) or VBScript (Win) to open Photoshop and run the JavaScript.
		//
		bool succeeded = true;
		
#if __APPLE__

		std::string command( "osascript -l AppleScript /Users/Jeff/Projects/git/ctcr/Fresh/tools/FreshAssetConverter/ps-open.scpt " );
		command += tempJavaScriptFilePath.string();

		try
		{
			FILE* const processFile = ::popen( command.c_str(), "r" );

			if( !processFile )
			{
				std::cerr << "Failed to run process for command '" << command << "'.\n";
				succeeded = false;
			}
			else
			{
				std::string resultText;
				
				char buffer[ 1024 ];
				while( std::fgets( buffer, sizeof( buffer ) - 1, processFile ))
				{
					resultText += buffer;
				}
				
				::pclose( processFile );
				
				processResultText( resultText );
			}
		}
		catch( ... )
		{
			std::cerr << "Error running conversion scripts.\n";
			succeeded = false;
		}
		
#elif WIN32

		std::ofstream tempExecutionScriptFile;
		path tempExecutionScriptFilePath = "tempvbscript.vbs"; //createTempFile( tempExecutionScriptFile );
		ASSERT( tempExecutionScriptFilePath.empty() == false );

		tempExecutionScriptFile.open( tempExecutionScriptFilePath.string() );

		std::string scriptContents = R"(
			Dim appRef
			Set appRef = CreateObject( "Photoshop.Application" )
			appRef.DoJavaScriptFile )";

		tempExecutionScriptFile << scriptContents;
		tempExecutionScriptFile << tempJavaScriptFilePath.string();

		tempExecutionScriptFile.close();

		::system( tempExecutionScriptFilePath.c_str() );

#else
#	error FAC is unsupported on this platform.
#endif
		
		// Delete the JavaScript file.
		//
		std::remove( tempJavaScriptFilePath.c_str() );
		
		// Delete the execution file.
		//
#if WIN32
		std::remove( tempExecutionScriptFilePath.c_str() );
#endif

		return succeeded;
	}
	
	void AssetTransformTexture::writeAssetToDatabaseEnd( std::ostream& assetDatabaseXML ) const
	{
		if( !m_doMipMap )
		{
			assetDatabaseXML << "\t\t<doMipMap>" << std::boolalpha << m_doMipMap << "</doMipMap>\n";
		}
		
		if( m_alphaBorderFlags != 0 )
		{
			assetDatabaseXML << "\t\t<alphaBorderFlags>" << m_alphaBorderFlags << "</alphaBorderFlags>\n";
		}		
		
		if( m_clampModeU != fr::Texture::ClampMode::Clamp )
		{
			assetDatabaseXML << "\t\t<clampModeU>" << m_clampModeU << "</clampModeU>\n";
		}
		
		if( m_clampModeV != fr::Texture::ClampMode::Clamp )
		{
			assetDatabaseXML << "\t\t<clampModeV>" << m_clampModeV << "</clampModeV>\n";
		}
		
		if( m_originalDimensions != m_dimensions )
		{
			assetDatabaseXML << "\t\t<originalDimensions>" << m_originalDimensions << "</originalDimensions>\n";
		}
		
		Super::writeAssetToDatabaseEnd( assetDatabaseXML );
	}
	
	void AssetTransformTexture::configurePhotoshopScript( std::ofstream& out ) const
	{
		// Include preamble.
		//
		out << scriptPreamble;
		
		// Open the image.
		//
		out << std::endl << std::endl << "openImage( \"" << fullPathSrc().string() << "\", " << std::boolalpha << m_forceReopenSrcFile << " );\n";
		
		// Operations.
		//
		if( m_layersInitiallyAllShow )
		{
			out << "showAllBaseLayers( true );\n";
		}
		else if( m_layersInitiallyAllHide )
		{
			out << "showAllBaseLayers( false );\n";
		}
		
		if( !m_layersToShow.empty() )		// Don't isolate nothing.
		{
			std::ostringstream layerArray;
			for( const auto& layerName : m_layersToShow )
			{
				if( !layerArray.str().empty() )
				{
					layerArray << ", ";
				}
				
				layerArray << "\"" << layerName << "\"";
			}

			out << "isolateLayers( [" << layerArray.str() << "] );\n";
		}
		
		for( const auto& layerName : m_layersToHide )
		{
			out << "showLayer( \"" << layerName << "\", false );\n";
		}
		
		if( m_trimTransparency )
		{
			out << "trimEdges();\n";
		}
		
		if( m_postScale.x != 1.0f || m_postScale.y != 1.0f )
		{
			out << "scaleImage( " << m_postScale.x << ", " << m_postScale.y << " );\n";
		}
		
		out << "resultText += getImageSizeText() + \" \";\n";
		
		if( m_reshapeToPowerOf2 )
		{
			out << "resizeToPowTwo( true );\n";
		}
		else if( m_resizeToPowerOf2 )
		{
			out << "resizeToPowTwo( false );\n";
		}
		
		out << "resultText += getImageSizeText() + \" \";\n";
		
		// Save the image.
		//
		if( destExtension() == ".png" )
		{
			out << "saveAsPNG( \"" << fullPathDest().string() << "\" );\n";
		}
		else
		{
			if( destExtension() != ".jpg" )
			{
				std::cerr << "WARNING: " << *this << " indicated unrecognized extension '" << destExtension() << "'. Using JPEG format.\n";
			}
			
			auto jpegQualityToUse = m_jpegQuality;
			if( jpegQualityToUse < 0 || jpegQualityToUse > 12 )
			{
				std::cerr << "WARNING: jpegQuality " << jpegQualityToUse << " is out of range. Setting to 10.\n";
				jpegQualityToUse = 10;
			}
			
			out << "saveAsJpeg( \"" << fullPathDest().string() << "\", " << jpegQualityToUse << " );\n";
		}
		
		if( m_closeSrcWhenDone )
		{
			out << "closeDocument();\n";
		}
		
		// Include postamble.
		//
		out << std::endl << scriptPostamble;
		
		out.close();
	}

	struct expector
	{
		struct Mismatch {};
		
		expector( const std::string& s ) : m_text( s ) {}
		
		void check( const std::string& s ) const
		{
			if( s != m_text )
			{
				throw Mismatch();
			}
		}
		
		size_t size() const { return m_text.size(); }
		
		bool matchesPart( const std::string& part )
		{
			if( part.size() == m_text.size() ) return false;
			
			std::string myPart = m_text.substr( 0, part.size() );
			return part == myPart;
		}
		
	private:
		
		std::string m_text;
	};
	
	std::istream& operator>>( std::istream& in, expector&& e )
	{
		std::string whatsThere;
		
		while( in && e.matchesPart( whatsThere ))
		{
			whatsThere += in.get();
		}		
		
		e.check( whatsThere );
		
		return in;
	}
	
	
	void AssetTransformTexture::processResultText( const std::string& resultText )
	{
		std::istringstream s( resultText );
		try
		{
			s >> std::ws >> expector( "( " ) >> m_originalDimensions.x >> expector( " px, " ) >> m_originalDimensions.y >> expector( " px ) " );
			s >> std::ws >> expector( "( " ) >> m_dimensions.x >> expector( " px, " ) >> m_dimensions.y >> expector( " px )" );
		}
		catch( expector::Mismatch )
		{
			std::cerr << "Incorrect result format.\n";
		}
	}
}

