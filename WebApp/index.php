<?php
//Indique le niveau des erreurs
error_reporting(E_ALL | ~E_STRICT);

//Zone pour le calcul des dates
date_default_timezone_set("Europe/Paris");

//Calcul du chemin depuis la racine du site
$root=dirname(__FILE__).DIRECTORY_SEPARATOR;

// Chargement de la configuration
require_once("config.php");

//Classe de la bibliothèque
/*
require_once("");
require_once("");
require_once("");
*/


// Si on est en échappement auto, on annule
// les échappements pour que l'application soit indépendante de magic_quote_gpc
/*
if (get_magic_quotes_gpc() )
{
	$_POST = NormalisationHTTP ( $_POST ) ;
	$_GET = NormalisationHTTP ( $_GET ) ;
	$_REQUEST = NormalisationHTTP ($_REQUEST) ;
	$_COOKIE = NormalisationHTTP ( $_COOKIE) ;
}
*/

//indiquons si on affiche ou pas les serveurs, avec la constantes venant de config.php
//init_set("display_errors", DISPLAY_ERRORS);

header("Location: ./control/index.php");

?>
