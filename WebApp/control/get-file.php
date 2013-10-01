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
	}
	require_once("../model/upload.php");

	if (isset($_POST['newcat']) AND ($_POST['newcat'] != ''))
	{
			$cat=$_POST['newcat'];
	}
	else if (isset($_POST['cat']))
		$cat=$_POST['cat'];
	
	if (isset($_POST['comment']))
		$comment=$_POST['comment'];
	if (isset($_FILES['filename']) AND $_FILES['filename']['name'])
		$filename=$_FILES['filename']['name'];
	if (isset($_FILES['filename']) AND $_FILES['filename']['size'])
		$size=$_FILES['filename']['size'];
	
	//echo $cat."<br /".$comment."<br />".$filename."<br />".$size;
	
	if (isset($_FILES['filename']) AND $_FILES['filename']['error'] == 0)
	{
		if ($_FILES['filename']['size'] <= 1099511627776)
		{
				move_uploaded_file($_FILES['filename']['tmp_name'], '../uploads/' . basename($_FILES['filename']['name']));
				$fileHash = sha1_file('../uploads/' . $_FILES["filename"]["name"]);
				add_fullfile($fileHash, $filename, $comment, $cat, $size, $login);
				header("Location: ../control/main.php");
		}
	}
	
