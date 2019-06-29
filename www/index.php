<?php

session_start();

/*if (!isset($_SERVER['HTTPS']) || $_SERVER['HTTPS'] == 'off') {
	header('Location: https://' . $_SERVER['HTTP_HOST'] . $_SERVER['REQUEST_URI'], true, 301);
	exit;
}*/

//header('Strict-Transport-Security: max-age=31536000; includeSubDomains');
header('X-XSS-Protection: 1; mode=block');
header('X-Frame-Options: DENY');
header('X-Content-Type-Options: nosniff');
header('Referrer-Policy: no-referrer');

// todo: tools to capitalizer/lowercase track names?
// note: for large files, use fopen to file_put_contents to allow for streaming

function login($user) {
    $_SESSION['user'] = $user;
}

function logout() {
    session_unset();
}

function get_user() {
    if (!isset($_SESSION['user'])) {
        return false;
    }
    return $_SESSION['user'];
}

login('dib'); // TODO: Proper login

function get_args() {
	$base = implode('/', array_slice(explode('/', $_SERVER['SCRIPT_NAME']), 0, -1)) . '/';
	$uri = substr($_SERVER['REQUEST_URI'], strlen($base));
	if (strstr($uri, '?')) {
		$uri = substr($uri, 0, strpos($uri, '?'));
	}
    $path = trim($uri, '/');
    return explode('/', $path);
}

function get_extension_for_content($content) {
    $info = new finfo(FILEINFO_MIME);
    $mime = $info->buffer($content);
    switch ($mime) {
        case 'image/jpeg; charset=binary': return 'jpg';
        case 'image/png; charset=binary': return 'png';
        case 'image/gif; charset=binary': return 'gif';
        case 'image/webp; charset=binary': return 'webp';
        default: return 'txt';
    }
}

function require_fields($fields, $array) {
    foreach ($fields as $field) {
        if (!isset($array[$field])) {
            echo "Missing field: $field\n";
            var_dump($array);
            exit;
        }
    }
}

function has_fields($fields, $array) {
    foreach ($fields as $field) {
        if (!isset($array[$field])) {
            return false;
        }
    }
    return true;
}

function get_random_prefix() {
    $time = time();
    $rand = random_int(1000, 9999);
    return $time . $rand;
}

function unzip($source, $destination, $name) {
    $zip = new ZipArchive();
    $result = $zip->open($source);
    if ($result !== true) {
        return;
    }
    $prefix = get_random_prefix();
    $name = "$prefix $name";
    $name = basename($name, '.zip');
    $zip->extractTo("$destination/$name");
    $zip->close();
}

function cli_program($program, $command, $args) {
    setlocale(LC_CTYPE, 'en_US.UTF-8');
    foreach ($args as &$arg) {
        $arg = escapeshellarg($arg);
    }
    $args = implode(' ', $args);
    return shell_exec("$program $command $args");
}

function cli($command, $args) {
    return cli_program('../dmusic', $command, $args);
}

function upload($args) {
    require_fields(['albums'], $_FILES);
    $uploads = cli('path', ['uploads']);
    $albums = $_FILES['albums'];
    foreach ($albums['tmp_name'] as $index => $tmp_name) {
        $name = $albums['name'][$index];
        $error = $albums['error'][$index];
        $size = $albums['size'][$index];
        $type = $albums['type'][$index];
        if ($size === 0 || $tmp_name === '') {
            echo 'Empty file.';
            continue;
        }
        // note: $type is user input - not checked by server
        if ($type !== 'application/x-zip-compressed') {
            echo 'Not a zip file.';
            continue;
        }
        if ($error !== UPLOAD_ERR_OK) {
            if ($error === UPLOAD_ERR_INI_SIZE) {
                echo "File $name exceeds upload_max_filesize in php.ini";
            } else if ($error === UPLOAD_ERR_NO_FILE) {
                echo 'No file was uploaded.';
            } else if ($error === UPLOAD_ERR_CANT_WRITE) {
                echo 'Failed to write to disk.';
            } else {
                echo "File upload error: $error";
            }
            continue;
        }
        unzip($tmp_name, $uploads, $name);
    }
}

// TODO: Make this whitelist based on database.
function is_allowed_format($format) {
    return in_array($format, [
        'aac-256', 
        'flac-16', 'flac-24', 'flac-cd-16', 'flac-cd-24', 'flac-web-16', 'flac-web-24', 'mp3-128', 
        'mp3-192', 'mp3-256', 'mp3-320', 'mp3-v0', 'mp3-v1', 'mp3-v2'
    ]);
}

