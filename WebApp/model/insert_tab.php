<?php

function add_tab($tabname, $sha1, $filename)
{
	include('../model/mysql_connection.php');
	$sql_tab = "INSERT INTO `$tabname` (`id`, `filename`, `sha1`) VALUES (NULL, \"$filename\", \"$sha1\")";
	$file = $bdd->query("$sql_tab");

	$file->closeCursor();
	return;
}
