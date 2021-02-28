//
//  Editor.cpp
//  Fresh
//
//  Created by Jeff Wofford on 3/8/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#include "Editor.h"
#include "CommandProcessor.h"
#include "escaped_string.h"
#include "FreshTime.h"
#include "FreshFile.h"
#include "ApplicationStaged.h"
#include "SimpleButton.h"
#include "Packages.h"
#include "EdObjectBrowser.h"
#include "UIPanel.h"
#include "EdTimeline.h"
#include "UIEditBox.h"
#include <cstdio>				// For std::remove() to delete files.

#define TRACE_HISTORY

#ifdef TRACE_HISTORY
#	define trace_history( expression ) trace( expression )
#else
#	define trace_history( expression )
#endif

namespace
{
	using namespace fr;
	
	path getAutosavePath()
	{
		return getDocumentPath( "autosave" );
	}
	
	const char STOCK_AUTOSAVE_FILEBASE[] = "~autosave";
	
	const size_t MAX_AUTOSAVES_TO_KEEP = 10;
	const size_t MAX_VALID_AUTOSAVE_INDEX = 10;
	
	const TimeType AUTOSAVE_INTERVAL = 2 * 60;		// Measured in seconds.
}

namespace fr
{
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( EditorSettings )
	DEFINE_VAR( EditorSettings, std::vector< path >, m_recentFiles );
	DEFINE_VAR( EditorSettings, bool, m_reopenLastOnLaunch );
	FRESH_IMPLEMENT_STANDARD_CONSTRUCTORS( EditorSettings )
	
	path EditorSettings::mostRecentFile() const
	{
		if( m_recentFiles.empty() )
		{
			return "";
		}
		else
		{
			return m_recentFiles.back();
		}
	}
	
	void EditorSettings::onAddressingFile( const path& filePath )
	{
		// Already have this file?
		//
		auto iter = std::find( m_recentFiles.begin(), m_recentFiles.end(), filePath );
		
		if( iter == m_recentFiles.end() )
		{
			// No. Add it.
			//
			m_recentFiles.push_back( filePath );
		}
		else
		{
			// Yes. Move it to the back.
			
			// Careful! If filePath is a reference to a member fo m_recentFiles (and it very well may be), erase() will (eventually) corrupt it.
			// So make a copy.
			auto copy( filePath );
			
			m_recentFiles.erase( iter );
			m_recentFiles.push_back( copy );
		}
	}
	
	/////////////////////////////////////////////////////////////
	
	FRESH_DEFINE_CLASS_UNPLACEABLE( Editor )
	DEFINE_VAR( Editor, ClassInfo::cptr, m_newStageClass );
	DEFINE_VAR( Editor, Stage::ptr, m_editedRoot );
	DEFINE_VAR( Editor, DisplayObject::ptr, m_editedCurrent );
	DEFINE_VAR( Editor, Manipulator::ptr, m_manipulatorCurrent );
	DEFINE_VAR( Editor, PropertyPane::ptr, m_propertyPane );
	DEFINE_VAR( Editor, SmartPtr< EdClassInventory >, m_classInventory );
	DEFINE_VAR( Editor, DisplayObjectContainer::ptr, m_toolbarHost );
	DEFINE_VAR( Editor, SmartPtr< EdObjectBrowser >, m_objectBrowser );
	DEFINE_VAR( Editor, SmartPtr< EdTimeline >, m_timeline );
	DEFINE_VAR( Editor, ClassName, m_defaultManipulatorClass );
	DEFINE_VAR( Editor, ManipulatorClasses, m_manipulatorClasses );
	DEFINE_VAR( Editor, ClassInfo::Name, m_playingDisplayClass );
	DEFINE_VAR( Editor, path, m_defaultFileTemplate );
	DEFINE_VAR( Editor, path, m_settingsDocumentPath );
	DEFINE_VAR( Editor, bool, m_isHUDMinimized );

	DEFINE_METHOD( Editor, createNew )
	DEFINE_METHOD( Editor, saveWithPromptIfNecessary )
	DEFINE_METHOD( Editor, openWithPrompt )
	DEFINE_METHOD( Editor, play )
	DEFINE_METHOD( Editor, resumeEditing )
	DEFINE_METHOD( Editor, undo )
	DEFINE_METHOD( Editor, redo )

	Editor::Editor( CreateInertObject c )
	:	Super( c )
	,	m_changeHistory( std::bind( &Editor::applyHistoryState, this, std::placeholders::_1 ))
	{}
	
