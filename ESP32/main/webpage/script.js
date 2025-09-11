document.addEventListener('DOMContentLoaded', function () {
    const uploadBtn = document.getElementById('uploadBtn');
    const downloadBtn = document.getElementById('downloadBtn');
    const firmwareFile = document.getElementById('firmwareFile');
    const uploadProgressBar = document.getElementById('uploadProgressBar');
    const uploadProgressContainer = document.getElementById('uploadProgressContainer');
    const uploadStatus = document.getElementById('uploadStatus');
    const downloadProgressBar = document.getElementById('downloadProgressBar');
    const downloadProgressContainer = document.getElementById('downloadProgressContainer');
    const downloadStatus = document.getElementById('downloadStatus');

    // Upload firmware file
    uploadBtn.addEventListener('click', async function () {
        if (!firmwareFile.files.length) {
            showStatus(uploadStatus, 'Please select a file first.', 'error');
            return;
        }

        const file = firmwareFile.files[0];
        const formData = new FormData();
        formData.append('firmware', file);

        try {
            uploadBtn.disabled = true;
            uploadProgressContainer.style.display = 'block';
            uploadStatus.style.display = 'none';

            const xhr = new XMLHttpRequest();
            xhr.open('POST', '/upload', true);

            // Upload progress
            xhr.upload.onprogress = function (e) {
                if (e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    uploadProgressBar.style.width = percent + '%';
                    uploadProgressBar.textContent = percent + '%';
                }
            };

            xhr.onload = function () {
                if (xhr.status === 200) {
                    showStatus(uploadStatus, 'Firmware uploaded successfully!', 'success');
                } else {
                    showStatus(uploadStatus, 'Upload failed: ' + xhr.responseText, 'error');
                }
                uploadBtn.disabled = false;
            };

            xhr.onerror = function () {
                showStatus(uploadStatus, 'Upload failed. Please try again.', 'error');
                uploadBtn.disabled = false;
            };

            xhr.send(formData);
        } catch (error) {
            showStatus(uploadStatus, 'Error: ' + error.message, 'error');
            uploadBtn.disabled = false;
        }
    });

    // Download firmware to device
    downloadBtn.addEventListener('click', async function () {
        try {
            downloadBtn.disabled = true;
            downloadProgressContainer.style.display = 'block';
            downloadStatus.style.display = 'none';

            // Simulate progress for demo (replace with actual progress events)
            let progress = 0;
            const interval = setInterval(() => {
                progress += 5;
                if (progress > 100) progress = 100;
                downloadProgressBar.style.width = progress + '%';
                downloadProgressBar.textContent = progress + '%';

                if (progress === 100) {
                    clearInterval(interval);
                    showStatus(downloadStatus, 'Firmware downloaded to device successfully!', 'success');
                    downloadBtn.disabled = false;
                }
            }, 200);

            // In a real implementation, you would use fetch or WebSocket for progress updates
            
            const response = await fetch('/download', {
                method: 'POST'
            });
            
            if (response.ok) {
                showStatus(downloadStatus, 'Firmware downloaded to device successfully!', 'success');
            } else {
                showStatus(downloadStatus, 'Download failed: ' + (await response.text()), 'error');
            }
            downloadBtn.disabled = false;
            
        } catch (error) {
            showStatus(downloadStatus, 'Error: ' + error.message, 'error');
            downloadBtn.disabled = false;
        }
    });

    function showStatus(element, message, type) {
        element.textContent = message;
        element.className = 'status ' + type;
        element.style.display = 'block';
    }
});