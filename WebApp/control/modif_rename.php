<?php
	session_start();
	include('../config.php');
	if ($_SESSION["login"] == NULL)
	{
		header("Location: ../control/index.php");
	}
	else
	{
		$login = $_SESSION["login"];
		$enable = $_SESSION["enable"];
		$tabname = $_SESSION["tabname"];
		$add_cmt = 0;
		$present = 0;
		include_once('../model/mysql_connection.php');
		include_once('../model/modif_rename.php');
		include_once('../model/info.php');
		

		if (isset($_GET['sha1']))
		{
			if (isset($_POST['newfilename']))
			{
				$sha1 = $_GET['sha1'];
				$newfilename = $_POST['newfilename'];
				$files = info_file($_GET['sha1']);
				foreach($files as $cle => $file)
				{
					$files[$cle]['cat'] = htmlspecialchars($file['cat']);
					$files[$cle]['sha1'] = htmlspecialchars($file['sha1']);
					$files[$cle]['name'] = nl2br(htmlspecialchars($file['name']));
					$files[$cle]['size'] = htmlspecialchars($file['size']);
					include_once('../model/find_file_onserv.php');
					//$result=is_on_web_serv($files[$cle]['sha1']);
					//if ($result == 0)
					//{
						include_once('../model/get_name_with_sha1.php');
						$filename = get_name_with_sha1sum($files[$cle]['sha1']);
						$location = "../uploads/".$filename;
						rename($location, "../uploads/".$newfilename);
						//echo $location;
					//}
				}
				rename_file($sha1, $newfilename);
				header("Location: ../control/main.php");
			}
		}
		else
		{
			header("Location: ../control/main.php");
		}
	}
?>


