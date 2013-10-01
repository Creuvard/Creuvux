<?php
	session_start();
	include('../config.php');
	if ($_SESSION["login"] == NULL)
	{
		header("Location ../control/index.php");
	}
	else
	{
		$login = $_SESSION["login"];
		$enable = $_SESSION["enable"];
		include_once('../model/mysql_connection.php');
		
		if (isset($_POST['comment']) AND isset($_POST['sha1']))
		{
			include_once('../model/add_msg.php');
			$comment = $_POST['comment'];
			$sha1 = $_POST['sha1'];
			add_comment ($sha1, $comment, $login);
		}
	}
	header("Location: ../control/info.php?sha1=$sha1");
?>