function attach($args) {
    require_fields(['method', 'folder'], $_POST);
    $method = $_POST['method'];
    $folder = $_POST['folder'];
    $attachment = false;
    if ($method === 'url') {
        if (!isset($_POST['file'])) {
            echo 'URL not found.';
            return;
        }
        $url = $_POST['file'];
        $attachment = @file_get_contents($url);
        if ($attachment === false) {
            echo 'Failed to download attachment.';
            return;
        }
    } else if ($method === 'file') {
        if (!isset($_FILES['file'])) {
            echo 'File not found.';
            return;
        }
        $attachment = file_get_contents($_FILES['file']['tmp_name']);
        if ($attachment === false) {
            echo 'Failed to read attachment.';
            return;
        }
    } else {
        echo 'Invalid method.';
        return;
    }
    $uploads = cli('path', ['uploads']);
    $extension = get_extension_for_content($attachment);
    $name = get_random_prefix() . '.' . $extension;
    $destination = "$uploads/$folder/$name";
    $written = @file_put_contents($destination, $attachment);
    if ($written === false) {
        echo 'Failed to save file to ' . $destination;
        return;
    }
    echo cli('render', ['prepare', $folder, 'attachment', $name]);
}

function prepare($args) {
    require_fields(['name', 'released_at', 'catalog', 'format', 'type', 'folder', 'release_type', 'groups', 'attachments', 'discs'], $_POST);
    $name = $_POST['name'];
    $released_at = $_POST['released_at'];
    $catalog = $_POST['catalog'];
    $format = $_POST['format'];
    $type = $_POST['type'];
    $attachments = json_decode($_POST['attachments'], true);
    $folder = $_POST['folder'];
    $discs = json_decode($_POST['discs'], true);
    $groups = json_decode($_POST['groups'], true);
    $release_type = $_POST['release_type'];
    if (!is_array($attachments) || !is_array($discs) || !is_array($groups)) {
        return;
    }
    if (count($discs) === 0 || count($groups) === 0) {
        return;
    }
    foreach ($attachments as $attachment) {
        require_fields(['keep', 'target', 'path'], $attachment);
    }
    foreach ($discs as $disc) {
        require_fields(['num', 'name', 'tracks'], $disc);
        if (!is_array($disc['tracks'])) {
            return;
        }
        foreach ($disc['tracks'] as $tracks) {
            require_fields(['num', 'name'], $tracks);
        }
    }
    if (!is_allowed_format($format)) {
        return;
    }
    // Add album
    $album_id = (int)cli('create', ['album', 'true', $name, $type]);
    if ($album_id === 0) {
        return;
    }
    echo "Created album $album_id<br>";
    // Add release
    $album_release_id = (int)cli('create', ['album_release', 'true', $album_id, $release_type, $catalog, $released_at]);
    if ($album_release_id === 0) {
        return;
    }
    echo "Created album release $album_release_id<br>";
    // Add album release groups
    $priority = 1;
    foreach ($groups as $group_id) {
        cli('create', ['album_release_group', 'false', $album_release_id, $group_id, $priority]);
        $priority++;
    }
    // Get general path info and make album directory
    $uploads_directory = cli('path', ['uploads']);
    $albums_directory = cli('path', ['albums']);
    mkdir("$albums_directory/$album_release_id");
    // Add discs
    foreach ($discs as $disc) {
        $disc_num = (int)$disc['num'];
        $disc_folder = "$albums_directory/$album_release_id/$disc_num";
        mkdir($disc_folder);
        mkdir("$disc_folder/$format");
        cli('create', ['disc', 'false', $album_release_id, $disc_num, $disc['name']]);
        echo "Created disc $disc_num<br>";
        // Add and move tracks
        foreach ($disc['tracks'] as $track) {
            $track_num = (int)$track['num'];
            $extension = pathinfo($track['path'], PATHINFO_EXTENSION);
            $source = $uploads_directory . $track['path'];
            $destination = "$disc_folder/$format/$track_num.$extension";
            if (file_exists($destination)) {
                echo "Skipping file copy. File exists: $destination<br>";
                continue;
            }
            copy($source, $destination);
            $seconds = (int)cli_program('soxi', '-D', [$destination]);
            cli('create', ['track', 'false', $album_release_id, $disc_num, $track_num, $seconds, $track['name']]);
            echo "Created track $track_num ($seconds sec)<br>";
        }
    }
    // Add and move attachments
    $num = 1;
    foreach ($attachments as $attachment) {
        if (!$attachment['keep']) {
            echo 'Skipping attachment ' . $attachment['path'] . '<br>';
            continue;
        }
        $type = $attachment['target'];
        $description = '';
        cli('create', ['album_attachment', 'false', $album_release_id, $num, $type, $description]);
        echo "Created attachment $num ... ";
        $extension = pathinfo($attachment['path'], PATHINFO_EXTENSION);
        $source = $uploads_directory . $attachment['path'];
        $destination = "$albums_directory/$album_release_id/$num.$extension";
        if (file_exists($destination)) {
            echo "Skipping file copy. File exists: $destination<br>";
            continue;
        }
        copy($source, $destination);
        echo "Copied to $destination<br>";
        $num++;
    }
}

