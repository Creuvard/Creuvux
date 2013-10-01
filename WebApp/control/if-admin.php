<?php
if ($_SESSION["login"] == NULL)
{
	header("Location: ./index.php");	
}
else
{
	$login = $_SESSION["login"];
	$enable = $_SESSION["enable"];
	$tabname = $_SESSION["tabname"];
}


function ifadmin()
{
	include('../model/mysql_connection.php');
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
