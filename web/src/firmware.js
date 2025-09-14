import { resetMessage, setDisabled, showMessage, handleFetch } from './utils.js';

const submitUpdate = document.getElementById('submit-update');
setDisabled(submitUpdate, true);
const feedbackMessage = document.getElementById('submit-update-message');
resetMessage(feedbackMessage);
const updateFileInput = document.getElementById('update-file');
updateFileInput.value = null;
const updateFileLabel = updateFileInput.parentElement.querySelector('span');
const initialFileLabelText = updateFileLabel.innerText;
const fileRemove = document.getElementById('remove-update-file');
fileRemove.classList.add('is-hidden');

export const firmware = (baseUrl) => {
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
    document.querySelectorAll('.upload-update input[name="update-mode"]')
        .forEach((input) => {
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

        const url = new URL('ota', baseUrl);
        url.searchParams.set('mode', mode);
        url.searchParams.set('size', file.size);
        if (
            await handleFetch(url)
            && await handleFetch(new URL('ota', baseUrl), { method: 'POST', body: file })
        ) {
            showMessage(feedbackMessage, 'Update applyed, rebooting', 'is-success');

            return;
        }

        setDisabled(submitUpdate, false);
        showMessage(feedbackMessage, 'Error applying update', 'is-error');
    });
};
