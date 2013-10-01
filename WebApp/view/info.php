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
								else
								{
									$taille = $file['size'];
									$size = number_format($taille, 2, ',', ' ');
									$size=$size.'<i> Octets</i>';
								}

								echo $size;
							?>
							
							</p></li>
							<li><p><u>Sha1</u>: <?php echo $file['sha1']; ?></p></li>
							</ul><hr />
							<?php 
								}
							?>
							<h3>Synopsis:</h3>
							<ul class=\"list\">
							<?php
								foreach($comments as $comment)
								{
							?>
								<li>
                  <div class=\"left\">
                    <b><?php echo $comment['user']; ?></b>
                  </div>
                  <div class=\"item\">
                    <p><?php echo $comment['comment']; ?><br /></p>
										<p><small><?php echo $comment['date_reply']; ?></small></p>
                  </div>
									<hr/>
                </li>
							<?php
								}
							?>
							</ul>
							<p>
						<?php
							if ($add_cmt == 1)
							{
						?>
						<hr />
						<form action="../control/add_msg.php" method="post" class="form">
                <div class="columns wat-cf">
                  <div class="column left">
                    <div class="group">
                      <label class="label">Text area</label>
                      <input type="radio" name="sha1" value=<?php echo $sha1; ?> checked="checked" />
											<textarea class="text_area" rows="10" cols="80" name="comment"></textarea>
                    </div>
                  </div>
                  
                </div>
                <div class="group navform wat-cf">
                  <button class="button" type="submit">
                    <img src="./images/tick.png" alt="Save"> Save
                  </button>
                  <span class="text_button_padding">or</span>
									<a href="../control/info.php?sha1=<?php echo $sha1; ?>" class="button"><img src="./images/cross.png" alt="Delete"> Cancel</a>
                </div>
              </form>



						<?php
							}
						?>
					
						<?php
							if ($add_cmt == 0)
							{
								echo "<a href=\"./del_onglet.php?sha1=".$sha1."\" class=\"button\"><img src=\"./images/cross.png\" alt=\"Delete\"> Enlever l'onglet</a>";
								$result=ifadmin();
								if ($result==0) {	
									echo "<a href=\"./modif.php?sha1=".$sha1."\" class=\"button\"><img src=\"./images/tick.png\" alt=\"Modify\"> Modify</a>";
								}
								echo "<a href=\"./info.php?add=cmt&sha1=".$sha1."\" class=\"button\"><img src=\"./images/tick.png\" alt=\"Add Comment\"> Add Comment</a>";
							if ($present == 1)
							{
								echo "<a href=\"";
								echo $location;
								echo "\" class=\"button\">";
								echo "<img src=\"./images/tick.png\" alt=\"Direct Download\">Direct Download</a>";
							} else {
								echo "<a href=\"./download.php?sha1=";
								echo $file['sha1'];
								echo "\" class=\"button\">";
								echo "<img src=\"./images/tick.png\" alt=\"Crv Download\">Download</a>";
							}
							}
						?>




						<br />
            </p>
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
