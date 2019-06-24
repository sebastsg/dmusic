$(window).bind('popstate', () => location.reload());

$(document).ready(() => {
    $('#audio').on('ended', () => {
        let $active = $('#playlist li.active');
        if ($active.length === 0) {
            return;
        }
        $active.next().click();
    });
});

$(document).on('click', 'nav a', function (event) {
    $('nav a').removeClass('active');
    $(this).addClass('active');
});

$(document).on('click', 'a', function (event) {
    if ($(this).hasClass('external-link')) {
        return;
    }
    if ($(this).hasClass('queue-track') || $(this).hasClass('play-track')) {
        return;
    }
    if ($(this).hasClass('queue-album') || $(this).hasClass('play-album')) {
        return;
    }
    event.preventDefault();
    $('main section').load('/ajax' + $(this).attr('href'));
    history.pushState({}, '', $(this).attr('href'));
});

function addToPlaylist(album, disc, track) {
    $.ajax({
        url: '/form/addsessiontrack',
        type: 'post',
        data: {
            album: album,
            disc: disc,
            track: track
        },
        success: response => $('#playlist ul').append(response)
    });
}

$(document).on('click', '.queue-track', function (event) {
    event.preventDefault();
    let parts = $(this).attr('href').split('/');
    const album = parts[2];
    const disc = parts[3];
    const track = parts[4];
    addToPlaylist(album, disc, track);
});

function playTrack(album, disc, track) {
    let $audio = $('#audio');
    $audio.attr('src', '/data/albums/' + album + '/' + disc + '/mp3-320/' + track + '.mp3');
    $audio[0].play();
}

$(document).on('click', '#playlist li', function (event) {
    $('#playlist li').removeClass('active');
    $(this).addClass('active');
    const album = $(this).data('album');
    const disc = $(this).data('disc');
    const track = $(this).data('track');
    playTrack(album, disc, track);
});

$(document).on('submit', '.upload-form', function (event) {
    event.preventDefault();
    $(this).find('button').prop('disabled', true);
    let $self = $(this);
    let onProgress = event => {
        let percent = 0;
        if (event.lengthComputable) {
            let position = event.loaded || event.position;
            percent = Math.ceil(position / event.total * 100);
        }
        $self.find('#prog').html(percent + '%');
    };
    $.ajax({
        url: $(this).attr('action'),
        type: $(this).attr('method'),
        enctype: $(this).attr('enctype'),
        data: new FormData(this),
        contentType: false,
        processData: false,
        xhr: () => {
            let xhr = $.ajaxSettings.xhr();
            if (xhr.upload) {
                xhr.upload.addEventListener('progress', event => onProgress(event), false);
            }
            return xhr;
        },
        success: response => $self.append(response)
    });
});

$(document).on('change', '.file-upload-method', function (event) {
    const $method = $(this).val();
    const input = $(this).data('input');
    let $file = null;
    if (input === undefined) {
        $file = $(this).next();
    } else {
        $file = $(input);
    }
    if ($method === 'url') {
        $file.attr('type', 'url');
    } else if ($method === 'file') {
        $file.attr('type', 'file');
    }
});

$(document).on('submit', '.attach-form', function (event) {
    event.preventDefault();
    let $self = $(this);
    $self.find('button').prop('disabled', true);
    $.ajax({
        url: $(this).attr('action'),
        type: $(this).attr('method'),
        enctype: $(this).attr('enctype'),
        data: new FormData(this),
        contentType: false,
        processData: false,
        success: response => {
            $('.attachments').append(response);
            $self.find('input').val('');
            $self.find('button').prop('disabled', false);
        }
    });
});

