<div id="box">
	<div class="block" id="block-login">
		<h2>Login Box</h2>
		<div class="content login">
			
			<div class="message notice">
				<p></p>
			</div>
			
			<!--FORM -->
			<form method="post" action="../model/verifLogin.php" class="form login">
				
				<div class="group wat-cf">
					<div class="left">
						<label class="label right">Login</label>
					</div>
					<div class="right">
						<input class="text_field" type="text" name="login">
					</div>
				</div>
				
				<div class="group wat-cf">
					<div class="left">
						<label class="label right">Password</label>
					</div>
					<div class="right">
						<input class="text_field" type="password" name="password">
					</div>
				</div>
				
				<div class="group navform wat-cf">
					<div class="right">
						<button class="button" type="submit">
							<img src="./images/key.png" alt="Save"> Login
						</button>
						<a href="inscription.php" class="button"><img src="./images/tick.png" alt="Save"> Inscription</a>
					</div>
				</div>

			</form>
			<!-- FORM -->

		</div>
	</div>
</div>
