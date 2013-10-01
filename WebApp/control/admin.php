<?php
	session_start();
	include('../config.php');
	if ($_SESSION["login"] == NULL)
	{
		header("Location: http://www.google.fr");
	}
	else
	{
		$login = $_SESSION["login"];
		$enable = $_SESSION["enable"];
	}
	require_once("../view/doctype.php");
?>

<html xmlns="http://www.w3.org/1999/xhtml" lang="fr">
<?php
	require_once("../view/http-head.php");
?>
<body>
	<!-- CONTAINER -->
  <div id="container">
			<!-- HEADER -->
			<?php	require_once("../view/header.php"); ?>
			<!-- END HEADER -->
			
			
			<!-- WRAPPER -->
			<div id="wrapper" class="wat-cf">
			
				<!-- MAIN -->
				<div id="main">
					<!-- ADMIN -->
					<h1>Panneau d'administration</h1>
					
					<div class="inner">
						<div class="flash">
							<div class="message notice">
								<h3>Gestion des serveurs du réseau</h3>
							</div>
						</div>
					</div>
					<?php
						require_once("./admin_list_servers_htmlgen.php");
					?>

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

	

					
					<!-- FOOTER -->
					<?php	require_once("../view/footer.php"); ?>
				</div>
				<!-- END MAIN -->
				<!-- SIDEBAR -->
				<?php
					require_once("../model/if-admin.php");
					$result=ifadmin();
					if ($result==0) {	
						require_once("../view/admin-info.php"); 
					}	
				?>
				<?php require_once("../view/login-info.php"); ?>
			</div>
			<!-- END WRAPPER -->

	</div>
	<!-- END CONTAINER -->

</body>
</html>
