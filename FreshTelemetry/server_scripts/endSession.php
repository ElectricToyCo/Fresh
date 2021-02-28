<?php	
	
	require_once( './inc/error.inc.php' );
	require_once( './inc/db-access.inc.php' );
	
	// Ensure that required values are set.
	//
	if( !isset( $_GET['session'] ))
	{
		reportError( 'Session id required.' );
	}
    
    // Read values.
    //
	$session = $_GET['session'];
    
    // Update the session.
    //
    connectToDatabase();

    mysql_query( 'UPDATE sessions SET exitTime = NOW() WHERE id = '.$session ) or reportError( mysql_error() );

    echo mysql_affected_rows();

?>