<?php

function get_name_with_sha1sum($sha1)
{
	include('../model/mysql_connection.php');
	$filename = '';
	$sql_cat = "SELECT name FROM `crv_web_file` WHERE sha1 like '".$sha1."'";
	$file = $bdd->query("$sql_cat");
	while ($data = $file->fetch())
	{
		$filename=$data['name'];
	}
	$file->closeCursor();
	return $filename;
}
?>
