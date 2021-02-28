<?php

	require_once( "config.inc.php" );

	function connectToDatabase()
	{
		global $dbHost, $dbUsername, $dbPassword, $dbName;
		
		// Connect to server and select database.
		mysql_connect( $dbHost, $dbUsername, $dbPassword ) or die( mysql_error() );
		mysql_select_db( $dbName ) or die( "Cannot select database." );
	}	
	
    
    function ordinalize( $tableName, $fieldName, $fieldValue, $getOnly = false )
    {
        // Find this entry.
        //
        $query = "SELECT id FROM $tableName WHERE `$fieldName` = '$fieldValue' LIMIT 1";
        $result = mysql_query( $query ) or die( mysql_error() );
        
        if( !$result || mysql_num_rows( $result ) == 0 )
        {
            if( !$getOnly )
            {
                // Create this entry.
                //
                $query = "INSERT INTO $tableName ( `$fieldName` ) VALUES ( '$fieldValue' )";
                mysql_query( $query ) or die( mysql_error() );

                $id = mysql_insert_id();                
            }
            else
            {
                $id = -1;   // Error -- doesn't exist and not allowed to create it.
            }
        }
        else
        {        
            $row = mysql_fetch_assoc( $result );
            $id = $row[ 'id' ];
        }
        
        return $id;
    }


    function ordinalize2( $tableName, $fieldName1, $fieldValue1, $fieldName2, $fieldValue2, $getOnly = false )
    {
        // Find this entry.
        //
        $result = mysql_query( "SELECT id FROM $tableName WHERE `$fieldName1` = '$fieldValue1' AND `$fieldName2` = '$fieldValue2' LIMIT 1" ) or die( mysql_error() );
        
        if( !$result || mysql_num_rows( $result ) == 0 )
        {
            if( !$getOnly )
            {
                // Create this entry.
                //
                mysql_query( "INSERT INTO $tableName ( `$fieldName1`, `$fieldName2` ) VALUES ( '$fieldValue1', '$fieldValue2' )" ) or die( mysql_error() );

                $id = mysql_insert_id();
            }
            else
            {
                $id = -1;   // Error -- doesn't exist and not allowed to create it.
            }
        }
        else
        {        
            $row = mysql_fetch_assoc( $result );
            $id = $row[ 'id' ];
        }
        
        return $id;
    }
?>