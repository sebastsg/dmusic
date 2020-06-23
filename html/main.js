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
	ajaxPost('/form/add-session-track', {
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
	ajaxPost('/form/add-group', data, response => document.querySelector('main section').innerHTML = response);
}

function onEditGroupForm(target) {
	target.querySelector('button[type=submit]').setAttribute('disabled', true);
	let data = new FormData();
	data.append('id', target.dataset.group);
	appendFormData(data, ['name', 'country', 'website', 'description']);
	for (const alias of target.querySelector('.group-aliases').tBodies[0].rows) {
		data.append('alias', alias.children[0].children[0].value);
	}
	ajaxPost('/form/edit-group', data, () => location.href = location.href.replace('edit-group', 'group'));
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
	} else if (target.classList.contains('queue-album')) {
		event.preventDefault();
		let parts = target.getAttribute('href').split('/');
		const album = parts[2];
		addToPlaylist(album, 0, 0);
	} else if (target.classList.contains('clear-session-playlist')) {
		event.preventDefault();
		ajaxPost('/form/clear-session-playlist', null, () => document.querySelector('#playlist ul').innerHTML = '');
	} else if (!target.classList.contains('external-link')) {
		event.preventDefault();
		onInternalLinkClick(target);
	}
}

function onImportForm(target) {
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
	let isCoverAttachmentPresent = false;
	for (let disc of document.querySelectorAll('.import-disc')) {
		let tracks = 0;
		let headers = 3;
		let addedTrackNums = [];
		for (let track of disc.querySelectorAll('tr')) {
			if (headers > 0) {
				headers--;
				continue;
			}
			const trackNum = track.querySelector('input[name=num]').value;
			if (addedTrackNums.includes(trackNum)) {
				alert('Track numbers are invalid.');
				return;
			}
			addedTrackNums.push(trackNum);
			data.append('track-num', trackNum);
			data.append('track-name', track.querySelector('input[name=name]').value);
			data.append('track-path', track.querySelector('input[name=path]').value);
			tracks++;
		}
		data.append('disc-num', disc.querySelector('input[name=num]').value);
		data.append('disc-name', disc.querySelector('input[name=name]').value);
		data.append('disc-tracks', tracks);
		isCoverAttachmentPresent = true;
	}
	if (isCoverAttachmentPresent || target.dataset.ignoreErrors === 'yes') {
		ajaxPost('/form/import', data, response => document.querySelector('main section').innerHTML = response);
		target.setAttribute('disabled', '');
	} else {
		target.nextElementSibling.style.display = 'block';
	}
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

function onLoginSubmit() {
	ajaxPost('/form/login', {
		name: document.getElementById('name').value,
		password: document.getElementById('password').value
	}, response => location.href = '/');
}

function onRegisterSubmit() {
	ajaxPost('/form/register', {
		name: document.getElementById('name').value,
		password: document.getElementById('password').value
	}, response => location.href = '/');
}

function onEditGroupTags(group, destination) {
	ajaxGet('/render/group-tags/' + group + '/edit', response => destination.innerHTML = response);
}

function onDoneEditingGroupTags(group, destination) {
	ajaxGet('/render/group-tags/' + group, response => destination.innerHTML = response);
}

function onDeleteGroupTag(target) {
	ajaxPost('/form/delete-group-tag', {
		group: target.dataset.group,
		tag: target.dataset.tag
	});
	target.parentNode.remove();
}

function onAddGroupTag(target) {
	let input = target.parentNode.querySelector('input[name=tag]').nextElementSibling;
	ajaxPost('/form/add-group-tag', {
		group: target.dataset.group,
		tag: input.value
	}, response => document.getElementById('group_tags_being_edited').innerHTML += response);
	input.value = '';
}

function onAddImportDisc() {
	ajaxGet('/render/import-disc', response => document.getElementById('import_discs').innerHTML += response);
}

function updateImportTrackNums() {
	let discs = document.getElementsByClassName('import-disc');
	for (let discNum = 1; discNum <= discs.length; discNum++) {
		let disc = discs[discNum - 1];
		let tracks = disc.tBodies[0].rows;
		for (let trackNum = 1; trackNum <= tracks.length; trackNum++) {
			tracks[trackNum - 1].querySelector('input[name=num]').value = trackNum;
		}
		disc.tHead.querySelector('input[name=num]').value = discNum;
	}
}

function moveImportTrack(track, otherTrack, otherDisc, fallbackDisc) {
	if (otherTrack !== null) {
		const rowHtml = track.innerHTML;
		track.innerHTML = otherTrack.innerHTML;
		otherTrack.innerHTML = rowHtml;
	} else {
		if (otherDisc !== null) {
			otherDisc.tBodies[0].innerHTML += track.innerHTML;
			track.remove();
		} else if (fallbackDisc !== null) {
			fallbackDisc.tBodies[0].innerHTML += track.innerHTML;
			track.remove();
		}
	}
	updateImportTrackNums();
}

function moveImportTrackUp(track) {
	let previousTrack = track.previousElementSibling;
	let previousDisc = track.parentNode.parentNode.previousElementSibling;
	let nextDisc = track.parentNode.parentNode.nextElementSibling;
	moveImportTrack(track, previousTrack, previousDisc, nextDisc);
}

function moveImportTrackDown(track) {
	let nextTrack = track.nextElementSibling;
	let previousDisc = track.parentNode.parentNode.previousElementSibling;
	let nextDisc = track.parentNode.parentNode.nextElementSibling;
	moveImportTrack(track, nextTrack, nextDisc, previousDisc);
}

function onLogout() {
	ajaxPost('/form/logout', {}, () => location.replace('/'));
}

function onDownloadRemoteEntry(target) {
	ajaxPost('/form/download-remote-entry', { name: target.parentNode.dataset.name }, () => {
		target.style.color = 'green';
	});
	target.setAttribute('disabled', '');
}

function onHideRemoteEntry(target) {
	ajaxPost('/form/hide-remote-entry', { name: target.parentNode.dataset.name }, () => {
		target.previousElementSibling.setAttribute('disabled', '');
	});
	target.setAttribute('disabled', '');
}

function onDeleteUpload(target) {
	ajaxPost('/form/delete-upload', { prefix: target.dataset.prefix });
	target.setAttribute('disabled', '');
	target.parentNode.previousElementSibling.children[0].setAttribute('disabled', '');
}

function onToggleFavourite(target) {
	target.setAttribute('disabled', '');
	ajaxPost('/form/toggle-favourite-group', { group: target.dataset.group }, response => {
		target.removeAttribute('disabled');
		target.innerHTML = response;
	});
}

function onLoadRemoteEntries(target) {
	target.setAttribute('disabled', '');
	ajaxGet('/render/remote-entries', response => {
		document.getElementById('remote_entries').innerHTML = response;
		target.removeAttribute('disabled');
	});
}

function onAddGroupAlias(target) {
	const id = target.parentNode.dataset.group;
	let tableBody = target.previousElementSibling.tBodies[0];
	for (let row of tableBody.rows) {
		let alias = row.children[0].children[0];
		alias.setAttribute('value', alias.value);
	}
	ajaxGet('/render/edit-group-alias/' + id, response => tableBody.innerHTML += response);
}

function onDeleteGroupAlias(target) {
	let row = target.parentNode.parentNode;
	ajaxPost('/form/delete-group-alias', {
		group: target.dataset.group,
		alias: row.children[0].children[0].value
	}, () => row.remove());
}

function onClickButton(event) {
	let target = event.target;
	if (target.classList.contains('submit-import')) {
		onImportForm(target);
	} else if (target.classList.contains('attach-group-image')) {
		event.preventDefault();
		onAttachGroupImage();
	} else if (target.classList.contains('login-submit')) {
		onLoginSubmit();
	} else if (target.classList.contains('register-submit')) {
		onRegisterSubmit();
	} else if (target.classList.contains('edit-group-tags')) {
		onEditGroupTags(target.dataset.group, target.parentNode);
	} else if (target.classList.contains('done-editing-group-tags')) {
		onDoneEditingGroupTags(target.dataset.group, target.parentNode);
	} else if (target.classList.contains('delete-group-tag')) {
		onDeleteGroupTag(target);
	} else if (target.classList.contains('add-group-tag')) {
		onAddGroupTag(target);
	} else if (target.classList.contains('logout-submit')) {
		onLogout();
	} else if (target.classList.contains('add-import-disc')) {
		onAddImportDisc();
	} else if (target.classList.contains('move-import-track-up')) {
		moveImportTrackUp(target.parentNode.parentNode);
	} else if (target.classList.contains('move-import-track-down')) {
		moveImportTrackDown(target.parentNode.parentNode);
	} else if (target.classList.contains('download-remote-entry')) {
		onDownloadRemoteEntry(target);
	} else if (target.classList.contains('hide-remote-entry')) {
		onHideRemoteEntry(target);
	} else if (target.classList.contains('delete-upload')) {
		onDeleteUpload(target);
	} else if (target.classList.contains('toggle-favourite')) {
		onToggleFavourite(target);
	} else if (target.classList.contains('load-remote-entries')) {
		onLoadRemoteEntries(target);
	} else if (target.classList.contains('add-group-alias')) {
		onAddGroupAlias(target);
	} else if (target.classList.contains('delete-group-alias')) {
		onDeleteGroupAlias(target);
	} else if (target.classList.contains('ignore-errors')) {
		for (let element of document.getElementsByClassName(target.dataset.targetClass)) {
			element.dataset.ignoreErrors = 'yes';
		}
		target.remove();
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
	}
}

document.addEventListener('click', event => {
	let target = event.target;
	if (target.tagName === 'DIV') {
		if (target.parentNode.classList.contains('thumbnail')) {
			event.preventDefault();
			onInternalLinkClick(target.parentNode);
		}
	} else if (target.tagName === 'A') {
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

function onFilterRemoteEntries(value) {
	let entries = document.getElementById('remote_entries').children;
	for (let entry of entries) {
		entry.style.display = entry.innerText.toLowerCase().includes(value.toLowerCase()) ? 'table-row' : 'none';
	}
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
	} else if (target.classList.contains('edit-group-form')) {
		onEditGroupForm(target);
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
			if (target.dataset.filter === 'remote-entries') {
				onFilterRemoteEntries(target.value);
			} else {
				let query = encodeURIComponent(target.value);
				if (target.dataset.render.length === 0) {
					let destination = target.nextElementSibling;
					ajaxGet('/render/search/' + target.dataset.type + '/' + query, response => {
						destination.innerHTML = response;
						destination.style.display = 'block';
					});
				} else {
					let destination = document.querySelector(target.dataset.results);
					ajaxGet('/render/expanded-search/' + target.dataset.type + '/' + query, response => {
						destination.innerHTML = response
					});
				}
			}
		}
	}
});

document.addEventListener('blur', event => {
	let target = event.target;
	if (target.tagName === 'INPUT') {
		if (target.getAttribute('type') === 'search' && target.dataset.filter === undefined) {
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

document.addEventListener('keyup', event => {
	const Del = 46;
	const A = 65;
	let playlist = document.querySelector('#playlist ul');
	if (event.which === Del && !playlist.classList.contains('is-deleting-track')) {
		let hovered = playlist.querySelector('li:hover');
		if (hovered !== null) {
			playlist.classList.add('is-deleting-track');
			ajaxPost('/form/delete-session-track', {
				num: Array.from(playlist.children).indexOf(hovered) + 1
			}, () => playlist.classList.remove('is-deleting-track'));
			hovered.remove();
		}
	}
	if (event.ctrlKey && event.altKey && event.which == A) {
		ajaxPost('/form/toggle-edit-mode', {}, () => location.reload());
	}
});
