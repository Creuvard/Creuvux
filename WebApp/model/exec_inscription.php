<?php
session_start(); // D�marage de la session
include('../config.php');

// R�cup�ration des variables n�cessaires a l'inscription et au mail de confirmation
// Penser � faire les v�rifications sur les diff�rentes valeur entr�es
$email = $_POST['email'];
$login = $_POST['login'];
$mdp   = $_POST['password'];


// G�n�ration al�atoire d'une cl�
$cle = md5(microtime(TRUE)*100000);


/*
 * Connection a la base de donnee MySQL
 */
try
{
    // On se connecte � MySQL
    $pdo_options[PDO::ATTR_ERRMODE] = PDO::ERRMODE_EXCEPTION;
    $bdd = new PDO($SQL_DSN, $SQL_USERNAME, $SQL_PASSWORD, $pdo_options);
    
    // On ecrit dans la base de donnee les login,password et email du client
    $req = $bdd->prepare("INSERT INTO crv_users(id,login,password,mail,validkey,enable, date) VALUES ('', :pseudo, :mdp, :mail, :key, 0, NOW())");
    $req->execute(array(
      'pseudo' => $login,
      'mail' => $email,
      'mdp' => SHA1($mdp),
      'key' => $cle
     ));

    /*******************/
    /*  Envoie du Mail */
    /*******************/
    // Pr�paration du mail contenant le lien d'activation
    $destinataire = $email;
    $sujet = "Activation de votre compte Creuvux" ;
    $entete = "From: no-reply@creuvux.org" ;

    // Le lien d'activation est compos� du login(log) et de la cl�(cle)
    $message = 'Bienvenue sur le r�seau CREUVUX,
    
    Pour activer votre compte, veuillez cliquer sur le lien ci dessous
    ou copier/coller dans votre navigateur internet.
    
    http://www.crv.ingenix.eu/control/validation.php?log='.urlencode($login).'&cle='.urlencode($cle).'
    
    
    ---------------
    Ceci est un mail automatique, Merci de ne pas y r�pondre.';
    
    mail($destinataire, $sujet, $message, $entete) ; // Envoi du mail
    /* Fin Envoie Mail */
   
    //echo 'Vous allez recevoir un mail de confirmation pour votre inscription';
		header('Location: ../control/index.php?comment=mail');
  
}
catch(Exception $e)
{
    // En cas d'erreur pr�c�demment, on affiche un message et on arr�te tout
    die('Erreur : '.$e->getMessage());
}


?>
