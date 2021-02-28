//
//  Editor.h
//  Fresh
//
//  Created by Jeff Wofford on 3/8/12.
//  Copyright (c) 2012 jeffwofford.com. All rights reserved.
//

#ifndef Fresh_Editor_h
#define Fresh_Editor_h

#include "Stage.h"
#include "PropertyPane.h"
#include "EdManipulator.h"
#include "EdClassInventory.h"
#include "ChangeHistory.h"
#include "DisplayPackage.h"

namespace fr
{

	class EditorSettings : public Object
	{
		FRESH_DECLARE_CLASS( EditorSettings, Object );
	public:
		
		SYNTHESIZE( bool, reopenLastOnLaunch )
		path mostRecentFile() const;		// May return an empty path. Otherwise the full path to the last addressed file.
		void onAddressingFile( const path& filePath );	// Full path.

	private:

		VAR( std::vector< path >, m_recentFiles );		// In oldest-to-newest order.
		DVAR( bool, m_reopenLastOnLaunch, true );
	};
	
	///////////////////////////////////////////////////////////////
	
	class EdObjectBrowser;
	class EdTimeline;
	class UIEditBox;
	
	class Editor : public Stage
	{
	public:
		
		virtual ~Editor();

		SYNTHESIZE_GET( PropertyPane::ptr, propertyPane );
		SmartPtr< EdClassInventory > classInventory() const;
		SYNTHESIZE_GET( SmartPtr< EdTimeline >, timeline );
		
		Stage::ptr root() const;
		DisplayObject::ptr base() const;
		DisplayObject::ptr current() const;
		
		Manipulator& currentManipulator() const					{ ASSERT( m_manipulatorCurrent ); return *m_manipulatorCurrent; }
		
		bool isOpen() const;
		virtual void createNew();
		virtual void createNewFromTemplate( const path& fullPathToTemplate, const ObjectId& editingBase = ObjectId::NULL_OBJECT );
		virtual void openWithPrompt();
		virtual void openFile( const path& fullPath );
		virtual void saveWithPromptIfNecessary();
		virtual void saveToFile( const path& fullPath );
		virtual void saveToDocument( path documentFilePath );
		virtual void autoSave() const;
		virtual void close();
		
		DisplayPackage& editedPackage() const					{ ASSERT( m_editedPackage ); return *m_editedPackage; }
		
		virtual void beginEditing( DisplayObject::ptr base );
		virtual void beginEditingNamedObject( const ObjectId& objectToEdit = ObjectId::NULL_OBJECT );
		
		void editParent();
		void editAncestor( DisplayObjectContainer::ptr ancestor );
		void editChild( DisplayObject::ptr child );

		SYNTHESIZE_GET( bool, isEditedRootPlaying )
		virtual void play();
		virtual void resumeEditing( bool retainPlayedState = false );
		
		virtual SmartPtr< EdObjectBrowser > openObjectBrowser( const ClassInfo& baseClass );
		
		void setToolbar( DisplayObject::ptr toolbar );
		
		SYNTHESIZE_GET( bool, isTouchActionInProgress )
		void beginTouchAction();
		void endTouchAction();
		
		virtual void update() override;
		virtual void render( TimeType relativeFrameTime, RenderInjector* injector = nullptr ) override;
		virtual void onTouchBegin( const EventTouch& event ) override;
		virtual void onTouchMove( const EventTouch& event ) override;
		virtual void onTouchEnd( const EventTouch& event ) override;
		virtual void onWheelMove( const EventTouch& event ) override;

		virtual void onAppMayTerminate() override;
		
		virtual void undo();
		virtual void redo();
		virtual void saveHistoryState();
		
		SYNTHESIZE( WeakPtr< UIEditBox >, objectBrowserSelectionDestination )
		
		virtual WeakPtr< UIEditBox > beginObjectBrowserEditBoxModification();
		virtual void endObjectBrowserEditBoxModification();
		
