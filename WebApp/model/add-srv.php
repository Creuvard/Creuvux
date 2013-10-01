<?php
session_start();
include('../config.php');
include('../model/mysql_connection.php');
if ($_SESSION["login"] == NULL)
{
	header("Location: ../control/index.php");	
}
else
{
	$login = $_SESSION["login"];
	$enable = $_SESSION["enable"];
}
$server = $_POST['server'];

/*
$sql = "INSERT INTO `crv`.`crv_servers` (`id`, `host`, `enable`) VALUES (NULL, \'creuvux.org\', \'0\');";
*/
$stmt = $bdd->prepare("INSERT INTO `crv_servers` (`id`, `host`, `enable`) VALUES (NULL, :srv, '1')");
$stmt->execute(array(':srv' => htmlspecialchars($server)));
header("Location: ../control/admin.php");
?>
