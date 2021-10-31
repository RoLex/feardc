<?
function error()
{
	exit('Error. <a href="https://dcplusplus.sourceforge.io/">Click here to go back to the main DC++ site.</a>');
}

if (!isset($_SERVER) || !isset($_SERVER['SCRIPT_URL']))
{
	error();
}

@session_start();

if (isset($_POST) && isset($_POST['language']))
{
	$_SESSION['language'] = $_POST['language'];
}

if (isset($_SESSION) && isset($_SESSION['language']))
{
	$language = $_SESSION['language'];
}

else
{
	$language = 'en-US';

	if (isset($_SERVER['HTTP_ACCEPT_LANGUAGE']))
	{
		$langs = explode(',', $_SERVER['HTTP_ACCEPT_LANGUAGE']);
		foreach ($langs as $lang)
		{
			// some languages have an "affinity" argument; remove it
			$pos = strpos($lang, ';');
			if ($pos !== FALSE)
			{
				$lang = substr($lang, 0, $pos);
			}

			// look for an exact match
			if (@is_dir($lang))
			{
				$language = $lang;
				break;
			}

			// ignore the "sub-language" part
			$pos = strpos($lang, '-');
			if ($pos !== FALSE)
			{
				$lang = substr($lang, 0, $pos);
				if (@is_dir($lang))
				{
					$language = $lang;
					break;
				}
			}
		}
	}
}

$name = str_replace('/webhelp/', '', $_SERVER['SCRIPT_URL']); // todo to be correct, one should strrev the last slash etc
if ($name == '')
{
	$name = 'index.html';
}

$output = @file_get_contents("$language/$name");
if ($output === FALSE)
{
	error();
}

$pos = strpos($output, '<body');
if ($pos === FALSE)
{
	exit($output);
}
$pos = strpos($output, '>', $pos);
if ($pos === FALSE)
{
	exit($output);
}
$pos++;

echo substr($output, 0, $pos);

$toc = @file_get_contents("$language/toc.inc");
if ($toc !== FALSE)
{
	echo "\n<div style=\"float: right; width: 25%; margin-left: 15px; padding: 10px 10px 10px 10px; background-color: #F0F0F0\">\n$toc\n</div>";
}
?>

<div style="margin-bottom: 15px; padding: 10px 10px 10px 10px; background-color: #F0F0F0">
<form method="post" action="<?= $name ?>">
	Language:
	<select name="language">
<?
$dirs = array_diff(@scandir('.'), array('.', '..'));
foreach ($dirs as $dir)
{
	if (!@is_dir($dir))
	{
		continue;
	}

	echo "\t\t<option value=\"$dir\"";
	if ($dir == $language)
	{
		echo ' selected="selected"';
	}
	echo ">$dir";
	$name = @file_get_contents("$dir/name.txt");
	if ($name !== FALSE)
	{
		echo ": $name";
	}
	echo "</option>\n";
}
?>
	</select>
	<input type="submit" value="Change"/>
</form>
</div>

<?= substr($output, $pos); ?>
