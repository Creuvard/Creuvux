<?php

function info_comment_file($sha1)
{
	global $bdd;
	$req = $bdd->prepare("SELECT `crv_main_comment`.`comment`,`crv_main_comment`.`user`,`crv_main_comment`.`date_reply` FROM `crv_main_comment` WHERE `crv_main_comment`.`sha1` LIKE '$sha1' ORDER BY `date_reply` ASC");
	#$req->bindParam(':sha1sum1', $sha1, PDO::PARAM_STR);
  #$req->bindParam(':sha1sum2', $sha1, PDO::PARAM_STR);
	$req->execute();
	$comments = $req->fetchAll();
	return $comments;
}
