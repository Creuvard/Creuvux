<?php
		
function add_comment($sha1, $comment, $login)
{
		global $bdd;
    $stmt = $bdd->prepare("INSERT INTO `crv_main_comment` (`sha1` ,`comment`, `user`, `date_reply`)VALUES ( :sha1_sum, :comment, :username, NOW());");
    $stmt->execute(array(
			':sha1_sum' => $sha1,
			':comment' => $comment,
			':username' => $login
			/*
			':sha1_sum' => htmlspecialchars($sha1),
			':comment' => nl2br(htmlspecialchars($comment)),
			':username' => htmlspecialchars($login)
			*/
		));
 		return;   		
}
?>
