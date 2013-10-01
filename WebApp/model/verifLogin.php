<?php
session_start(); // Démarage de la session
include('../config.php');
include('../model/mysql_connection.php');

// Récupération des variables nécessaires a l'inscriptio et au mail de confirmation
// Penser à faire les vérifications sur les différentes valeur entrées
$login = $_POST['login'];
$mdp   = $_POST['password'];
$actif = 0;


    // On se connecte à MySQL
    //$pdo_options[PDO::ATTR_ERRMODE] = PDO::ERRMODE_EXCEPTION;
    //$bdd = new PDO($SQL_DSN, $SQL_USERNAME, $SQL_PASSWORD, $pdo_options);
    // Récupération de la valeur du champ actif pour le login $login
    $stmt = $bdd->prepare("SELECT enable, password,validkey FROM crv_users WHERE login like :login ");
    if($stmt->execute(array(':login' => $login))  && $row = $stmt->fetch())
      {
       	$actif = $row['enable'];        // $actif contiendra alors 0 ou 1
        $password = $row['password'];
				$validkey = $row['validkey'];
      }
    		
    		
    // Il ne nous reste plus qu'à tester la valeur du champ 'actif' pour
    // autoriser ou non le membre à se connecter
    		
    if($actif == '1') // Si $actif est égal à 1, on autorise la connexion
      {
        if(SHA1($mdp) == $password)
				{
        	// On le met des variables de session pour stocker le nom de compte et le mot de passe :
        	$_SESSION["login"] = $login;
        	$_SESSION["password"] = $mdp;
        	$_SESSION["enable"] = '1';
					/*
					CREATE TABLE IF NOT EXISTS `crv_creuvard_validkey` (
					`id` int(11) NOT NULL AUTO_INCREMENT,
					`filename` varchar(128) NOT NULL,o such file or directory in /var/www/localhost/htdocs/crv2/
					PRIMARY KEY (`id`)
					)
					*/
					$tabname = 'crv_'.$login.'_'.$validkey;
					$_SESSION["tabname"] = $tabname;
					$stmt = $bdd->query("CREATE TABLE IF NOT EXISTS `$tabname` (`id` int(11) NOT NULL AUTO_INCREMENT, `filename` varchar(128) NOT NULL, `sha1` varchar(40) NOT NULL, PRIMARY KEY (`id`))");
					
					header('Location: ../control/main.php');

				}
				else 
				{
					header('Location: ../control/index.php');
				}
      }
    else // Sinon la connexion est refusé...
      {
       echo "Non identifié";
			 header('Location: ../control/index.php');
       //...
       // On refuse la connexion et/ou on previent que ce compte n'est pas activé
       //...
      }
  
?>
