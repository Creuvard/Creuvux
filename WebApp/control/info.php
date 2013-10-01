<?php
	session_start();
	include('../config.php');
	if ($_SESSION["login"] == NULL)
	{
		header("Location ../control/index.php");
	}
	else
	{
		$login = $_SESSION["login"];
		$enable = $_SESSION["enable"];
		$tabname = $_SESSION["tabname"];
		$add_cmt = 0;
		$present = 0;
		include_once('../model/mysql_connection.php');
		include_once('../model/get_list.php');
		include_once('../model/insert_tab.php');
		
		if (isset($_GET['add']))
		{
			$add_cmt = 1;
		}
		
		if (isset($_GET['sha1']))
		{
			include_once('../model/info.php');
			include_once('../model/info_comment.php');
			$sha1 = $_GET['sha1'];
			$files = info_file($_GET['sha1']);
			$comments = info_comment_file($_GET['sha1']);

		}
		else
		{
			header("Location ../control/main.php");
		}
		foreach($files as $cle => $file)
		{
			$files[$cle]['cat'] = htmlspecialchars($file['cat']);
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
				$present = 1;
				//$files[$cle]['loc'] = $location;
				//$files[$cle]['present'] = 1;
			}
			else {
				include_once('../model/get_name_with_sha1.php');
				$filename = get_name_with_sha1sum($files[$cle]['sha1']);
			}
		}

		foreach($comments as $cle => $comment)
		{
			#$comments[$cle]['sha1'] = htmlspecialchars($comment['sha1']);
			$comments[$cle]['user'] = nl2br(htmlspecialchars($comment['user']));
			$comments[$cle]['comment'] = nl2br(htmlspecialchars($comment['comment']));
			$comments[$cle]['date_reply'] = htmlspecialchars($comment['date_reply']);
		}
		/*
		echo $tabname;
		echo $sha1;
		echo $filename;
		*/
		add_tab($tabname, $sha1, $filename);
		include_once('../view/info.php');
	}
?>


