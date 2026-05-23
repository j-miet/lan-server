const dropZone = document.getElementById("drop-zone");
const fileInput = document.getElementById("file-input");
const fileList = document.getElementById("file-list");

// file drag & drop handlers: these call hidden input element
dropZone.addEventListener("click", () => fileInput.click());

dropZone.addEventListener("dragover", (e) => {
  e.preventDefault();
  dropZone.classList.add("dragover");
});

dropZone.addEventListener("dragleave", () =>
  dropZone.classList.remove("dragover"),
);

dropZone.addEventListener("drop", (e) => {
  e.preventDefault();
  dropZone.classList.remove("dragover");

  const files = e.dataTransfer.files;

  uploadFiles(files);
});

fileInput.addEventListener("change", () => uploadFiles(fileInput.files));

async function deleteFile(file) {
  const response = await fetch(`/api/files/${file}`, { method: "DELETE" });

  if (!response.ok) {
    alert("Delete failed");

    return;
  }

  loadFiles();
}

/**
 * Get all uploaded server files and create WebUI entries with download links for each
 */
async function loadFiles() {
  const response = await fetch("/api/files");

  const files = await response.json();

  fileList.innerHTML = "";

  for (const file of files) {
    const div = document.createElement("div");

    div.className = "file-entry";
    div.innerHTML = `
        <a href="/download/${encodeURIComponent(file)}">
            ${file}
        </a>
        
        <button onclick="deleteFile('${encodeURIComponent(file)}')">
            Delete
        </button>
        `;

    fileList.appendChild(div);
  }
}

/**
 * Uploads selected files (click or drag&drop) to server
 */
async function uploadFiles(files) {
  for (const file of files) {
    const formData = new FormData();

    formData.append("file", file);

    const response = await fetch("/api/upload", {
      method: "POST",
      body: formData,
    });

    if (!response.ok) {
      alert("Upload failed: " + file.name);
    }
  }

  loadFiles();
}

loadFiles();
