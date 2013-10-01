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
if (isset($_POST['id']))
{
	$id = $_POST['id'];
	$ids = implode(' OR `id` =  ',$id);
}
else
	header("Location: ../control/admin.php");

//echo $ids;
/*
SELECT host
FROM `crv_servers`
WHERE `id` =32
OR `id` =33
OR `id` =34
*/

$req="SELECT host
FROM `crv_servers`
WHERE `id` = ".$ids;
echo $req;

/*

sur tous les serveurs de la liste on passe le enable à 0 afin de pouvoir sortir des serveurs pour x ou y raison.
On recupère le nom des serveurs, on concatène tous ca dans les commandes SQL ci-dessous afin de créer les commandes merges


*/
/*****************/
/* CRV_MAIN_FILE */
/*****************/
/*
$crv_main_file="
CREATE TABLE IF NOT EXISTS `crv_main_file` (
`id` int(11) NOT NULL AUTO_INCREMENT,
`sha1` varchar(40) CHARACTER SET utf8 COLLATE utf8_bin NOT NULL,
`size` int(11) NOT NULL,
`name` text,
PRIMARY KEY (`id`),
UNIQUE KEY `sha1` (`sha1`)
) ENGINE=MRG_MyISAM DEFAULT CHARSET=utf8 INSERT_METHOD=LAST UNION=(`crv_127.0.0.1`,`crv_creuvux.org`);";
$req = $bdd->query("$crv_main_file");
*/

/****************/
/* CRV_MAIN_CAT */
/****************/
/*
$crv_main_cat="
CREATE TABLE IF NOT EXISTS `crv_main_cat` (
`sha1` varchar(40) NOT NULL DEFAULT '',
`cat` varchar(128) NOT NULL,
PRIMARY KEY (`sha1`)
) ENGINE=MRG_MyISAM DEFAULT CHARSET=utf8 INSERT_METHOD=LAST UNION=(`crv_127.0.0.1_cat`,`crv_creuvux.org_cat`);
";
$req = $bdd->query("$crv_main_cat");
*/

/*
$file = $bdd->query($req);
$file->closeCursor();
header("Location: ../control/admin.php");
*/
?>
