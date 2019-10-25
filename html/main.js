window.addEventListener('popstate', () => location.reload());

function setAppropriateBackground() {
    const path = location.pathname;
    const index = path.lastIndexOf('group/');
    let url = '/img/bg.jpg';
    if (index >= 0) {
        url = '/img/group/' + parseInt(path.substr(index + 'group/'.length));
    }
    let main = document.getElementsByTagName('main')[0];
    main.style.backgroundImage = "url('" + url + "')";
}

document.addEventListener('DOMContentLoaded', () => {
    let audio = document.getElementById('audio');
    if (audio !== null) {
        audio.addEventListener('ended', () => {
            let active = document.querySelector('#playlist li.active');
            if (active !== null) {
                active.nextElementSibling.click();
            }
        });
    }
    setAppropriateBackground();
});

function removeClassFromAll(elements, className) {
    for (let element of elements) {
        element.classList.remove(className);
    }
}

function onNavClick(target) {
    removeClassFromAll(target.parentNode.children, 'active');
    target.classList.add('active');
}

function ajaxGet(path, done) {
    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
            done(this.responseText, this.status);
            setAppropriateBackground();
        }
    };
    xhttp.open('get', path, true);
    xhttp.send();
}

function ajaxPost(path, data, done, progress) {
    let xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && done !== undefined) {
            done(this.responseText, this.status);
        }
    };
    if (progress !== undefined) {
        xhttp.upload.addEventListener('progress', progress);
    }
    xhttp.open('post', path, true);
    if (data instanceof FormData) {
        xhttp.send(data);
    } else {
        let formData = new FormData();
        for (const key in data) {
            formData.append(key, data[key]);
        }
        xhttp.send(formData);
    }
}

function hasAnyClass(element, classNames) {
    for (let className of classNames) {
        if (element.classList.contains(className)) {
            return true;
        }
    }
    return false;
}

function addToPlaylist(album, disc, track) {
    ajaxPost('/form/addsessiontrack', {
        album: album,
        disc: disc,
        track: track
    }, response => document.querySelector('#playlist ul').innerHTML += response);
}

function onInternalLinkClick(target) {
    if (hasAnyClass(target, ['external-link', 'queue-track', 'play-track', 'queue-album', 'play-album'])) {
        return;
    }
    let section = document.querySelector('main section');
    ajaxGet('/render' + target.getAttribute('href'), response => section.innerHTML = response);
    history.pushState({}, '', target.getAttribute('href'));
}

function playTrack(album, disc, track) {
    let audio = document.getElementById('audio');
    audio.setAttribute('src', '/track/' + album + '/' + disc + '/' + track);
    audio.play();
}

function onAddGroupForm(target) {
    target.querySelector('button[type=submit]').setAttribute('disabled', true);
    let data = new FormData();
    data.append('tags', target.querySelector('input[name=tags]').value);
    appendFormData(data, ['name', 'country', 'website', 'description']);
    for (let person of target.querySelectorAll('.group-people tr')) {
        appendFormDataArray(data, person, 'people', ['id', 'role']);
    }
    for (let image of target.querySelectorAll('#group_attachments tr')) {
        let fileInput = image.querySelector('input[name=file]');
        if (fileInput === null) {
            continue;
        }
        let name = 'url';
        let file = fileInput.value;
        if (fileInput.getAttribute('type') === 'file') {
            file = fileInput.files[0];
            name = 'file';
        }
        data.append('image-source', name);
        data.append('image-is-background', image.querySelector('input[name=background]').getAttribute('checked'));
        data.append('image-file', file);
        data.append('image-description', image.querySelector('textarea[name=imagedescription]').value);
    }
    ajaxPost('/form/addgroup', data, response => document.querySelector('main section').innerHTML = response);
}

