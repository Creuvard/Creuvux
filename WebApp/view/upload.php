<form action="../control/get-file.php" method="post" enctype="multipart/form-data">
	<?php	
		require_once("../model/list_cat_for_upload.php");
	?>
	<p>
		<b>Création d'une catégorie:</b><input type="text" name="newcat" /> <i>(Vide si vous utilisez une catégorie existante)</i>
	</p>
	<div class="columns wat-cf">
		<div class="column left">
			<div class="group">
				<textarea class="text_area" rows="10" cols="80" name="comment"></textarea>
			</div>
		</div>
	</div>
	<p>Choix du fichier:<br />
	<input type="hidden" name="MAX_FILE_SIZE" value="1073741824" />
	<input type="file" name="filename" /><br />
	</p>
	<div class="group navform wat-cf">
		<button class="button" type="submit">
			<img src="./images/tick.png" alt="Save"> Save
		</button>
		<span class="text_button_padding">or</span>
		<a href="./main.php" class="button"><img src="./images/cross.png" alt="Delete"> Cancel</a>
	</div>
</form>
