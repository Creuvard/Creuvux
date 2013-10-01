<div class="block" id="block-tables">  
	<div class="content">
	
		<div class="inner">
			<form action="../model/gen-main-filedb.php" method="post" class="form">
				<table class="table">
					<tbody>
						<tr>
							<th class="first"></th>
							<th>ID</th>
							<th>Serveur</th>
							<th>Autorisé</th>
							<th class="last">&nbsp;</th>
						</tr>
						<?php require_once("./admin_list_servers.php");	?>
					</tbody>
				</table>
				
				<div class="actions-bar wat-cf">
					<div class="actions">
						<button class="button" type="submit">
							<img src="./images/tick.png" alt="Gen DB File"> Gen Database Config
						</button>
						<!--
						<a href="./add-server.php" class="button"><img src="./images/tick.png" alt="Add Server"> Ajouter un serveur</a>
						-->
					</div>
				</div>
			</form>
			<form action="../model/add-srv.php" method="post">
				<p>
					<input type="text" name="server" />
					<input type="submit" value="Ajouter un serveur" />
				</p>
			</form>
		</div>
	</div>
</div>