function onClickA(event) {
    let target = event.target;
    if (target.parentNode.tagName === 'NAV') {
        event.preventDefault();
        onNavClick(target);
    }
    if (target.classList.contains('queue-track')) {
        event.preventDefault();
        let parts = target.getAttribute('href').split('/');
        const album = parts[2];
        const disc = parts[3];
        const track = parts[4];
        addToPlaylist(album, disc, track);
        return;
    }
    if (target.classList.contains('queue-album')) {
        event.preventDefault();
        let parts = target.getAttribute('href').split('/');
        const album = parts[2];
        addToPlaylist(album, 0, 0);
        return;
    }
    if (target.classList.contains('clear-session-playlist')) {
        event.preventDefault();
        ajaxPost('/form/clear-session-playlist', null, () => document.querySelector('#playlist ul').innerHTML = '');
        return;
    }
    if (!target.classList.contains('external-link')) {
        event.preventDefault();
        onInternalLinkClick(target);
    }
}

function onImportForm(target) {
    target.setAttribute('disabled', true);
    let data = new FormData();
    data.append('name', document.querySelector('input[name=name]').value);
    data.append('released-at', document.querySelector('input[name=released_at]').value);
    data.append('catalog', document.querySelector('input[name=catalog]').value);
    data.append('format', document.querySelector('select[name=format]').value);
    data.append('type', document.querySelector('select[name=type]').value);
    data.append('release-type', document.querySelector('select[name=release_type]').value);
    // TODO: multiple groups in frontend
    data.append('groups', document.querySelector('input[name=groups]').value);
    for (let attachment of document.querySelectorAll('.attachments tr:not(:first-child)')) {
        if (attachment.querySelector('input[name=keep]').getAttribute('checked') !== null) {
            data.append('attachment-type', attachment.querySelector('select[name=target]').value);
            data.append('attachment-path', attachment.querySelector('input[name=path]').value);
        }
    }
    for (let disc of document.querySelectorAll('.import-disc')) {
        let tracks = 0;
        let headers = 3;
        for (let track of disc.querySelectorAll('tr')) {
            if (headers > 0) {
                headers--;
                continue;
            }
            data.append('track-num', track.querySelector('input[name=num]').value);
            data.append('track-name', track.querySelector('input[name=name]').value);
            data.append('track-path', track.querySelector('input[name=path]').value);
            tracks++;
        }
        data.append('disc-num', disc.querySelector('input[name=num]').value);
        data.append('disc-name', disc.querySelector('input[name=name]').value);
        data.append('disc-tracks', tracks);
    }
    ajaxPost('/form/import', data, response => document.querySelector('main section').innerHTML = response);
}

function onAttachGroupImage() {
    let form = document.getElementById('attach_group_image_wrapper');
    let newRow = form.querySelector('tr').cloneNode(true);
    const cleanAttachForm = form => {
        form.querySelector('input[name=background]').removeAttribute('checked');
        form.querySelector('input[name=file]').value = '';
        form.querySelector('textarea[name=imagedescription]').value = '';
    };
    cleanAttachForm(form);
    newRow.querySelector('#attach_file').removeAttribute('id');
    newRow.querySelector('.attach-group-image').parentNode.remove();
    let attachments = document.getElementById('group_attachments');
    attachments.appendChild(newRow);
}

function onClickLoginSubmit() {
    ajaxPost('/form/login', {
        name: document.getElementById('name').value,
        password: document.getElementById('password').value
    }, response => location.href = '/');
}

function onClickRegisterSubmit() {
    ajaxPost('/form/register', {
        name: document.getElementById('name').value,
        password: document.getElementById('password').value
    }, response => location.href = '/');
}

function onClickButton(event) {
    let target = event.target;
    if (target.classList.contains('submit-import')) {
        onImportForm(target);
    } else if (target.classList.contains('attach-group-image')) {
        event.preventDefault();
        onAttachGroupImage();
    } else if (target.classList.contains('login-submit')) {
        onClickLoginSubmit();
    } else if (target.classList.contains('register-submit')) {
        onClickRegisterSubmit();
    }
}

