<?php

$pw_in = trim($_POST['pw']);

if (empty($pw_in)) 
	$pw_in = "0822455D0A16";

$pw = null;
if (!empty($pw_in))
	$pw = decrypt($pw_in);

if (!empty($pw) && strlen($pw) > 0) {
	$html_pw = "Cleartext is <b>".htmlspecialchars($pw)."</b><br/><br/>";
} 

$html_pw_in = htmlspecialchars($pw_in);
echo <<<EOT
<h2>Decrypt Cisco <code>service password-encryption</code> obfuscated password</h2>
<form method="POST">
<input type="text" value="$html_pw_in" name="pw" />
<input type="submit" value="deobfuscate">
<form>
<br/><br/>
$html_pw
<hr/>
<pre>

char T[] = {
    0x64, 0x73, 0x66, 0x64, 0x3b, 0x6b, 0x66, 0x6f, 0x41, 0x2c, 0x2e,
    0x69, 0x79, 0x65, 0x77, 0x72, 0x6b, 0x6c, 0x64, 0x4a, 0x4b, 0x44,
    0x48, 0x53, 0x55, 0x42, 0x73, 0x67, 0x76, 0x63, 0x61, 0x36, 0x39,
    0x38, 0x33, 0x34, 0x6e, 0x63, 0x78, 0x76, 0x39, 0x38, 0x37, 0x33,
    0x32, 0x35, 0x34, 0x6b, 0x3b, 0x66, 0x67, 0x38, 0x37
};

#define UNHEX(c) ((c)>='A'&&(c)<='F' ? (c)-'A'+10 : (c)-'0')
#define UNPACK(x, o) (UNHEX(*(x+(o)*2)) << 4 | UNHEX(*(x+(o)*2+1))) & 0xff

void decrypt(char *in, char *out) {
    int seed, o, password_len;

    password_len = strlen(in)/2 - 1;

    seed = UNPACK(in, 0);

    for (o=0; o < password_len; o++)
        *(out++) = UNPACK(in, o+1) ^ T[seed++ % sizeof(T)];
    *out = 0;
}
</pre>
EOT;

function decrypt($enc) 
{
	$xlat = array(
		0x64, 0x73, 0x66, 0x64, 0x3b, 0x6b, 0x66, 0x6f, 0x41, 0x2c, 0x2e,
		0x69, 0x79, 0x65, 0x77, 0x72, 0x6b, 0x6c, 0x64, 0x4a, 0x4b, 0x44,
		0x48, 0x53, 0x55, 0x42, 0x73, 0x67, 0x76, 0x63, 0x61, 0x36, 0x39,
		0x38, 0x33, 0x34, 0x6e, 0x63, 0x78, 0x76, 0x39, 0x38, 0x37, 0x33,
		0x32, 0x35, 0x34, 0x6b, 0x3b, 0x66, 0x67, 0x38, 0x37
	);

	$seed = intval(substr($enc,0,2));
	if ($seed > 15)
		return NULL;

	$pw = '';
	for ($i=2; $i<=strlen($enc); $i+=2) {
		$s = strtolower(substr($enc, $i, 2));
		if (!preg_match('/[0-9a-f]{2}/i', $s))
			continue;
		$val = @ord(pack("H*", $s));
		$pw .= chr($val ^ $xlat[$seed++]);
	}

	return $pw;
}


?>