		void toggleHUDMinimized();
		
		// Manipulator class configuration.
		//
		ClassName manipulatorClassForClass( ClassNameRef forClass ) const;
		void addManipulatorClass( ClassNameRef forClass, ClassNameRef manipulatorClass );
		
	protected:
		
		virtual void createNewWithoutTemplate();
		
		virtual void setEditedObject( DisplayObject::ptr object );
		// REQUIRES( object != this );
		// REQUIRES( !object || object->hasStage() );
		// REQUIRES( !object || !hasDescendant( object ));

		vec2 screenToSubject( const vec2& screenLocation ) const;
		
		void establishEditedRoot();

		void callPlayingMethod( const std::function< void() >&& fnForRoot, const std::function< void() >&& fnForBaseClass );
		
		virtual void onStageLoaded() override;

		virtual void loadSettings();
		virtual void saveSettings();
		
		void showEditingHUD( bool show );

		void saveState( std::ostream& out, bool forceSaveAllProperties = false ) const;
		void loadState( const std::string& in );

		void getAutoSaveFileInformation( size_t& outNum, size_t& outMin, size_t& outMax, size_t& firstAvailable ) const;
		path autoSaveFilePathForIndex( size_t index ) const;
		void cleanupAutoSaves( size_t nSavesToKeep = 3 ) const;
		
		void traceHistory() const;
		
	private:
		
		typedef std::map< ClassName, ClassName > ManipulatorClasses;			// Maps subject class to manipulator class.
		
		DVAR( ClassInfo::cptr, m_newStageClass, &Stage::StaticGetClassInfo() );
		
		VAR( Stage::ptr, m_editedRoot );					// The stage of the edited display tree.
		VAR( DisplayObject::ptr, m_editedBase );			// The lowest object of the edited display tree to allow editing for.
		VAR( DisplayObject::ptr, m_editedCurrent );		// The object in the edited display tree currently being edited.
		
		VAR( Manipulator::ptr, m_manipulatorCurrent );
		VAR( PropertyPane::ptr, m_propertyPane );
		VAR( SmartPtr< EdClassInventory >, m_classInventory );
		VAR( DisplayObjectContainer::ptr, m_toolbarHost );
		VAR( SmartPtr< EdObjectBrowser >, m_objectBrowser );
		VAR( SmartPtr< EdTimeline >, m_timeline );
		
		DVAR( bool, m_isHUDMinimized, false );
		
		DVAR( ClassName, m_defaultManipulatorClass, "Manipulator" );
		VAR( ManipulatorClasses, m_manipulatorClasses );
		
		VAR( ClassInfo::Name, m_playingDisplayClass );
		
		VAR( path, m_defaultFileTemplate );

		DisplayPackage::ptr m_editedPackage;

		DisplayObject::ptr m_playingDisplay;
		
		WeakPtr< UIEditBox > m_objectBrowserSelectionDestination;
		
		DVAR( path, m_settingsDocumentPath, "editor-settings.xml" );
		EditorSettings::ptr m_settings;
		
		path m_currentDocumentPath;
		
		bool m_isEditedRootPlaying = false;
		
		bool m_isTouchActionInProgress = false;
		
		struct HistoryState
		{
			ObjectId editedCurrent;
			std::stringstream currentManifest;
			
			void apply( Editor& editor ) const;
		};
		
		ChangeHistory< std::shared_ptr< HistoryState >> m_changeHistory;
		
		void applyHistoryState( const std::shared_ptr< HistoryState >& state );
		
		FRESH_DECLARE_CALLBACK( Editor, onVirtualKeyDown, EventVirtualKey );
		FRESH_DECLARE_CALLBACK( Editor, onVirtualKeyUp, EventVirtualKey );
		
		FRESH_DECLARE_CALLBACK( Editor, onTimeToAutosave, Event );
		
		
		FRESH_DECLARE_CLASS( Editor, Stage )
	};
	
}

#endif
