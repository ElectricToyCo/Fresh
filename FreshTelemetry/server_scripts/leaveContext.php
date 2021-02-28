<?php	
	
	require_once( './inc/error.inc.php' );
	require_once( './inc/db-access.inc.php' );

    if( isset( $_GET['context'] ))
    {    
        $context = $_GET['context'];
    }
    else
    {
        reportError( 'Context id required.' );
    }

    connectToDatabase();

    mysql_query( 'UPDATE contexts SET exitTime = NOW() WHERE id = '.$context ) or reportError( mysql_error() );

    echo mysql_affected_rows();

?>