<?php
include('../model/mysql_connection.php');
if ($_SESSION["login"] == NULL)
{
	header("Location: ../control/index.php");	
}


function ifadmin()
{
	include('../model/mysql_connection.php');
	$login = $_SESSION["login"];
	$result=-1;
	$file = $bdd->query("SELECT `user` FROM `crv_admin` WHERE `user` like '$login'");
	while ($data = $file->fetch())
	{
		if ($data['user'] != NULL)
			$result=0;
	}
	$file->closeCursor();

	return $result;
}



?>
