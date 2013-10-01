<?php
include('../model/mysql_connection.php');
if ($_SESSION["login"] == NULL)
{
	header("Location: ../control/index.php");	
}
else
{
	$login = $_SESSION["login"];
	$enable = $_SESSION["enable"];
	$tabname = $_SESSION["tabname"];
}

	// Récupération de la valeur du champ actif pour le login $login
	$file = $bdd->query('SELECT DISTINCT `cat` FROM `crv_main_cat` ORDER BY `cat` DESC');
	echo "<b>Choix de la Categorie:</b> <select name=\"cat\">";
	while ($data = $file->fetch())
	{
	?>
		<option value="<?php echo $data['cat']; ?>"> <?php echo $data['cat']; ?></option>
	<?php
	}
	echo "</select><br />";

	$file->closeCursor();
?>
