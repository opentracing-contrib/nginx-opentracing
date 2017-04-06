<html>
<body>
Welcome <?php echo $_POST["firstname"]; echo $_POST["lastname"];?><br>

<p><?php
if (array_key_exists('profilepic_name', $_POST)) {
  $imgPath = $_POST['profilepic_path'];
  $aExtraInfo = getimagesize($imgPath);
  $sImg = "data:" . $aExtraInfo["mime"] . ";base64," . 
          base64_encode(file_get_contents($imgPath));
  echo '<image src="' . $sImg . '" alt="Profile"/>';
}
?><br>


</body>
</html>
