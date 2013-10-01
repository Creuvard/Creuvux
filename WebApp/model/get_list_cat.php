<?php

function get_list_cat($numpage, $cat)
{
		global $bdd;
    $numpage = (int) $numpage;
    $offset = $numpage * 50;
    $limit = $offset + 50;
		$sql = "SELECT `crv_main_file`.`id`, `crv_main_file`.`sha1`, `crv_main_file`.`name`, `crv_main_file`.`size` FROM `crv_main_file`,`crv_main_cat` WHERE `crv_main_cat`.`sha1` LIKE `crv_main_file`.`sha1` AND `crv_main_cat`.`cat` LIKE '".$cat."' ORDER BY `date_file` DESC LIMIT :offset, :limit";
    $req = $bdd->prepare($sql);
    $req->bindParam(':offset', $offset, PDO::PARAM_INT);
    $req->bindParam(':limit', $limit, PDO::PARAM_INT);
    $req->execute();
    $files = $req->fetchAll();
    
    return $files;
}
