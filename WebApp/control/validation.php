<?php
session_start(); // D�marage de la session
include('../config.php');

// R�cup�ration des variables n�cessaires � l'activation
$login = $_GET['log'];
$cle = $_GET['cle'];

/*
 * Connection a la base de donnee MySQL
 */
try
{
    // On se connecte � MySQL
    $pdo_options[PDO::ATTR_ERRMODE] = PDO::ERRMODE_EXCEPTION;
    $bdd = new PDO( $SQL_DSN, $SQL_USERNAME, $SQL_PASSWORD, $pdo_options);
    
    // R�cup�ration de la cl� correspondant au $login dans la base de donn�es
    $stmt = $bdd->prepare("SELECT validkey,enable FROM crv_users WHERE login like :login ");
    if($stmt->execute(array(':login' => $login)) && $row = $stmt->fetch()) 
      {
        $clebdd = $row['validkey'];	// R�cup�ration de la cl�
        $actif = $row['enable']; // $actif contiendra alors 0 ou 1
      }

    // On teste la valeur de la variable $actif r�cup�r� dans la BDD
    if($actif == '1') // Si le compte est d�j� actif on pr�vient
      {
         echo "Votre compte est d�j� actif !";
      }
    else // Si ce n'est pas le cas on passe aux comparaisons
      {
         if($cle == $clebdd) // On compare nos deux cl�s	
           {
              // Si elles correspondent on active le compte !	
              echo "Bonjour $login, votre compte a bien �t� activ� ! Vous pouvez revenir sur la page d'<a href=\"http://crv.ingenix.eu/control/index.php\">acceuil</a>";
    								
              // La requ�te qui va passer notre champ actif de 0 � 1
              $stmt = $bdd->prepare("UPDATE crv_users SET enable = 1 WHERE login like :login ");
              $stmt->bindParam(':login', $login);
              $stmt->execute();
           }
         else // Si les deux cl�s sont diff�rentes on provoque une erreur...
           {
              echo "Erreur ! Votre compte ne peut �tre activ�...";
           }
      }
}
catch(Exception $e)
{
    // En cas d'erreur pr�c�demment, on affiche un message et on arr�te tout
    die('Erreur : '.$e->getMessage());
}

