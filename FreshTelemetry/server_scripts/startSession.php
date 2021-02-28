<?php	
	
	require_once( './inc/error.inc.php' );
	require_once( './inc/db-access.inc.php' );

    connectToDatabase();

    if( !isset( $_GET['platform'] ))
    {
		reportError( 'Platform information required.' );
    }
    
    $platform = ordinalize( 'platforms', 'desc', $_GET['platform'] );
    
    if( !isset( $_GET['device'] ))
    {
		reportError( 'Device information required.' );
    }
    
    $device = ordinalize( 'devices', 'desc', $_GET['device'] );
    
    if( !isset( $_GET['version'] ))
    {
		reportError( 'Version information required.' );
    }
    
    $version = ordinalize( 'versions', 'desc', $_GET['version'] );
    
    if( isset( $_GET['username'] ))
    {    
        $userName = urldecode( $_GET['username'] );

        $userId = ordinalize( 'users', 'name', $userName, true );

        // Find the user.
        //
        if( $userId < 0 )
        {
            reportError( "Unrecognized user name '$userName'." );
        }        
        
        $resumeSession = "NULL";
    }
    else if( isset( $_GET['resumeSession'] ))
    {
        $resumeSession = $_GET['resumeSession'];
        
        $result = mysql_query( "SELECT * FROM sessions WHERE id='$resumeSession' LIMIT 1" );
        if( mysql_num_rows( $result ) == 0 )
        {
            reportError( "Unrecognized session id '$resumeSession'." );
        }
        
        $row = mysql_fetch_assoc( $result );
        
        $userId = $row[ 'user' ];
        
        // Stick tick marks around the $resumeSession to make insertion more uniform with NULL (which doesn't use them).
        //
        $resumeSession = '\''.$resumeSession.'\'';
    }
    else
    {
		reportError( 'User name or resuming session id required.' );
    }
    
    // Find the client address.
    //
    $ip = $_SERVER['REMOTE_ADDR'];
    
    // Create the session.
    //
    $result = mysql_query( 
        "INSERT INTO sessions (user, address, platform, device, version, enterTime, resumedSession) VALUES ('$userId', '$ip', '$platform', '$device', '$version', NOW(), $resumeSession )" ) 
        or reportError( mysql_error() );

    echo mysql_insert_id();       // Return new session id.

?>