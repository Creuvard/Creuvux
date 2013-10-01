<?php
session_start(); // Démarage de la session
include('../config.php');

// Récupération des variables nécessaires à l'activation
$login = $_GET['log'];
$cle = $_GET['cle'];

/*
 * Connection a la base de donnee MySQL
 */
try
{
    // On se connecte à MySQL
    $pdo_options[PDO::ATTR_ERRMODE] = PDO::ERRMODE_EXCEPTION;
    $bdd = new PDO( $SQL_DSN, $SQL_USERNAME, $SQL_PASSWORD, $pdo_options);
    
    // Récupération de la clé correspondant au $login dans la base de données
    $stmt = $bdd->prepare("SELECT validkey,enable FROM crv_users WHERE login like :login ");
    if($stmt->execute(array(':login' => $login)) && $row = $stmt->fetch()) 
      {
        $clebdd = $row['validkey'];	// Récupération de la clé
        $actif = $row['enable']; // $actif contiendra alors 0 ou 1
      }

    // On teste la valeur de la variable $actif récupéré dans la BDD
    if($actif == '1') // Si le compte est déjà actif on prévient
      {
         echo "Votre compte est déjà actif !";
      }
    else // Si ce n'est pas le cas on passe aux comparaisons
      {
         if($cle == $clebdd) // On compare nos deux clés	
           {
              // Si elles correspondent on active le compte !	
              echo "Bonjour $login, votre compte a bien été activé ! Vous pouvez revenir sur la page d'<a href=\"http://crv.ingenix.eu/control/index.php\">acceuil</a>";
    								
              // La requête qui va passer notre champ actif de 0 à 1
              $stmt = $bdd->prepare("UPDATE crv_users SET enable = 1 WHERE login like :login ");
              $stmt->bindParam(':login', $login);
              $stmt->execute();
           }
         else // Si les deux clés sont différentes on provoque une erreur...
           {
              echo "Erreur ! Votre compte ne peut être activé...";
           }
      }
}
catch(Exception $e)
{
    // En cas d'erreur précédemment, on affiche un message et on arrête tout
    die('Erreur : '.$e->getMessage());
}

