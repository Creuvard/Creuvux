<?php

function info_file($sha1)
{
	global $bdd;
	$req = $bdd->prepare("SELECT `crv_main_file`.`name`, `crv_main_file`.`size` , `crv_main_file`.`sha1` , `crv_main_cat`.`cat` FROM `crv_main_file` , `crv_main_cat` WHERE `crv_main_file`.`sha1` LIKE '$sha1' AND `crv_main_cat`.`sha1` LIKE '$sha1'");
	#$req->bindParam(':sha1sum1', $sha1, PDO::PARAM_STR);
  #$req->bindParam(':sha1sum2', $sha1, PDO::PARAM_STR);
	$req->execute();
	$files = $req->fetchAll();
	return $files;
}
