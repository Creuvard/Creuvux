<?php
session_start();
include('../config.php');
include('../model/mysql_connection.php');
	if (!isset($_SESSION["login"]))
	{	
		header("Location: ../control/index.php");
	}
	else if (isset($_SESSION["login"]))
	{
		$login = $_SESSION["login"];
		$enable = $_SESSION["enable"];
		$tabname = $_SESSION["tabname"];
		//DROP TABLE crv_pseudo_validekey
		$stmt = $bdd->query("DROP TABLE `$tabname`");
		session_destroy();
		header("Location: ./index.php");
	}
?>