function onClickLi(target) {
    let list = target.parentNode;
    let wrapper = list.parentNode;
    if (wrapper === null) {
        return;
    }
    if (wrapper.getAttribute('id') === 'playlist') {
        removeClassFromAll(list.children, 'active');
        target.classList.add('active');
        playTrack(target.dataset.album, target.dataset.disc, target.dataset.track);
    } else if (list.classList.contains('remote-directory-list')) {
        ajaxPost('/form/downloadremote', { directory: target.innerHTML });
        target.innerHTML = '';
    }
}

document.addEventListener('click', event => {
    let target = event.target;
    if (target.tagName === 'A') {
        onClickA(event);
    } else if (target.tagName === 'LI') {
        onClickLi(target);
    } else if (target.tagName === 'BUTTON') {
        onClickButton(event);
    }
});

function onUploadForm(target) {
    target.querySelector('button').setAttribute('disabled', true);
    let onProgress = event => {
        let percent = 0;
        if (event.lengthComputable) {
            const position = event.loaded || event.position;
            percent = Math.ceil(position / event.total * 100);
        }
        document.getElementById('prog').innerHTML = percent + '%';
    };
    ajaxPost(target.getAttribute('action'), new FormData(target), response => target.append(response), onProgress);
}

function onAttachForm(target) {
    target.querySelector('button').setAttribute('disabled', '');
    ajaxPost(target.getAttribute('action'), new FormData(target), response => {
        let attachments = document.getElementsByClassName('attachments')[0];
        attachments.innerHTML += response;
        target.querySelector('input').setAttribute('value', '');
        target.querySelector('button').removeAttribute('disabled');
    });
}

document.addEventListener('submit', event => {
    event.preventDefault();
    let target = event.target;
    if (target.classList.contains('upload-form')) {
        onUploadForm(target);
    } else if (target.classList.contains('attach-form')) {
        onAttachForm(target);
    } else if (target.classList.contains('add-group-form')) {
        onAddGroupForm(target);
    }
});

document.addEventListener('change', event => {
    let target = event.target;
    if (target.classList.contains('file-upload-method')) {
        const input = document.getElementById(target.dataset.input);
        let file = (input === null ? target.nextElementSibling : input);
        file.setAttribute('type', target.value);
    }
});

function appendFormData(data, names) {
    for (const name of names) {
        let input = document.querySelector('[name=' + name + ']');
        if (input !== null) {
            data.append(name, input.value);
        }
    }
}

function appendFormDataArray(data, element, key, names) {
    for (const name of names) {
        let input = element.querySelector('[name=' + name + ']');
        if (input !== null) {
            data.append(key + '-' + name, input.value);
        }
    }
}

document.addEventListener('focus', event => {
    let target = event.target;
    if (target.tagName === 'INPUT') {
        if (target.getAttribute('type') === 'search') {
            target.nextElementSibling.style.display = 'block';
            target.classList.add('is-active');
        }
    }
});

document.addEventListener('input', event => {
    let target = event.target;
    if (target.tagName === 'INPUT') {
        if (target.getAttribute('type') === 'search') {
            let result = target.nextElementSibling;
            let query = encodeURIComponent(target.value);
            ajaxGet('/render' + target.dataset.search + '/' + query, response => {
                result.innerHTML = response;
                result.style.display = 'block'; // fade in?
            });
        }
    }
});

document.addEventListener('blur', event => {
    let target = event.target;
    if (target.tagName === 'INPUT') {
        if (target.getAttribute('type') === 'search') {
            setTimeout(() => target.nextElementSibling.style.display = 'none', 130);
            target.classList.remove('is-active');
        }
    }
}, true);

document.addEventListener('click', event => {
    let target = event.target;
    if (target.matches('input[type=search] + ul > li')) {
        let input = target.parentNode.previousElementSibling;
        let hidden = input.previousElementSibling;
        hidden.value = target.dataset.value;
        input.value = target.innerHTML;
    }
});