function add_group($args) {
    require_fields(['name', 'tags', 'country', 'website', 'description', 'people'], $_POST);
    $name = $_POST['name'];
    $tags = $_POST['tags'];
    $country = $_POST['country'];
    $website = $_POST['website'];
    $description = $_POST['description'];
    $people = $_POST['people'];
    $imageurl = (isset($_POST['imageurl']) ? $_POST['imageurl'] : []);
    $imagefile = (isset($_POST['imagefile']) ? $_POST['imagefile'] : []);
    if (!is_array($tags) || !is_array($people) || !is_array($imageurl) || !is_array($imagefile)) {
        return;
    }
    // Group
    $group_id = (int)cli('create', ['group', 'true', $country, $name, $website, $description]);
    if ($group_id === 0) {
        echo 'Failed to create group.';
        return;
    }
    $groups_path = cli('path', ['groups']);
    mkdir("$groups_path/$group_id");
    // Tags
    $priority = 1;
    foreach ($tags as $tag) {
        echo cli('create', ['group_tag', 'false', $group_id, $tag, $priority]);
        echo "Created group tag \"$tag\" with priority $priority<br>";
        $priority++;
    }
    // People
    require_fields(['id', 'role'], $people);
    foreach ($people['id'] as $index => $person_id) {
        $role = $people['role'][$index];
        $started_at = '';
        $ended_at = '';
        cli('create', ['group_member', 'false', $group_id, $person_id, $role, $started_at, $ended_at]);
    }
    // Images
    $images = [];
    $num = 1;
    if (has_fields(['background', 'file', 'description'], $imageurl)) {
        foreach ($imageurl['description'] as $index => $description) {
            $url = $imageurl['file'][$index];
            if ($url === '') {
                continue;
            }
            $file = @file_get_contents($url);
            if ($file === false) {
                echo 'Failed to download image.<br>';
                continue;
            }
            $images[] = [
                'num' => $num,
                'file' => $file,
                'description' => $description
            ];
            $num++;
        }
    }
    if (has_fields(['background', 'description'], $imagefile)) {
        foreach ($imagefile['description'] as $index => $description) {
            $file = file_get_contents($_FILES['imagefile']['tmp_name']['file'][$index]);
            if ($file === false) {
                echo 'Failed to read image.<br>';
                continue;
            }
            $images[] = [
                'num' => $num,
                'file' => $file,
                'description' => $description
            ];
            $num++;
        }
    }
    foreach ($images as $image) {
        $file = $image['file'];
        $num = $image['num'];
        $description = $image['description'];
        $extension = get_extension_for_content($file);
        $destination = "$groups_path/$group_id/$num.$extension";
        $written = @file_put_contents($destination, $file);
        if ($written === false) {
            echo 'Failed to save image to ' . $destination;
            continue;
        }
        cli('create', ['group_image', 'false', $group_id, $num, $description]);
    }
}

function add_session_track($args) {
    $user = get_user();
    if ($user === false) {
        return;
    }
    require_fields(['album', 'disc', 'track'], $_POST);
    $album = (int)$_POST['album'];
    $disc = (int)$_POST['disc'];
    $track = (int)$_POST['track'];
    $num = (int)cli('create', ['session_track', 'false', $user, $album, $disc, $track]);
    echo cli('render', ['session_track', $user, $num]);
}

function transcode($args) {
    if (get_user() === false) {
        return;
    }
    require_fields(['album', 'format'], $_POST);
    $album = (int)$_POST['album'];
    $format = $_POST['format'];
    if (!is_allowed_format($format)) {
        return;
    }
    set_time_limit(15 * 60);
    echo cli('transcode', [$album, $format]);
}

function render($args) {
    $ajax = ($args[0] === 'ajax');
    if ($ajax) {
        array_shift($args);
    } else {
        $args = array_merge(['main', 'dib'], $args);
    }
    foreach ($args as &$arg) {
        $arg = urldecode($arg);
    }
    echo cli('render', $args);
}

function form($args) {
    if (count($args) < 1) {
        return;
    }
    $command = $args[0];
    array_shift($args);
    if ($command === 'upload') {
        upload($args);
    } else if ($command === 'attach') {
        attach($args);
    } else if ($command === 'prepare') {
        prepare($args);
    } else if ($command === 'addgroup') {
        add_group($args);
    } else if ($command === 'addsessiontrack') {
        add_session_track($args);
    } else if ($command === 'transcode') {
        transcode($args);
    }
}

function start($args) {
    if (count($args) === 0) {
        render(['main', 'dib', 'playlists']);
        return;
    }
    if ($args[0] === 'form') {
        array_shift($args);
        form($args);
    } else {
        render($args);
    }
}

start(get_args());