	Editor::Editor( const ClassInfo& assignedClassInfo, NameRef name )
	:	Super( assignedClassInfo, name )
	,	m_changeHistory( std::bind( &Editor::applyHistoryState, this, std::placeholders::_1 ))
	{
		m_substepForRealTimeAdjustment = false;	// Editor doesn't fiddle with multiple substeps per frame or any special frame rate manipulation.
		isDragEnabled( true );
		
		//
		// Add console commands.
		//
		
		// Create the edit command.
		//
		{
			std::function< void( const ObjectId& ) > boundFn( std::bind( &Editor::beginEditingNamedObject, this, std::placeholders::_1 ) );
			auto caller = stream_function< void( const ObjectId& ) >( boundFn );
			auto command = CommandProcessor::instance().registerCommand( this, "edit", "begins editing a specified object", caller );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "object-id", false, "the object ID, or part of the ID, of the desired root object. Default: the stage" } ));
		}
		
		// Create the edload command.
		//
		{
			std::function< void( const path& ) > boundFn( std::bind( &Editor::openFile, this, std::placeholders::_1 ) );
			auto caller = stream_function< void( const path& ) >( boundFn );
			auto command = CommandProcessor::instance().registerCommand( this, "edload", "begins editing a specified file", caller );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "file-path", true, "the full path of a manifest to edit." } ));
		}
		
		// Create the edsave command.
		//
		{
			std::function< void( const path& ) > boundFn( std::bind( &Editor::saveToDocument, this, std::placeholders::_1 ) );
			auto caller = stream_function< void( const path& ) >( boundFn );
			auto command = CommandProcessor::instance().registerCommand( this, "edsave", "saves the edited root object to a file", caller );
			command->addArgument( CommandProcessor::CommandAbstract::Argument( { "filename", false, "the file name to save into the Document Root. Default: fresh-editor-save-DATETIME.xml" } ));
		}

		// Create the ednew command.
		//
		{
			std::function< void() > boundFn( std::bind( &Editor::createNew, this ) );
			auto caller = stream_function< void()  >( boundFn );
			auto command = CommandProcessor::instance().registerCommand( this, "ednew", "begins editing a blank object.", caller );
		}
		
		// Create the edplay command.
		//
		{
			std::function< void() > boundFn( std::bind( &Editor::play, this ) );
			auto caller = stream_function< void()  >( boundFn );
			CommandProcessor::instance().registerCommand( this, "edplay", "stops editing and resumes with the stage", caller );
		}
		
		// Schedule autosave callbacks.
		//
		scheduleCallback( FRESH_CALLBACK( onTimeToAutosave ), AUTOSAVE_INTERVAL );
	}
	
	Editor::~Editor()
	{
		if( CommandProcessor::doesExist() )
		{
			CommandProcessor::instance().unregisterAllCommandsForHost( this );
		}
	}
	
	SmartPtr< EdClassInventory > Editor::classInventory() const
	{
		return m_classInventory;
	}
	
	Stage::ptr Editor::root() const
	{
		return m_editedRoot;
	}
	
	DisplayObject::ptr Editor::base() const
	{
		return m_editedBase;
	}

	DisplayObject::ptr Editor::current() const
	{
		return m_editedCurrent;
	}	
	
	bool Editor::isOpen() const
	{
		return (bool) m_editedRoot;
	}
	
	void Editor::createNewWithoutTemplate()
	{
		close();
				
		m_editedBase = createObject< DisplayObject >( *m_newStageClass );

		if( m_editedBase )
		{
			m_editedPackage = createPackage< DisplayPackage >( "~edited-scene" );
			m_editedPackage->root( m_editedBase );
			
			beginEditing( m_editedBase );
		}
		else
		{
			con_error( "Could not create DisplayObject of class " << m_newStageClass << "." );
		}
	}
	
	void Editor::createNew()
	{
		bool opened = false;
		if( !m_defaultFileTemplate.empty() )
		{
			path templatePath = getFilePath( m_defaultFileTemplate );
			
			if( !templatePath.empty() )
			{
				createNewFromTemplate( templatePath );
				opened = true;
			}
		}
		
		if( !opened )
		{
			createNewWithoutTemplate();
		}
	}
	
	void Editor::createNewFromTemplate( const path& fullPathToTemplate, const ObjectId& editingBase )
	{
		close();
		
		m_editedPackage = createPackage< DisplayPackage >( "~untitled" );
		m_editedPackage->retainMembers();
		
		m_editedPackage->loadFile( fullPathToTemplate );
		
		// Load the manifest, just storing the assumed (not requested) base for now.
		//
		m_editedBase = m_editedPackage->root();
		
		if( editingBase )
		{
			m_editedBase = m_editedPackage->find< DisplayObject >( editingBase );
		}
		
		if( m_editedBase )
		{
			// Finished establishing this file as the edited one by establishing its root based on the assumed base.
			//
			establishEditedRoot();

			beginEditing( m_editedBase );
			
			m_editedPackage->releaseMembers();
		}
		else
		{
			con_error( "Failed to load editable display object from template '" << fullPathToTemplate << "'." );
			m_editedPackage = nullptr;
		}
		
		ASSERT( m_currentDocumentPath.empty() );
	}

	void Editor::openFile( const path& fullPath )
	{
		if( !exists( fullPath ))
		{
			con_error( "Could not find a file at " << fullPath << " to open for editing." );
			return;
		}
		
		close();
		
		trace( "Editing the object in manifest " << fullPath );
		
		m_editedPackage = createPackage< DisplayPackage >( fullPath.filename().string() );
		m_editedPackage->retainMembers();
		m_editedPackage->loadFile( fullPath );
		
		m_editedBase = m_editedPackage->root();
		
		if( m_editedBase )
		{
			m_currentDocumentPath = fullPath;
			Application::instance().windowTitle( m_currentDocumentPath.stem().string() );
			
			m_settings->onAddressingFile( fullPath );
			saveSettings();
			
			beginEditing( m_editedBase );

			m_editedPackage->releaseMembers();
		}
		else
		{
			con_error( "Failed to load editable display object from manifest '" << fullPath << "'." );
			m_editedPackage = nullptr;
		}
	}

	void Editor::establishEditedRoot()
	{
		REQUIRES( m_editedBase );
		REQUIRES( m_editedPackage );
		
		if( !m_editedBase->hasStage() )
		{
			// Create a temporary stage for this base.
			//
			m_editedRoot = createObject< Stage >( *m_newStageClass, "theStage" );
			
			m_editedRoot->stageDimensions( stageDimensions() );
		}
		else
		{
			m_editedRoot = &m_editedBase->stage();
		}

		ASSERT( m_editedRoot );

		// Now that the base has a root, make sure it's actually in the root tree.
		//
		if( m_editedRoot != m_editedBase && !m_editedBase->parent() )
		{
			m_editedRoot->addChild( m_editedBase );
		}		
		
		Singleton< ApplicationStaged >::instance().completeStageLoading( *m_editedRoot );
		
		m_editedRoot->isRootOfRendering( false );
	}
	
	void Editor::beginEditing( DisplayObject::ptr newBase )
	{
		REQUIRES( newBase );
		
		if( isEditedRootPlaying() )
		{
			resumeEditing();
		}
		
		establishEditedRoot();

		setEditedObject( newBase );
		
		trace_history( this << " clearing undo/redo history" );
		
		m_changeHistory.clear();
		saveHistoryState();
		
		PROMISES( newBase == base() );
		PROMISES( root() );
	}
	
	void Editor::beginEditingNamedObject( const ObjectId& objectToEdit )
	{
		DisplayObject::ptr newBase;
		
		if( objectToEdit )
		{
			newBase = m_editedPackage->find< DisplayObject >( objectToEdit );
		}
		else
		{
			newBase = m_editedBase;
		}
		
		if( !newBase )
		{
			con_error( "Nothing to edit." );
		}
		else
		{
			if( newBase == this || hasDescendant( newBase ) )
			{
				con_error( "Cannot edit " << newBase << " because it is part of the editing system." );
			}
			else
			{
				beginEditing( newBase );
			}
		}
	}

	void Editor::editParent()
	{
		if( m_editedCurrent != m_editedBase )
		{
			setEditedObject( m_editedCurrent->parent() );
		}
	}
	
	void Editor::editAncestor( DisplayObjectContainer::ptr ancestor )
	{
		REQUIRES( ancestor );
		REQUIRES( m_editedCurrent->hasAncestor( ancestor ));
		
		setEditedObject( ancestor );
	}

	void Editor::editChild( DisplayObject::ptr child )
	{
		REQUIRES( child );
		
		if( !child->editorLocked() )
		{
			setEditedObject( child );
		}
	}

	void Editor::setEditedObject( DisplayObject::ptr object )
	{
		REQUIRES( object != this );
		REQUIRES( !object || object->hasStage() );
		REQUIRES( !object || !hasDescendant( object ));

		if( m_editedCurrent != object )
		{
			vec2 savedManipulatorTranslation( 0, 0 );
			real savedManipulatorScale( 1.0f );
			
			// Remove old manipulator.
			//
			if( m_manipulatorCurrent )
			{
				savedManipulatorTranslation = m_manipulatorCurrent->position();
				savedManipulatorScale = m_manipulatorCurrent->scale().x;
				
				removeChild( m_manipulatorCurrent );
				m_manipulatorCurrent = nullptr;
			}
			
			m_editedCurrent = object;		// Might assign nullptr.
			
			if( m_editedCurrent )
			{
				trace( "Editing object " << m_editedCurrent );
				
				// Create the manipulator for this object and put it in the editor namespace.
				//
				auto manipulatorClassName = manipulatorClassForClass( m_editedCurrent->className() );
				
				auto manipulatorClass = getClass( manipulatorClassName );
				ASSERT( manipulatorClass );
				
				m_manipulatorCurrent = createObject< Manipulator >( *manipulatorClass );
				ASSERT( m_manipulatorCurrent );
				
				addChildAt( m_manipulatorCurrent, 0 );
				
				ASSERT( m_manipulatorCurrent->verifyTree() );
				
				m_manipulatorCurrent->setup( this );
				
				m_manipulatorCurrent->position( savedManipulatorTranslation );
				m_manipulatorCurrent->scale( savedManipulatorScale );				
			}
			
			saveHistoryState();
		}
		else
		{
			// Edited object not changing: just refresh the manipulator.
			//
			if( m_manipulatorCurrent )
			{
				m_manipulatorCurrent->onSelectedObjectsChanged();
			}
		}
	}

	void Editor::saveToDocument( path documentFilePath )
	{
		if( documentFilePath.empty() )
		{
			std::string documentFileName( "fresh-editor-save-" );			
			documentFileName += getStandardTimeDisplay( std::time( NULL ));
			documentFileName += ".xml";
			
			documentFilePath = documentFileName;
		}
		
		saveToFile( getDocumentPath( documentFilePath ));
	}
	
	void Editor::saveToFile( const path& fullPath )
	{
		REQUIRES( fullPath.empty() == false );
		
		std::ofstream out;
		out.exceptions( std::ios::badbit | std::ios::failbit );
		
		try
		{
			out.open( fullPath.c_str() );
		}
		catch( const std::exception& e )
		{
			con_error( "Exception saving editor package to " << fullPath << ": " << e.what() );
			throw;
		}
		
		trace( "Saving editor package to " << fullPath );
		
		saveState( out );
		
		m_currentDocumentPath = fullPath;
		Application::instance().windowTitle( m_currentDocumentPath.stem().string() );
		m_settings->onAddressingFile( fullPath );
		saveSettings();
	}
	
	void Editor::saveState( std::ostream& out, bool forceSaveAllProperties ) const
	{
		ASSERT( m_editedPackage );
		ASSERT( m_editedPackage->root() == m_editedBase );

		// Detach the base from its parent so that it's isolated.
		//
		DisplayObjectContainer::ptr baseParent = m_editedBase->parent();
		int baseIndexInParent = -1;
		
		if( baseParent )
		{
			baseIndexInParent = baseParent->getChildIndex( m_editedBase );
			ASSERT( baseIndexInParent >= 0 );
			
			baseParent->removeChild( m_editedBase );
		}
		
		ASSERT( m_editedPackage );
		ASSERT( m_editedPackage->root() == m_editedBase );
		
		// Save the package itself.
		//
		m_editedPackage->save( out, forceSaveAllProperties );
		
		// Reattach the base to its parent, if any.
		//
		if( baseParent )
		{
			ASSERT( baseIndexInParent >= 0 );
			baseParent->addChildAt( m_editedBase, baseIndexInParent );
		}
	}
	
	void Editor::loadState( const std::string& in )
	{
		ASSERT( m_editedPackage );
		
		std::unique_ptr< XmlElement > element = stringToXmlElement( in );
		ASSERT( element );
		
		m_editedPackage->retainMembers();
		m_editedPackage->loadFromElement( *element );

		ASSERT( m_editedBase );
		ASSERT( m_editedBase == m_editedPackage->root() );
		
		// The edited base may have become detached from the root while loading. Reattach it.
		//
		ASSERT( m_editedRoot );
		if( !m_editedRoot->hasChild( m_editedBase ) && m_editedRoot != m_editedBase )
		{
			m_editedRoot->addChild( m_editedBase );
		}
		
		m_editedRoot->onStageLoaded();
		
		m_editedPackage->releaseMembers();
		
		if( m_manipulatorCurrent )
		{
			m_manipulatorCurrent->onObjectsChangedExternally();
		}
	}
	
	FRESH_DEFINE_CALLBACK( Editor, onTimeToAutosave, Event )
	{
		if( !isEditedRootPlaying() )
		{
			autoSave();
		}
		scheduleCallback( FRESH_CALLBACK( onTimeToAutosave ), AUTOSAVE_INTERVAL );
	}
	
	void Editor::autoSave() const
	{
		cleanupAutoSaves( MAX_AUTOSAVES_TO_KEEP - 1 );		// Keeping the latest few.
		
		size_t num, min, max, firstAvailable;
		getAutoSaveFileInformation( num, min, max, firstAvailable );
		ASSERT( firstAvailable >= 0 && firstAvailable < MAX_AUTOSAVES_TO_KEEP );
		
		const auto autoSaveIndex = firstAvailable;
			
		const path tempFilePath = autoSaveFilePathForIndex( autoSaveIndex );
		
		try
		{
			const auto& hostDirectory = tempFilePath.parent_path();
			
			if( !exists( hostDirectory ))
			{
				create_directories( hostDirectory );
			}
			
			std::ofstream tempFile;
			tempFile.exceptions( std::ios::badbit | std::ios::failbit );

			tempFile.open( tempFilePath.string() );
			
			trace( "Autosaving " << tempFilePath.string() );
			saveState( tempFile );
		}
		catch( const std::exception& e )
		{
			con_error( "Exception saving autoSave file to " << tempFilePath << ": " << e.what() );
			throw;
		}
	}
	
	void Editor::getAutoSaveFileInformation( size_t& outNum, size_t& outMin, size_t& outMax, size_t& firstAvailable ) const
	{
		outNum = 0;
		outMin = -1;
		outMax = -1;
		firstAvailable = -1;
		
		for( size_t i = 0; i <= MAX_VALID_AUTOSAVE_INDEX; ++i )
		{
			if( exists( autoSaveFilePathForIndex( i )))
			{
				++outNum;
				if( outMin >= MAX_VALID_AUTOSAVE_INDEX )
				{
					outMin = i;
				}
				outMax = i;
			}
			else if( firstAvailable == size_t( -1 ))
			{
				firstAvailable = i;
			}
		}
	}
	
	path Editor::autoSaveFilePathForIndex( size_t index ) const
	{
		std::ostringstream tempFileName;
		tempFileName << STOCK_AUTOSAVE_FILEBASE << "~" << std::setfill( '0' ) << std::setw( 3 ) << index << ".xml";
		return getAutosavePath() / tempFileName.str();
	}

	void Editor::cleanupAutoSaves( size_t nSavesToKeep ) const
	{
		// Delete all but the newest nSavesToKeep autosave files.
		
		size_t num, min, max, firstAvailable;
		getAutoSaveFileInformation( num, min, max, firstAvailable );
		
		std::vector< path > autoSavePaths;
		autoSavePaths.reserve( num );
		
		for( size_t i = 0; i <= MAX_VALID_AUTOSAVE_INDEX; ++i )
		{
			const auto& aPath = autoSaveFilePathForIndex( i );
			if( exists( aPath ))
			{
				autoSavePaths.push_back( aPath );
			}
		}
		
		if( nSavesToKeep < autoSavePaths.size() )
		{
			// Sort newest to oldest.
			//
			std::sort( autoSavePaths.begin(), autoSavePaths.end(), []( const path& a, const path& b )
					  {
						  return getFileLastModifiedTime( a ) > getFileLastModifiedTime( b );
					  } );
			
			// Delete the oldest, keeping the nSavesToKeep newest.
			//
			for( auto aPath = autoSavePaths.begin() + nSavesToKeep; aPath != autoSavePaths.end(); ++aPath )
			{
				trace( "Deleting older autosave file " << aPath->string() );
				std::remove( aPath->c_str() );
			}
		}
	}
	
	void Editor::close()
	{
		ASSERT( !isEditedRootPlaying() );
		if( isOpen() )
		{
			autoSave();
			
			ASSERT( m_editedRoot );
			
			setEditedObject( nullptr );
			m_editedRoot = nullptr;
			m_editedBase = nullptr;
			
			m_editedPackage = nullptr;
			
			m_currentDocumentPath.clear();
			Application::instance().setWindowTitleToDefault();
		}
		PROMISES( !isOpen() );
	}
	
	void Editor::setToolbar( DisplayObject::ptr toolbar )
	{
		ASSERT( m_toolbarHost );
		while( m_toolbarHost->numChildren() ) m_toolbarHost->removeChildAt( 0 );
		m_toolbarHost->addChild( toolbar );
		
		// Hookup to editor buttons.
		//
		{
			auto button = m_toolbarHost->getDescendantByName< SimpleButton >( "_button_new", NameSearchPolicy::Substring );
			ASSERT( button );
//		button->addEventListener( SimpleButton::TAPPED, FRESH_CALLBACK( onButtonNew ));
		}
	}

	vec2 Editor::screenToSubject( const vec2& screenLocation ) const
	{
		return Renderer::instance().screenToWorld2D( screenLocation )
		/ m_manipulatorCurrent->scale()
		- m_manipulatorCurrent->position();
	}

	void Editor::showEditingHUD( bool show )
	{
		for( auto child : *this )
		{
			child->visible( show ^ ( child == m_playingDisplay ));
		}
	}
	
	void Editor::play()
	{
		if( !m_isEditedRootPlaying )
		{
			saveHistoryState();
		
			if( m_playingDisplayClass.empty() == false && !m_playingDisplay )
			{
				m_playingDisplay = createObject< DisplayObject >( ObjectId( m_playingDisplayClass, "playing_display" ));
				addChild( m_playingDisplay );
			}
						
			m_isEditedRootPlaying = true;

			if( m_playingDisplay )
			{
				m_playingDisplay->visible( m_isEditedRootPlaying );
			}
			showEditingHUD( !m_isEditedRootPlaying );
			
			if( m_manipulatorCurrent )
			{
				m_manipulatorCurrent->isTouchEnabled( !m_isEditedRootPlaying );
			}

			if( m_editedRoot )
			{
				ASSERT( m_editedPackage );
				
				// Establish the package for the edited stage based on the edited package.
				//
				auto clonedPackage = createPackage< DisplayPackage >();
				clonedPackage->merge( *m_editedPackage, Package::MergePolicy::ReplaceExisting );
				
				m_editedRoot->stagePackage( clonedPackage );
				
				m_editedRoot->isRootOfRendering( m_isEditedRootPlaying );
				isRootOfRendering( !m_isEditedRootPlaying );
				m_editedRoot->beginPlay();
			}
		}
	}
	
	void Editor::resumeEditing( bool retainPlayedState )
	{
		if( m_isEditedRootPlaying )
		{
			m_isEditedRootPlaying = false;

			ASSERT( m_editedRoot );
			m_editedRoot->endPlay();
			
			if( m_playingDisplay )
			{
				m_playingDisplay->visible( m_isEditedRootPlaying );
			}
			showEditingHUD( !m_isEditedRootPlaying );

			if( m_manipulatorCurrent )
			{
				m_manipulatorCurrent->isTouchEnabled( !m_isEditedRootPlaying );
			}
			
			if( m_editedRoot )
			{
				m_editedRoot->isRootOfRendering( m_isEditedRootPlaying );
				isRootOfRendering( !m_isEditedRootPlaying );
			}
			
			if( !retainPlayedState )
			{
				// Restore the state as it was prior to playing.
				
				// The edited base's relationship with the root may have changed.
				// Regularize the relationship by separating the base from its parent.
				//
				if( m_editedBase->parent() )
				{
					m_editedBase->parent()->removeChild( m_editedBase );
				}
				
				undo();
			}
			else
			{
				// Record the new played state.
				saveHistoryState();
			}
		}
	}
	
	EdObjectBrowser::ptr Editor::openObjectBrowser( const ClassInfo& baseClass )
	{
		if( m_objectBrowser )
		{
			// Preload all assets of this type.
			//
			Application::instance().assetPackage().loadAssets( [&]( const ClassInfo& classInfo, ObjectNameRef )
			{
				return classInfo.isKindOf( baseClass );
			} );
				
			// Make in-package objects available in the browser.
			//
			addSearchPackage( m_editedPackage );
			
			m_objectBrowser->populate( ObjectId( "", baseClass.className(), "" ));
			
			removeSearchPackage( m_editedPackage );
		}
		
		return m_objectBrowser;
	}
	
	void Editor::callPlayingMethod( const std::function< void() >&& fnForRoot, const std::function< void() >&& fnForBaseClass )
	{
		if( m_isEditedRootPlaying && m_editedRoot )
		{
			fnForRoot();
		}
		fnForBaseClass();
	}

	void Editor::beginTouchAction()
	{
//		trace( "beginTouchAction()" );
		if( m_isTouchActionInProgress )
		{
			dev_warning( this << ".beginTouchAction() found an action already in progress. Failed to call endTouchAction()?" );
		}
		
		m_isTouchActionInProgress = true;
	}
	
	void Editor::endTouchAction()
	{
//		trace( "endTouchAction()" );
		if( !m_isTouchActionInProgress )
		{
			dev_warning( this << ".endTouchAction() found no action in progress. Failed to call beginTouchAction(), or too many calls to endTouchAction?" );
		}
		m_isTouchActionInProgress = false;
	}
	
	void Editor::update()
	{
		if( !doUpdate() ) return;
		
		callPlayingMethod( std::bind( &Stage::update, m_editedRoot ), [&]() { Super::update(); });
	}
	
	void Editor::render( TimeType relativeFrameTime, RenderInjector* injector )
	{
		callPlayingMethod( std::bind( &Stage::render, m_editedRoot, relativeFrameTime, injector ), [&]() { Super::render( relativeFrameTime, injector ); });
	}
	
	void Editor::onTouchBegin( const EventTouch& event )
	{
		callPlayingMethod( std::bind( &Stage::onTouchBegin, m_editedRoot, event ), [&]() { Super::onTouchBegin( event ); });
	}
	
	void Editor::onTouchMove( const EventTouch& event )
	{
		callPlayingMethod( std::bind( &Stage::onTouchMove, m_editedRoot, event ), [&]() { Super::onTouchMove( event ); });
	}
	
	void Editor::onTouchEnd( const EventTouch& event )
	{
		callPlayingMethod( std::bind( &Stage::onTouchEnd, m_editedRoot, event ), [&]() { Super::onTouchEnd( event ); });
	}
	
	void Editor::onWheelMove( const EventTouch& event )
	{
		callPlayingMethod( std::bind( &Stage::onWheelMove, m_editedRoot, event ), [&]() { Super::onWheelMove( event ); });
	}
	
	void Editor::openWithPrompt()
	{
		path openFilePath = Application::instance().getPromptedFilePath( false /* open */, ".xml" );
		if( !openFilePath.empty() )
		{
			openFile( openFilePath );
		}
	}
	
	void Editor::saveWithPromptIfNecessary()
	{
		path saveFilePath = m_currentDocumentPath;
		if( saveFilePath.empty() )
		{
			saveFilePath = Application::instance().getPromptedFilePath( true /* save */, ".xml" );
		}
		
		if( !saveFilePath.empty() )
		{
			saveToFile( saveFilePath );
		}
	}

	void Editor::onAppMayTerminate()
	{
		autoSave();
		
		saveSettings();
		
		Super::onAppMayTerminate();
	}
	
	void Editor::onStageLoaded()
	{
		Super::onStageLoaded();

		loadSettings();
		ASSERT( m_settings );

		if( virtualKeys() )
		{
			virtualKeys()->addEventListener( EventVirtualKey::VIRTUAL_KEY_DOWN, FRESH_CALLBACK( onVirtualKeyDown ));
			virtualKeys()->addEventListener( EventVirtualKey::VIRTUAL_KEY_UP, FRESH_CALLBACK( onVirtualKeyUp ));
		}
		
		// Enact initial settings.
		//		
		if( m_settings->reopenLastOnLaunch() && !m_settings->mostRecentFile().empty() )
		{
			openFile( m_settings->mostRecentFile() );
			
			if( !isOpen() )
			{
				createNew();
			}
		}
		else
		{
			createNew();
		}
	}
	
	void Editor::loadSettings()
	{
		ASSERT( !m_settingsDocumentPath.empty() );
		
		m_settings = nullptr;
		
		const path fullPath = getDocumentPath( m_settingsDocumentPath );
		
		if( exists( fullPath ))
		{
			Package::ptr settingsPackage = createPackage();
			settingsPackage->retainMembers();
			
			try
			{
				settingsPackage->loadFile( fullPath );
				m_settings = settingsPackage->find< EditorSettings >( "settings" );
			}
			catch( ... )
			{
				con_error( "Tried to load editor settings from '" << fullPath << "' but something went wrong. Clearing all settings." );
			}
		}
		
		if( !m_settings )
		{
			// Couldn't find settings. Probably our first time to run.
			//
			m_settings = createObject< EditorSettings >( "settings" );
		}
		
		PROMISES( m_settings );
	}
	
	void Editor::saveSettings()
	{
		REQUIRES( m_settings );
		ASSERT( !m_settingsDocumentPath.empty() );

		const auto& settingsPath = getDocumentPath( m_settingsDocumentPath );
		
		Package::ptr settingsPackage = createPackage();
		settingsPackage->add( m_settings );
		settingsPackage->save( settingsPath );
	}
	
	void Editor::saveHistoryState()
	{
		trace_history( this << ".saveHistoryState()" );

		std::shared_ptr< HistoryState > state = std::make_shared< HistoryState >();
		state->editedCurrent = m_editedCurrent->objectId();
		
		state->currentManifest.str("");
		saveState( state->currentManifest, true /* force saving all properties */ );

#if 1	// Save the current history state to an on-disk file so that we can inspect it precisely.
		{
			path fullPath( getDocumentPath( "~editor-state.xml" ));

			std::ofstream out;
			out.exceptions( std::ios::badbit | std::ios::failbit );
			
			try
			{
				create_directories( fullPath.parent_path() );
				out.open( fullPath.c_str() );
			}
			catch( const std::exception& e )
			{
				con_error( "Exception saving history state package to " << fullPath << ": " << e.what() );
				throw;
			}
			
			out << state->currentManifest.str();
		}
#endif

		m_changeHistory.addState( std::move( state ));
		
//		traceHistory();
	}
	
	void Editor::HistoryState::apply( Editor& editor ) const
	{
		trace_history( "HistoryState::apply()" );

		// Load the manifest.
		//
		DisplayObject::ptr newEditedCurrent = editor.editedPackage().find< DisplayObject >( editedCurrent );

		editor.loadState( currentManifest.str() );
		
		ASSERT( newEditedCurrent );
		editor.setEditedObject( newEditedCurrent );
	}
	
	void Editor::applyHistoryState( const std::shared_ptr< HistoryState >& state )
	{
		ASSERT( state );
		state->apply( *this );
	}

	void Editor::undo()
	{
		trace_history( this << ".undo()" );
		
		if( !m_manipulatorCurrent->isChanging() && m_changeHistory.canUndo() )
		{
			trace_history( this << ".undo() actually happening." );
			m_changeHistory.undo();

//			traceHistory();
		}
	}
	
	void Editor::redo()
	{
		trace_history( this << ".redo()" );
		
		if( !m_manipulatorCurrent->isChanging() && m_changeHistory.canRedo() )
		{
			trace_history( this << ".redo() actually happening." );
			m_changeHistory.redo();
			
//			traceHistory();
		}
	}
	
	void Editor::toggleHUDMinimized()
	{
		m_isHUDMinimized = !m_isHUDMinimized;
		forEachDescendant< UIPanel >( [&] ( UIPanel& panel )
									 {
										 if( m_isHUDMinimized )
										 {
											 panel.collapse();
										 }
										 else
										 {
											 panel.expand();
										 }
									 } );
	}

	ClassName Editor::manipulatorClassForClass( ClassNameRef forClass ) const
	{
		const ClassInfo::cptr classInfo = getClass( forClass );
		ASSERT( classInfo );
		
		// Walk through looking for the closest match.
		//
		int bestDepth = std::numeric_limits< int >::max();
		ClassName bestManipulatorClass( m_defaultManipulatorClass );
		
		for( const auto& pair : m_manipulatorClasses )
		{
			ClassInfo::cptr const possibleSubjectClass = getClass( pair.first );
			const int depth = classInfo->getSuperClassDepth( *possibleSubjectClass );
			
			if( depth >= 0 && depth < bestDepth )
			{
				bestDepth = depth;
				bestManipulatorClass = pair.second;
			}
		}
		
		return bestManipulatorClass;
	}
	
	void Editor::addManipulatorClass( ClassNameRef forClass, ClassNameRef manipulatorClass )
	{
		m_manipulatorClasses[ forClass ] = manipulatorClass;
	}

	FRESH_DEFINE_CALLBACK( Editor, onVirtualKeyDown, EventVirtualKey )
	{}
	
	FRESH_DEFINE_CALLBACK( Editor, onVirtualKeyUp, EventVirtualKey )
	{
		if( !keyboardFocusHolder() )
		{
			const auto& keyName = event.keyName();
			
			if( m_isEditedRootPlaying )
			{
				if( keyName == "StopPlaying" )
				{
					resumeEditing();
				}
			}
			else
			{				
				if( keyName == "FileNew" )
				{
					createNew();
				}
				else if( keyName == "FileNewFromTemplate" )
				{
					path openFilePath = Application::instance().getPromptedFilePath( false /* open */, ".xml" );
					if( !openFilePath.empty() )
					{
						createNewFromTemplate( openFilePath );
					}
				}
				else if( keyName == "FileSaveAs" )
				{
					path saveFilePath = Application::instance().getPromptedFilePath( true /* save */, ".xml" );
					if( !saveFilePath.empty() )
					{
						saveToFile( saveFilePath );
					}
				}
				else if( keyName == "FileSave" )
				{
					saveWithPromptIfNecessary();
				}
				else if( keyName == "FileOpen" )
				{
					openWithPrompt();
				}
				else if( keyName == "Undo" )
				{
					undo();
				}
				else if( keyName == "Redo" )
				{
					redo();
				}
				else if( keyName == "Cut" )
				{
					m_manipulatorCurrent->cutSelected();
				}
				else if( keyName == "Copy" )
				{
					m_manipulatorCurrent->copySelected();
				}
				else if( keyName == "Paste" )
				{
					m_manipulatorCurrent->paste();
				}
				else if( keyName == "Delete" )
				{
					m_manipulatorCurrent->deleteSelected();
				}
				else if( keyName == "SelectAll" )
				{
					m_manipulatorCurrent->selectAll();
				}
				else if( keyName == "SelectNone" )
				{
					m_manipulatorCurrent->deselectAll();
				}
				else if( keyName == "Play" )
				{
					play();
				}
				else if( keyName == "ToggleAllPanels" )
				{
					toggleHUDMinimized();
				}
			}
		}
	}

	WeakPtr< UIEditBox > Editor::beginObjectBrowserEditBoxModification()
	{
		pushActivePackage( m_editedPackage );
		return m_objectBrowserSelectionDestination;
	}
	
	void Editor::endObjectBrowserEditBoxModification()
	{
		if( m_objectBrowserSelectionDestination )
		{
			m_objectBrowserSelectionDestination->endEditing();
		}
		ASSERT( activePackage() == m_editedPackage );
		popActivePackage();
	}

	void Editor::traceHistory() const
	{
#ifdef TRACE_HISTORY
		trace( "Editor change history:" );
		trace( "# states: " << m_changeHistory.size() );
		trace( "-------------------------------------" );
		const size_t current = m_changeHistory.currentIndex();
		
		const size_t maxChars = 200;
		
		m_changeHistory.forEachState( [&]( std::shared_ptr< HistoryState > state, size_t index )
									 {
										 std::string stateString = state->currentManifest.str();
										 if( stateString.size() > maxChars )
										 {
											 stateString.resize( maxChars );
											 stateString += "...";
										 }
										 
										 trace( "   " << ( ( current == index ) ? "*" : " " ) << "[" << index << "]: \"" << stateString << "\"" );
									 } );
		
		trace( "-------------------------------------" );
#endif
	}
}
