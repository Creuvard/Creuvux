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

	// Récupération de la valeur du champ actif pour le login $login
	$file = $bdd->query('SELECT DISTINCT `cat` FROM `crv_main_cat` ORDER BY `cat` DESC');
	echo "<form method=\"GET\" action=\"main.php\"> <br />";
	echo "<b>Choix de la Categorie:</b> <select name=\"cat\"><br />";
	while ($data = $file->fetch())
	{
	?>
		<option value="<?php echo $data['cat']; ?>"> <?php echo $data['cat']; ?></option>
	<?php
	}
	echo "</select>";
	echo "<input type=\"submit\">";
	echo "</form>";

	$file->closeCursor();
?>
