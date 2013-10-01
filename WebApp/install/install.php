<?php
include('../config.php');
try
{
    // On se connecte à MySQL
    $pdo_options[PDO::ATTR_ERRMODE] = PDO::ERRMODE_EXCEPTION;
    $bdd = new PDO($SQL_DSN, $SQL_USERNAME, $SQL_PASSWORD, $pdo_options);
		
		$cle = md5(microtime(TRUE)*100000);
		$admin = $ADMIN;

		/*************/
		/* CRV_USERS */
		/*************/
		$crv_users="
		CREATE TABLE IF NOT EXISTS `crv_users` (
		`id` int(11) NOT NULL AUTO_INCREMENT,
		`login` varchar(20) NOT NULL,
		`password` varchar(40) NOT NULL,
		`mail` varchar(50) NOT NULL,
		`key` varchar(100) DEFAULT NULL,
		`cert` varchar(100) DEFAULT NULL,
		`validkey` varchar(32) NOT NULL,
		`enable` int(11) NOT NULL,
		`date` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
		PRIMARY KEY (`id`)
		)";
		$req = $bdd->query("$crv_users");

		/**************/
		/* CREAT USER */
		/**************/
		/*
		$crv_create_user="
		INSERT INTO crv_users(id,login,password,mail,validkey,enable, date) 
		VALUES ('', ".$admin.", ".$sha1.", ".$mail.", ".$cle.", 1, NOW()
		)";
		echo $crv_create_user;
		$req = $bdd->query("$crv_create_user");
		*/


		/***************/
		/* CRV_SERVERS */
		/***************/
		$crv_servers="
		CREATE TABLE IF NOT EXISTS `crv_servers` (
		`id` int(10) NOT NULL AUTO_INCREMENT,
		`host` varchar(128) NOT NULL,
		`enable` int(1) NOT NULL,
		PRIMARY KEY (`id`)
		)";
		$req = $bdd->query("$crv_servers");

		/*************/
		/* CRV_ADMIN */
		/*************/
		$crv_admin="
		CREATE TABLE IF NOT EXISTS `crv_admin` (
		`id` int(11) NOT NULL AUTO_INCREMENT,
		`user` varchar(128) NOT NULL,
		`enable` int(11) NOT NULL,
		PRIMARY KEY (`id`)
		)";
		$req = $bdd->query("$crv_admin");

		/**************/
		/* CREAT ADMIN */
		/**************/
		$crv_create_admin="INSERT INTO `crv_admin` (`id`, `user`, `enable`) VALUES (NULL, '".$admin."', '1')";
		$req = $bdd->query("$crv_create_admin");

		/************/
		/* CRV_MODO */
		/************/
		$crv_modo="
		CREATE TABLE IF NOT EXISTS `crv_modo` (
		`id` int(11) NOT NULL AUTO_INCREMENT,
		`user` varchar(128) NOT NULL,
		`enable` int(11) NOT NULL,
		PRIMARY KEY (`id`)
		)";
		$req = $bdd->query("$crv_modo");


		/********************/
		/* CRV_MAIN_COMMENT */
		/********************/
		$crv_main_comment="
		CREATE TABLE IF NOT EXISTS `crv_main_comment` (
		`sha1` varchar(40) NOT NULL,
		`comment` text NOT NULL,
		`user` varchar(128) NOT NULL,
		`date_reply` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP
		)";
		$req = $bdd->query("$crv_main_comment");

		/****************/
		/* CRV_WEB_FILE */
		/****************/
		$crv_web_file="
		CREATE TABLE IF NOT EXISTS `crv_web_file` (
		`id` int(11) NOT NULL AUTO_INCREMENT,
		`sha1` varchar(40) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
		`size` int(11) NOT NULL,
		`name` text,
		`date_file` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
		PRIMARY KEY (`id`),
		UNIQUE KEY `sha1` (`sha1`)
		)";
		$req = $bdd->query("$crv_web_file");

		/*****************/
		/* CRV_MAIN_FILE */
		/*****************/
		$crv_main_file="
		CREATE TABLE IF NOT EXISTS `crv_main_file` (
		`id` int(11) NOT NULL AUTO_INCREMENT,
		`sha1` varchar(40) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
		`size` int(11) NOT NULL,
		`name` text,
		`date_file` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
		PRIMARY KEY (`id`),
		UNIQUE KEY `sha1` (`sha1`)
		) ENGINE=MRG_MyISAM INSERT_METHOD=LAST UNION=(`crv_web_file`);";
		$req = $bdd->query("$crv_main_file");

		/****************/
		/* CRV_WEB_CAT */
		/****************/
		$crv_web_cat="
		CREATE TABLE IF NOT EXISTS `crv_web_cat` (
		`sha1` varchar(40) NOT NULL DEFAULT '',
		`cat` varchar(128) NOT NULL,
		PRIMARY KEY (`sha1`)
		)";
		$req = $bdd->query("$crv_web_cat");


		/****************/
		/* CRV_MAIN_CAT */
		/****************/
		$crv_main_cat="
		CREATE TABLE IF NOT EXISTS `crv_main_cat` (
		`sha1` varchar(40) NOT NULL DEFAULT '',
		`cat` varchar(128) NOT NULL,
		PRIMARY KEY (`sha1`)
		) ENGINE=MRG_MyISAM INSERT_METHOD=LAST UNION=(`crv_web_cat`);";
		$req = $bdd->query("$crv_main_cat");
		


}
catch(Exception $e)
{
    // En cas d'erreur précédemment, on affiche un message et on arrête tout
    die('Erreur : '.$e->getMessage());
}
	echo "FINI";
?>
