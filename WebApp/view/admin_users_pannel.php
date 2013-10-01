<div class="inner">
	<div class="flash">
		<div class="message notice">
			<h3>Gestion des Membres</h3>
		</div>
	</div>
</div>
<?php
	$users = get_list_clients();
	foreach($users as $cle => $user)
	{
		$users[$cle]['id'] = htmlspecialchars($user['id']);
		$users[$cle]['name'] = nl2br(htmlspecialchars($user['name']));
	}
	include("./admin_list_clients_htmlgen.php");
?>

