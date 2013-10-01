<?php
	session_start();
	include('../config.php');
	if ($_SESSION["login"] == NULL)
	{
		header("Location: ../control/index.php");
	}
	else
	{
		$login = $_SESSION["login"];
		$enable = $_SESSION["enable"];
		$tabname = $_SESSION["tabname"];
		include_once('../model/mysql_connection.php');
		
		if (isset($_GET['sha1']))
		{
			include_once('../model/del_tab.php');
			$sha1 = $_GET['sha1'];
		}
		else
		{
			header("Location: ../control/main.php");
		}
		del_tab($tabname, $sha1);
		header("Location: ../control/main.php");

	}
?>


