<?php
	include('../config.php');
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
					<?php 
						if (isset($_GET['comment']))
						{	
							echo "<div class=\"inner\">";
							echo "<div class=\"flash\">";
							echo "<div class=\"message notice\">";
							echo "<h3>Vous devez cliquer sur le lien contenu dans le mail que vous vennez de recevoir</h3>";
							echo "</div>";
							echo "</div>";
							echo "</div>";					
						}
					?>
					<!-- LOGIN BOX -->
					<?php require_once("../view/login.php"); ?>
					<!-- FOOTER -->
					<?php	require_once("../view/footer.php"); ?>
				</div>
				<!-- END MAIN -->
				<!-- SIDEBAR -->
				<?php require_once("../view/login-info.php"); ?>
			</div>
			<!-- END WRAPPER -->

	</div>
	<!-- END CONTAINER -->
</body>
</html>
