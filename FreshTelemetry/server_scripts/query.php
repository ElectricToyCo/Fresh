<html><head><title>Game Telemetry</title></head>
<body>

<?php	
	
	require_once( './inc/error.inc.php' );
	require_once( './inc/db-access.inc.php' );

    $session = '%';
    if( isset( $_GET['session'] ))
    {
		$session = $_GET['session'];
    }
    
    $context = '%';
    if( isset( $_GET['context'] ))
    {
		$context = $_GET['context'];
    }
    
    $format = 'Mathematica';
    if( isset( $_GET['format'] ))
    {
		$format = $_GET['format'];
        
        // TODO verify validity
    }
    
    $type = '%';
    if( isset( $_GET['type'] ))
    {
        $type = urldecode($_GET['type']);
    }
    
    
    connectToDatabase();

    // Find the user.
    //
    $result = mysql_query( "SELECT * FROM events WHERE session LIKE '$session' AND context LIKE '$context' AND type LIKE '$type' ORDER BY `time` ASC" );
    
    $nRows = mysql_num_rows( $result );
    
    echo '{';
    for( $iRow = 0; $iRow < $nRows; ++$iRow )
    {
        $row = mysql_fetch_assoc( $result );
        
        echo '{'.$row['session'];
        echo ', '.$row['context'];
        echo ', '.$row['time'];
        echo ', "'.$row['type'].'"';
        echo ', '.$row['x'];
        echo ', '.$row['y'];
        echo '}';
        
        if( $iRow + 1 < $nRows )
        {
            echo ',';
            echo '<br />';
        }
        
    }
    echo '}';

?>


</body>
</html>