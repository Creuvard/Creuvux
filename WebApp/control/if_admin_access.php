<?php
include('../model/mysql_connection.php');

	// Récupération de la valeur du champ actif pour le login $login
	$enable_admin = -1;
	$file = $bdd->query("SELECT `user` FROM `crv_admin` WHERE `user` like '$login'");
	while ($data = $file->fetch())
	{
		if ($data['user'] != NULL)
			$enable_admin = 1;
	}
	$file->closeCursor();
	if ($enable_admin != 1)
		header('Location: ./main.php');
?>
