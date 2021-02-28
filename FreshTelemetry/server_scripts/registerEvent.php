<?php	
	
	require_once( './inc/error.inc.php' );
	require_once( './inc/db-access.inc.php' );

    if( isset( $_GET['session'] ))
    {    
        $session = '\''.$_GET['session'].'\'';
    }
    else
    {
        reportError( 'Event session required.' );
    }

    if( isset( $_GET['time'] ))
    {    
        $time = '\''.$_GET['time'].'\'';
    }
    else
    {
        reportError( 'Event time required.' );
    }

    if( isset( $_GET['type'] ))
    {    
        $type = '\''.urldecode( $_GET['type'] ).'\'';
    }
    else
    {
        reportError( 'Event type required.' );
    }

    if( isset( $_GET['context'] ))
    {    
        $context = '\''.$_GET['context'].'\'';
    }
    else
    {
        $context = 'NULL';
    }

    if( isset( $_GET['comment'] ))
    {    
        $comment = '\''.urldecode( $_GET['comment'] ).'\'';
    }
    else
    {
        $comment = 'NULL';
    }

    if( isset( $_GET['x'] ))
    {    
        $x = '\''.$_GET['x'].'\'';
    }
    else
    {
        $x = 'NULL';
    }

    if( isset( $_GET['y'] ))
    {    
        $y = '\''.$_GET['y'].'\'';
    }
    else
    {
        $y = 'NULL';
    }

    if( isset( $_GET['z'] ))
    {    
        $z = '\''.$_GET['z'].'\'';
    }
    else
    {
        $z = 'NULL';
    }

    connectToDatabase();

    if( isset( $_GET['subjectType'] ) && isset( $_GET['subjectName'] ))
    {    
        $subjectType = urldecode( $_GET['subjectType'] );
        $subjectName = urldecode( $_GET['subjectName'] );
        
        $subjectId = ordinalize2( 'subjects', 'type', $subjectType, 'name', $subjectName );
        $subjectId = '\''.$subjectId.'\'';
    }
    else
    {
        $subjectId = 'NULL';
    }

    // Create the event.
    //
    mysql_query( 
        'INSERT INTO events (session, context, time, type, subject, comment, x, y, z) VALUES '.
        "($session, $context, $time, $type, $subjectId, $comment, $x, $y, $z )" ) 
        or reportError( mysql_error() );

    echo mysql_insert_id();       // Return new event id.

?>