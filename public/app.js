"use strict";

import { getAuthHeaders, handleAuthFailure } from "./auth.js";

const dropZone = document.getElementById("drop-zone");
const progressBar = document.getElementById("progress-bar");
const uploadFileName = document.getElementById("upload-file-name");
const fileInput = document.getElementById("file-input");
const fileList = document.getElementById("file-list");

// file drag & drop handlers: these utilize a hidden input element
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

/**
 * Download a file from server
 */
function downloadFile(file) {
  fetch(`/download/${encodeURIComponent(file)}`, {
    headers: getAuthHeaders(),
  })
    .then((r) => r.blob())
    .then((blob) => {
      const url = URL.createObjectURL(blob);
      const a = document.createElement("a");

      a.href = url;
      a.download = file;

      a.click();
    });
}

/**
 * Delete a uploaded server file
 */
async function deleteFile(file) {
  const response = await fetch(`/api/files/${file}`, {
    method: "DELETE",
    headers: getAuthHeaders(),
  });

  if (handleAuthFailure(response)) return;

  loadFiles();
}

/**
 * Get all uploaded server files and create WebUI entries with download links for each
 */
async function loadFiles() {
  const response = await fetch("/api/files", {
    headers: getAuthHeaders(),
  });

  if (!response.ok) {
    if (handleAuthFailure(response)) return;
    alert("Request failed");
    return;
  }

  const files = await response.json();

  fileList.innerHTML = "";

  for (const file of files) {
    const div = document.createElement("div");
    div.className = "file-entry";

    const a = document.createElement("a");
    a.textContent = file;
    a.style.cursor = "pointer";
    a.addEventListener("click", () => downloadFile(file));

    const btn = document.createElement("button");
    btn.textContent = "Delete";
    btn.addEventListener("click", () => deleteFile(file));

    div.appendChild(a);
    div.appendChild(btn);
    fileList.appendChild(div);
  }
}

/**
 * Handles a single file upload with progress bar updates
 */
async function uploadFile(file) {
  return new Promise((resolve, reject) => {
    const formData = new FormData();

    formData.append("file", file);

    const xhr = new XMLHttpRequest();

    xhr.open("POST", "/api/upload");
    xhr.setRequestHeader(
      "Authorization",
      "Bearer " + localStorage.getItem("token"),
    );

    uploadFileName.innerHTML = file.name;

    xhr.upload.addEventListener("progress", (e) => {
      if (!e.lengthComputable) return;

      const pct = (e.loaded / e.total) * 100;

      progressBar.style.width = pct + "%";
    });

    xhr.addEventListener("load", () => {
      if (xhr.status === 401) {
        localStorage.removeItem("token");
        window.location.href = "/login.html";
        return;
      }

      progressBar.style.width = "100%";

      setTimeout(() => {
        progressBar.style.width = "0%";
        uploadFileName.innerHTML = "";
      }, 500);

      resolve();
    });

    xhr.addEventListener("error", () => {
      alert("Upload failed: " + file.name);

      reject();
    });

    xhr.send(formData);
  });
}

/**
 * Uploads selected files to server
 */
async function uploadFiles(files) {
  for (const file of files) {
    await uploadFile(file);
  }

  loadFiles();
}

loadFiles();
