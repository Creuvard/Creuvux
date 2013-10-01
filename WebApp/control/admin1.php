<?php
	session_start();
	include('../config.php');
	
	/*
	if (!isset($_SESSION["login"]))
		echo "Variable vide";
	else if (isset($_SESSION["login"]))
		echo "Variable pas vide";
	*/

	if (!isset($_SESSION["login"]))
	{
		header("Location: ../control/index.php");
	}
	else if (isset($_SESSION["login"]))
	{
		$login = $_SESSION["login"];
		$enable = $_SESSION["enable"];
		$tabname = $_SESSION["tabname"];
		include_once('../model/mysql_connection.php');
		include_once('../model/get_list.php');
		
		if (isset($_GET['cat']))
		{
			include_once('../model/get_list_cat.php');
			$files = get_list_cat(0, $_GET['cat']);
			echo "cat et page";
		}
		else
		if (isset($_GET['key']) AND isset($_GET['page']))
		{
			include_once('../model/get_list_key.php');
			$files = get_list_key($_GET['page']);
			echo "Key et page";
		}
		else
		if (isset($_GET['page']))
		{
			include_once('../model/get_list.php');
			$files = get_list($_GET['page']);
			echo "Page";
		}
		else
		{
			$files = get_list(0);
		}
		
		foreach($files as $cle => $file)
		{
			$files[$cle]['id'] = htmlspecialchars($file['id']);
			$files[$cle]['sha1'] = htmlspecialchars($file['sha1']);
			$files[$cle]['name'] = nl2br(htmlspecialchars($file['name']));
			$files[$cle]['size'] = htmlspecialchars($file['size']);
			include_once('../model/find_file_onserv.php');
			$result=is_on_web_serv($files[$cle]['sha1']);
			if ($result == 0)
			{
				include_once('../model/get_name_with_sha1.php');
				$filename = get_name_with_sha1sum($files[$cle]['sha1']);
				$location = "../uploads/".$filename;
				$files[$cle]['loc'] = $location;
				$files[$cle]['present'] = 1;
			}
			else
			{
				$files[$cle]['present'] = 0;
			}
		}
		include_once('../view/main.php');

	}
?>


