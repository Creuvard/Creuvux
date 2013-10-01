<div class="block" id="block-tables">  
	<div class="content">
	
		<div class="inner">
			<form action="../model/gen-main-filedb.php" method="post" class="form">
				<table class="table">
					<tbody>
						<tr>
							<th class="first"></th>
							<th>ID</th>
							<th>User</th>
							<th>Mail</th>
							<th>Date</th>
							<th class="last">Enable</th>
						</tr>
						<?php require_once("./admin_list_clients.php");	?>
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
		</div>
	</div>
</div>
