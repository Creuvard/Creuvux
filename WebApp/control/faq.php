<?php
	session_start();
	include('../config.php');
	if ($_SESSION["login"] == NULL)
	{
		//header("Location: ./index.php");	
		header("Location http://www.google.fr");
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
					<!-- LIST-->
					<div class="block" id="block-tables">
						<div class="secondary-navigation">
							<ul class="wat-cf">
								<li class="active first" ><a href="./main.php">Lists</a></li>
								<?php
									require_once("../model/list_onglet.php");
								?>
							</ul>
						</div>
						<div class="content">
						
							<h2 class="title">FAQ</h2>
								<?php
									require_once("../view/faq.php");
								?>	
						</div>
					</div>
				<!-- END LIST -->

					
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
