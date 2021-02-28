<?php

	// Install FreshTelemetry tables to the indicated database.


	require_once( './inc/error.inc.php' );
	require_once( './inc/db-access.inc.php' );

    connectToDatabase();

    mysql_query( "CREATE TABLE IF NOT EXISTS `contexts` ( `id` int(11) NOT NULL AUTO_INCREMENT, `name` varchar(64) NOT NULL, `session` int(11) NOT NULL, `parent` int(11) DEFAULT NULL, `enterTime` datetime NOT NULL, `exitTime` datetime DEFAULT NULL, PRIMARY KEY (`id`) ) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=9039 ;" );
	mysql_query( "CREATE TABLE IF NOT EXISTS `devices` (  `id` int(11) NOT NULL AUTO_INCREMENT,  `desc` varchar(64) NOT NULL,  PRIMARY KEY (`id`)) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=28 ;" );
	mysql_query( "CREATE TABLE IF NOT EXISTS `events` (  `id` int(11) NOT NULL AUTO_INCREMENT,  `session` int(11) NOT NULL,  `context` int(11) DEFAULT NULL,  `time` decimal(8,2) NOT NULL,  `type` varchar(64) NOT NULL,  `subject` int(11) DEFAULT NULL,  `comment` text,  `x` float DEFAULT NULL,  `y` float DEFAULT NULL,  `z` float DEFAULT NULL,  PRIMARY KEY (`id`)) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=64722 ;" );
	mysql_query( "CREATE TABLE IF NOT EXISTS `platforms` (  `id` int(11) NOT NULL AUTO_INCREMENT,  `desc` varchar(64) NOT NULL,  PRIMARY KEY (`id`)) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=9 ;" );
	mysql_query( "CREATE TABLE IF NOT EXISTS `sessions` (  `id` int(11) NOT NULL AUTO_INCREMENT,  `user` int(11) NOT NULL,  `address` varchar(16) NOT NULL,  `platform` int(11) NOT NULL,  `device` int(11) NOT NULL,  `version` int(11) NOT NULL,  `enterTime` datetime NOT NULL,  `exitTime` datetime DEFAULT NULL,  `resumedSession` int(11) DEFAULT NULL,  PRIMARY KEY (`id`)) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=3704 ;" );
	mysql_query( "CREATE TABLE IF NOT EXISTS `subjects` (  `id` int(11) NOT NULL AUTO_INCREMENT,  `type` varchar(64) NOT NULL,  `name` varchar(64) NOT NULL,  PRIMARY KEY (`id`)) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=10833 ;" );
	mysql_query( "CREATE TABLE IF NOT EXISTS `users` (  `id` int(10) NOT NULL AUTO_INCREMENT,  `name` varchar(32) NOT NULL,  PRIMARY KEY (`id`)) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=2 ;" );
	mysql_query( "CREATE TABLE IF NOT EXISTS `versions` (  `id` int(11) NOT NULL AUTO_INCREMENT,  `desc` varchar(128) NOT NULL,  PRIMARY KEY (`id`)) ENGINE=InnoDB  DEFAULT CHARSET=latin1 AUTO_INCREMENT=21 ;" );

?>

Done.