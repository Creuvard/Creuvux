<?php

function add_fullfile($sha1, $filename, $comment, $newcat, $size, $login)
{
include('../model/mysql_connection.php');
/*
SELECT `crv_127.0.0.1`.`name` , `crv_127.0.0.1`.`size` , `crv_127.0.0.1_cat`.`cat`
FROM `crv_127.0.0.1` , `crv_127.0.0.1_cat`
WHERE `crv_127.0.0.1_cat`.`cat` LIKE 'Musique'
AND `crv_127.0.0.1_cat`.`sha1` LIKE `crv_127.0.0.1`.`sha1`
*/
	$sql_name = "INSERT INTO `crv_web_file` (`id`, `sha1`, `size`, `name`) VALUES (NULL, '".$sha1."', '".$size."', '".$filename."');";
	$file = $bdd->query("$sql_name");

	$sql_cat = "INSERT INTO `crv_web_cat` (`sha1`, `cat`) VALUES ('".$sha1."', '".$newcat."');";
	$file = $bdd->query("$sql_cat");

	$sql_comment = "INSERT INTO `crv_main_comment` (`sha1`, `comment`, `user`, `date_reply`) VALUES ('".$sha1."', '".$comment."', '".$login."', CURRENT_TIMESTAMP);";
	$file = $bdd->query("$sql_comment");
	$file->closeCursor();
	return;
}
