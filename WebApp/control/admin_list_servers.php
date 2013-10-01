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
	require_once("./if_admin_access.php");
}

		
		// Récupération de la valeur du champ actif pour le login $login
		$file = $bdd->query("SELECT `id`,`host`,`enable` FROM `crv_servers`");
		while ($data = $file->fetch())
		{
			if ($data != NULL)
			{	
				$id = $data['id'];
				$host = $data['host'];
				$enable = $data['enable'];
				
				echo "<tr class=\"odd\">";
        echo "<td><input class=\"checkbox\" name=\"id\" value=\"$id\" type=\"checkbox\"></td>";
				echo "<td>$id</td>";
				echo "<td>$host</td>";
				if ($enable == 1)
					echo "<td><img src=\"./images/tick.png\" alt=\"Yes\"></td>";
				else
					echo "<td><img src=\"./images/cross.png\" alt=\"No\"></td>";
				
				echo "<td class=\"last\"><a href=\"#\">edit</a></td>";
        echo "</tr>";

			}
		}
		//$file->closeCursor();
?>
