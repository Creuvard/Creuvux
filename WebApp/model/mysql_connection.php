<?php
include('../config.php');

try
{
	// On se connecte à MySQL
	$pdo_options[PDO::ATTR_ERRMODE] = PDO::ERRMODE_EXCEPTION;
	$bdd = new PDO($SQL_DSN, $SQL_USERNAME, $SQL_PASSWORD, $pdo_options);
}

catch(Exception $e)
{
	// En cas d'erreur précédemment, on affiche un message et on arrête tout
	die('Erreur : '.$e->getMessage());
}

?>
