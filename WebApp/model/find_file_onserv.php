<?php

function is_on_web_serv($sha1)
{
	include('../model/mysql_connection.php');
	$result = '-1';
	$sql_cat = "SELECT * FROM `crv_web_file` WHERE sha1 like '".$sha1."'";
	$file = $bdd->query("$sql_cat");
	while ($data = $file->fetch())
	{
		$result='0';
	}
	$file->closeCursor();
	return $result;
}
?>
