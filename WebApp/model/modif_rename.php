<?php

function rename_file($sha1, $filename)
{
	global $bdd;
	/*
		$sql = "UPDATE `crv_main_file` SET `name` = '$filename' WHERE `crv_main_file`.`sha1` = $sha1;";
	*/
	$req = $bdd->prepare("UPDATE `crv_main_file` SET `name` = '$filename' WHERE `crv_main_file`.`sha1` = '$sha1'");
	#$req->bindParam(':sha1sum1', $sha1, PDO::PARAM_STR);
  #$req->bindParam(':sha1sum2', $sha1, PDO::PARAM_STR);
	$req->execute();
	return ;
}
