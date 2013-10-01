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
						
							<h2 class="title">Fichiers présents sur le réseau</h2>
							<div class="inner">
								
								<?php
									require_once("../model/list_cat.php");
								?>
								<form method="post" action="main_key.php">
									<p>
										<strong>Entrer un mot clé:</strong>
										<input type="text" name="keyword" />
										<input type="submit" name="Search" />
									</p>
								</form>
															
								<form action="#" class="form">
									<table class="table">
										<tbody><tr>
										<th>ID</th>
										<th>Title</th>
										<th>Size</th>
										<th class="last">&nbsp;</th>
										</tr>
										<?php
											foreach($files as $file)
											{
											echo '<tr>';
										?>
										<td><?php echo $file['id']; ?></td>
										<td><?php echo $file['name']; ?></td>
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
											else
											{
												$taille = $file['size'];
												$size = number_format($taille, 2, ',', ' ');
												$size=$size.'<i> octets</i>';
											}
										?>
										<td><?php echo $size; ?></td>
										<td class="last"><a href="./info.php?sha1=<?php echo $file['sha1']; ?>">Info</a> |  
										<?php
										if (	$file['present'] == 1)
										{
											?>
											<a href="<?php echo $file['loc']; ?>">Direct Download</a></td>
											<?php
										} else {
											?>
											<a href="../control/main.php">Creuvux Download</a>
											<!--
											<a href="./download.php?sha1=<?php echo $file['sha1']; ?>">Download</a>
											-->
											<?php
										}
										?>
										<?php
											echo '</tr>';
										}
										echo '</tr></tbody></table>';
										?>
										</tbody>
									</table>
									<!--
									<div class="actions-bar wat-cf">
										<div class="pagination">
											<span class="disabled prev_page">« Previous</span><span class="current">1</span><a rel="next" href="#">2</a><a href="#">3</a><a href="#">4</a><a href="#">5</a><a href="#">6</a><a href="#">7</a><a href="#">8</a><a href="#">9</a><a href="#">10</a><a href="#">11</a><a rel="next" class="next_page" href="#">Next »</a>
										</div>
									</div>
									-->
								</form>
							</div>
						</div>
					</div>
				<!-- END LIST -->

					
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
