<?php	
	
	require_once( './inc/error.inc.php' );
	require_once( './inc/db-access.inc.php' );

    if( isset( $_GET['name'] ))
    {    
        $contextName = urldecode( $_GET['name'] );
    }
    else
    {
        reportError( 'Context name required.' );
    }

    if( isset( $_GET['session'] ))
    {    
        $session = $_GET['session'];
    }
    else
    {
        reportError( 'Session id required.' );
    }

    if( isset( $_GET['parent'] ))
    {    
        $parentId = '\''.$_GET['parent'].'\'';
    }
    else
    {
        $parentId = 'NULL';
    }

   connectToDatabase();

    // Create the context.
    //
    mysql_query( 
        "INSERT INTO contexts (name, session, parent, enterTime) VALUES ('$contextName', '$session', $parentId, NOW() )" ) 
        or reportError( mysql_error() );

    echo mysql_insert_id();       // Return new context id.

?>