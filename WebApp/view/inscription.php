<div id="box">
	<div class="block" id="block-signup">
		<h2>Sign up</h2>
			<div class="content">	
				<div class="message notice">
					<p></p>
				</div>
				
				<form method="post" class="form" action="../model/exec_inscription.php">
					<div class="group wat-cf">
						<div class="left">
							<label class="label">Login</label>
						</div>
						<div class="right">
							<input class="text_field" type="text" name="login">
							<span class="description">Ex: Babacool51</span>
						</div>
					</div>
					
					<div class="group wat-cf">
						<div class="left">
							<label class="label">Email</label>
						</div>
						<div class="right">
							<input class="text_field" type="text" name="email">
							<span class="description">Ex: test@example.com</span>
						</div>
					</div>
					
					<div class="group wat-cf">
						<div class="left">
							<label class="label">Password</label>
						</div>
						<div class="right">
							<input class="text_field" type="password" name="password">
							<span class="description">Must contains more than 8 caracters</span>
						</div>
					</div>
					
					<div class="group navform wat-cf">
						<button class="button" type="submit">
							<img src="./images/tick.png" alt="Save"> Valider
						</button>
					</div>
				</form>	
			</div>
		</div>
</div>
