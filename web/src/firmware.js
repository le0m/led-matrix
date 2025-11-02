import { resetMessage, setDisabled, setProgress, showMessage } from './utils.js';

const submitUpdate = document.getElementById('submit-update');
setDisabled(submitUpdate, true);
const feedbackMessage = document.getElementById('submit-update-message');
resetMessage(feedbackMessage);
const uploadProgress = document.getElementById('submit-update-progress');
const updateFileInput = document.getElementById('update-file');
updateFileInput.value = null;
const updateFileLabel = updateFileInput.parentElement.querySelector('span');
const initialFileLabelText = updateFileLabel.innerText;
const fileRemove = document.getElementById('remove-update-file');
fileRemove.classList.add('is-hidden');

/**
 * Initialize firmware module.
 *
 * @param {import('./state.js').State} state
 */
export const firmware = (state) => {
	// Clear selected file
	fileRemove.addEventListener('click', () => {
		updateFileInput.value = null;
		updateFileLabel.innerText = initialFileLabelText;
		fileRemove.classList.add('is-hidden');
		setDisabled(submitUpdate, true);
		resetMessage(feedbackMessage);
	});

	// Enable/disable submit on input change
	updateFileInput.addEventListener('change', (e) => {
		resetMessage(feedbackMessage);
		if (e.target.files.length === 0) {
			updateFileLabel.innerText = initialFileLabelText;
			setDisabled(submitUpdate, true);
			fileRemove.classList.add('is-hidden');

			return;
		}

		updateFileLabel.innerText = e.target.files[0].name;
		setDisabled(submitUpdate, false);
		fileRemove.classList.remove('is-hidden');
	});
	document.querySelectorAll('.upload-update input[name="update-mode"]').forEach((input) => {
		input.addEventListener('click', () => {
			resetMessage(feedbackMessage);
			if (updateFileInput.files.length) {
				setDisabled(submitUpdate, false);
			}
		});
	});

	// Upload update
	submitUpdate.addEventListener('click', async (e) => {
		setDisabled(submitUpdate, true);
		showMessage(feedbackMessage, 'Uploading...', 'is-primary');
		const file = updateFileInput.files?.[0];
		if (!file) {
			console.error('Update file not selected');
			showMessage(feedbackMessage, 'File not selected', 'is-warning');

			return;
		}

		const mode = document.querySelector('.upload-update input[name="update-mode"]:checked')?.value;
		if (!mode) {
			console.error('Update mode not selected');
			showMessage(feedbackMessage, 'Mode not selected', 'is-warning');

			return;
		}

		uploadProgress.classList.remove('is-hidden');
		const success = await state.uploadOTA(file, mode, (progress) =>
			setProgress(uploadProgress, (100 * progress.current) / progress.total, 'is-primary'),
		);
		uploadProgress.classList.add('is-hidden');
		setProgress(uploadProgress, undefined);
		if (!success) {
			showMessage(feedbackMessage, 'Error applying update', 'is-error');
			setDisabled(submitUpdate, false);

			return;
		}

		showMessage(feedbackMessage, 'Update applyed, rebooting', 'is-success');
	});
};
