<?php
include('../model/mysql_connection.php');
if ($_SESSION["login"] == NULL)
{
	header("Location: ./index.php");	
}
else
{
	$login = $_SESSION["login"];
	$enable = $_SESSION["enable"];
	$tabname = $_SESSION["tabname"];
}

/*
SELECT `crv_127.0.0.1`.`name` , `crv_127.0.0.1`.`size` , `crv_127.0.0.1_cat`.`cat`
FROM `crv_127.0.0.1` , `crv_127.0.0.1_cat`
WHERE `crv_127.0.0.1_cat`.`cat` LIKE 'Musique'
AND `crv_127.0.0.1_cat`.`sha1` LIKE `crv_127.0.0.1`.`sha1`
*/

	// Récupération de la valeur du champ actif pour le login $login

	// SELECT * FROM `crv_creuvard_123456789`
	$file = $bdd->query("SELECT DISTINCT `filename`,`sha1` FROM `$tabname` ORDER BY id DESC LIMIT 0 ,3");
	while ($data = $file->fetch())
	{
	?>
		<li><a href="./info.php?sha1=<?php echo $data['sha1']; ?>"><?php echo $data['filename']; ?></a></li>
	<?php
	}
	echo "<li><a href=\"../control/upload.php\">Upload</a></li>";
	echo "<li><a href=\"../control/faq.php\">Foire aux questions</a></li>";
	$file->closeCursor();
?>
