<html xmlns="http://www.w3.org/1999/xhtml" lang="fr">
<?php
	require_once("../view/http-head.php");
?>
<body>
	<!-- CONTAINER -->
  <div id="container">
			<!-- HEADER -->
			<?php	
				require_once("../view/header.php");
				require_once("../model/if-admin.php");
			?>
			<!-- END HEADER -->
			
			
			<!-- WRAPPER -->
			<div id="wrapper" class="wat-cf">
			
				<!-- MAIN -->
				<div id="main">
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
						<?php
							foreach($files as $file)
							{
						?>
							<h2 class="title"></h2>
							<div class="inner">
								<div class="flash">
									<div class="message warning">
										<h2>Admin mode</h2>
									</div>
									<div class="message notice">
									<h2>
											<?php 
												echo $file['name']; 
											?>
									</h2>
									</div>
								</div>
								<br />	
							<ul>
							<li><p><u>Size</u>: 
							<?php 
								if ($file['size'] > 1024*1024*1024)
								{
									$taille = $file['size'] / (1024*1024*1024);
									$size = number_format($taille, 2, ',', ' ');
									$size=$size.'<i> Go</i>';
								}
								else
								if ($file['size'] > 1024*1024)
								{
									$taille = $file['size'] / (1024*1024);
									$size = number_format($taille, 2, ',', ' ');
									$size=$size.'<i> Mo</i>';
								}
								else
								if ($file['size'] > 1024)
								{
									$taille = $file['size'] / (1024);
									$size = number_format($taille, 2, ',', ' ');
									$size=$size.'<i> Ko</i>';
								}
								echo $size;
							?>
							
							</p></li>
							<li><p><u>Sha1</u>: <?php echo $file['sha1']; ?></p></li>
							</ul><hr />
							<?php 
								}
							?>
							<h2>Renomer le fichier</h2>
							<form action="../control/modif_rename.php?sha1=<?php echo $sha1; ?>" method="post">
								<p>
									<input type="text" name="newfilename" />
									<input type="submit" value="Renommer" />
								</p>
							</form>
							
						</div>
					</div>
					</div>

					<!-- FOOTER -->
					<?php	require_once("../view/footer.php"); ?>
				</div>
				<!-- END MAIN -->
				<!-- SIDEBAR -->
				<?php
					//require_once("../model/if-admin.php");
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