$(document).on('click', '.submit-prepare', function (event) {
    $(this).prop('disabled', true);
    // TODO: multiple groups in frontend
    let groups = [
        $('input[name=groups]').val()
    ];
    let attachments = [];
    $('.attachments tr').each(function () {
        attachments.push({
            keep: $(this).find('input[name=keep]').prop('checked'),
            target: $(this).find('select[name=target]').val(),
            path: $(this).find('input[name=path]').val()
        });
    });
    let discs = [];
    $('.prepare-disc').each(function () {
        let disc = null;
        $(this).find('tr').each(function () {
            if ($(this).find('th').length !== 0) {
                return;
            }
            if (disc === null) {
                disc = {
                    num: $(this).find('input[name=num]').val(),
                    name: $(this).find('input[name=name]').val(),
                    tracks: []
                };
            } else {
                disc.tracks.push({
                    num: $(this).find('input[name=num]').val(),
                    name: $(this).find('input[name=name]').val(),
                    path: $(this).find('input[name=path]').val()
                });
            }
        });
        discs.push(disc);
    });
    $.ajax({
        url: '/form/prepare',
        type: 'post',
        data: {
            name: $('input[name=name]').val(),
            released_at: $('input[name=released_at]').val(),
            catalog: $('input[name=catalog]').val(),
            format: $('select[name=format]').val(),
            type: $('select[name=type]').val(),
            release_type: $('select[name=release_type]').val(),
            folder: $('input[name=folder]').val(),
            groups: groups,
            attachments: attachments,
            discs: discs
        },
        success: response => {
            $('main section').html(response);
        }
    });
});

function appendFormData(data, names) {
    for (let name of names) {
        let $input = $('[name=' + name + ']');
        if ($input.length > 0) {
            data.append(name, $input.val());
        }
    }
}

function appendFormDataArray(data, $element, key, names) {
    for (let name of names) {
        let $input = $element.find('[name=' + name + ']');
        if ($input.length > 0) {
            data.append(key + '[' + name + '][]', $input.val());
        }
    }
}

$(document).on('submit', '.add-group-form', function (event) {
    event.preventDefault();
    $(this).prop('disabled', true);
    let data = new FormData();
    let tags = $('input[name=tag]').val().split(',');
    for (let tag of tags) {
        data.append('tags[]', tag.trim());
    }
    appendFormData(data, ['name', 'country', 'website', 'description']);
    $('.group-people tr').each(function () {
        appendFormDataArray(data, $(this), 'people', ['id', 'role']);
    });
    $('.group-images tr').each(function () {
        let $fileInput = $(this).find('input[name=file]');
        let fileInput = $fileInput.get(0);
        if (fileInput === undefined) {
            return;
        }
        let name = 'imageurl';
        let file = $fileInput.val();
        if ($fileInput.attr('type') === 'file') {
            file = fileInput.files[0];
            name = 'imagefile';
        }
        data.append(name + '[background][]', $(this).find('input[name=background]').prop('checked'));
        data.append(name + '[file][]', file);
        data.append(name + '[description][]', $(this).find('textarea[name=imagedescription]').val());
    });
    $.ajax({
        url: '/form/addgroup',
        type: 'post',
        data: data,
        contentType: false,
        processData: false,
        success: response => {
            $('main section').html(response);
        }
    });
});

$(document).on('click', '.append-row', function (event) {
    let $table = $(this).closest('table');
    if ($table.length === 0) {
        return;
    }
    let $source = $($table.find('tr')[1]);
    let $copy = $source.clone();
    $copy.find('input').val('');
    $copy.find('input').prop('checked', false);
    $copy.find('textarea').val('');
    $copy.insertBefore($(this).parent().parent());
});

$(document).on('click', '.delete-row', function (event) {
    let $row = $(this).parent().parent();
    if ($row.parent().children('tr').length < 4) {
        return;
    }
    $row.remove();
});

$(document).on('focus', 'input[type=search]', function (event) {
    $(this).next().css('display', 'block');
    $(this).addClass('is-active');
});

$(document).on('input', 'input[type=search]', function (event) {
    let $result = $(this).next();
    let query = encodeURIComponent($(this).val());
    $.ajax({
        url: '/ajax' + $(this).data('search') + '/' + query,
        type: 'get',
        success: response => $result.html(response).fadeIn(200)
    });
});

$(document).on('blur', 'input[type=search]', function (event) {
    setTimeout(() => $(this).next().css('display', 'none'), 130);
    $(this).removeClass('is-active');
});

$(document).on('click', 'input[type=search] + ul > li', function (event) {
    let $input = $(this).parent().prev();
    let $hidden = $input.prev();
    $hidden.val($(this).data('value'));
    $input.val($(this).html());
});
