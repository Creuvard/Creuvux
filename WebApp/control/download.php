<?php
	session_start();
	include('../config.php');
	if ($_SESSION["login"] == NULL)
	{
		header("Location ../control/index.php");
	}
	else
	{
		include_once('../model/find_file_onserv.php');
		include_once('../model/get_name_with_sha1.php');
		$login = $_SESSION["login"];
		$enable = $_SESSION["enable"];
		$sha1 = $_GET['sha1'];
		$result=is_on_web_serv($sha1);
		if ($result == 0)
		{
			$filename = get_name_with_sha1sum($sha1);
			$location = "Location http://localhost/crv2/uploads/".$filename;
			//echo $location;
			header('Status: 301 Moved Permanently', false, 301);
			header($location);

			//$goto ="\"Location ../uploads/".$filename."\""; 
			//header($goto);	
			//header("Location ../control/index.php");
			//exit();
		}
		else
		{
			header('Status: 301 Moved Permanently', false, 301); 
			header("Location ../control/index.php");
		}
	}
	exit();
?>
