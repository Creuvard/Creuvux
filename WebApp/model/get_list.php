<?php

/*
function get_liste($ask1, $ask2)
{
include('../model/mysql_connection.php');


	$req = "SELECT `id`,`sha1`,`name`,`size` FROM `crv_main_file` ORDER BY `date_file` DESC";
	$file = $bdd->query("$ask1");
	while ($data = $file->fetch())
	{
	echo '<tr>';
	?>
		<td><?php echo $data['id']; ?></td>
		<td><?php echo $data['name']; ?></td>
		<?php 
			if ($data['size'] > 1024*1024*1024)
			{
			 $taille = $data['size'] / (1024*1024*1024);
			 $size = number_format($taille, 2, ',', ' ');
			 $size=$size.'<i> Go</i>';
			}
			else
			if ($data['size'] > 1024*1024)
			{
				$taille = $data['size'] / (1024*1024);
				$size = number_format($taille, 2, ',', ' ');
				$size=$size.'<i> Mo</i>';
			}
			else
			if ($data['size'] > 1024)
			{
				$taille = $data['size'] / (1024);
				$size = number_format($taille, 2, ',', ' ');
				$size=$size.'<i> Ko</i>';
			}
		?>
		<td><?php echo $size; ?></td>
		<td class="last"><a href="./info.php?sha1=<?php echo $data['sha1']; ?>">show</a> | <a href="./download.php?sha1=<?php echo $data['sha1']; ?>">Download</a> </td>
	<?php
	echo '</tr>';
	}
	echo '</tr></tbody></table>';
	$file->closeCursor();
}
*/
function get_list($numpage)
{
		//include_once('../model/mysql_connection.php');
		
		global $bdd;
    $numpage = (int) $numpage;
    $offset = $numpage * 500;
    $limit = $offset + 500;
    $req = $bdd->prepare('SELECT `id`,`sha1`,`name`,`size` FROM `crv_main_file` ORDER BY `date_file` DESC LIMIT :offset, :limit');
    $req->bindParam(':offset', $offset, PDO::PARAM_INT);
    $req->bindParam(':limit', $limit, PDO::PARAM_INT);
    $req->execute();
    $files = $req->fetchAll();
    
    return $files;
}
