<?php

function del_tab($tabname, $sha1)
{
	include('../model/mysql_connection.php');
	$sql_tab = "DELETE FROM `$tabname` WHERE `$tabname`.`sha1` like '$sha1'";
	$file = $bdd->query("$sql_tab");

	$file->closeCursor();
	return;
}
