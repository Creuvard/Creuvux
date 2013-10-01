<?php

function get_list_clients()
{
		//include_once('../model/mysql_connection.php');
		
		global $bdd;
    $req = $bdd->prepare('SELECT `id`,`login`,`mail` FROM `crv_users` ');
    $req->execute();
    $users = $req->fetchAll();
    
    return $users;
}
