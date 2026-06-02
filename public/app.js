"use strict";

const dropZone = document.getElementById("drop-zone");
const toggle = document.getElementById("file-icons-toggle");
const fileInput = document.getElementById("file-input");
const fileList = document.getElementById("file-list");

// add any file formats here you wish to display in previews as plain text
const TEXT_TYPES = [
  "txt",
  "json",
  "log",
  "md",
  // languages
  "c",
  "h",
  "cpp",
  "hpp",
  "py",
  "js",
  "ts",
  "lua",
];

let terminalTextColor;
let allFiles = [];
let currentSort = "name";
let sortAscending = true;
let currentSearch = "";

// terminal text color buttons
document.querySelectorAll(".terminal-color-btn").forEach((button) => {
  button.addEventListener("click", () => {
    const color = button.dataset.color;

    const terminal = document.getElementById("terminal-container");
    terminal.style.color = color;
  });
});

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

// toolbar listeners
document.getElementById("file-search").addEventListener("input", (e) => {
  currentSearch = e.target.value;

  renderFiles();
});

document.getElementById("file-sort").value = "name"; // force name field as default
document.getElementById("file-sort").addEventListener("change", (e) => {
  currentSort = e.target.value;

  renderFiles();
});

document.getElementById("file-sort-order").addEventListener("click", () => {
  sortAscending = !sortAscending;

  document.getElementById("file-sort-order").textContent = sortAscending
    ? "↑"
    : "↓";

  renderFiles();
});

// this will explicitly flush the search bar contents
window.addEventListener("load", () => {
  const search = document.getElementById("file-search");

  search.value = "";
  currentSearch = "";

  renderFiles();
});

function getTimestamp() {
  const now = new Date();
  const date = now.toISOString().slice(0, 10);
  const time = now.toLocaleTimeString();
  return `${date} ${time}`;
}

function formatFileSize(bytes) {
  if (bytes < 1024) return bytes + " B";

  if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + " KB";

  if (bytes < 1024 * 1024 * 1024)
    return (bytes / (1024 * 1024)).toFixed(1) + " MB";

  return (bytes / (1024 * 1024 * 1024)).toFixed(1) + " GB";
}

function formatTimestamp(ts) {
  return new Date(ts * 1000).toLocaleString();
}

function togglePreview(button, filename, type) {
  const container = button.parentElement.querySelector(
    ".file-preview-container",
  );

  const isOpen = container.dataset.open === "1";

  // close all previews
  document.querySelectorAll(".file-preview-container").forEach((c) => {
    c.innerHTML = "";
    c.dataset.open = "0";
  });

  document.querySelectorAll(".preview-btn").forEach((btn) => {
    btn.textContent = "Preview";
  });

  if (isOpen) {
    container.dataset.open = "0";
    button.textContent = "Preview";
    return;
  }

  container.dataset.open = "1";
  button.textContent = "Close";

  const url = `/preview/${encodeURIComponent(filename)}`;

  // images
  if (["png", "jpg", "jpeg", "gif", "webp"].includes(type)) {
    container.innerHTML = `
      <img
        class="file-preview-image"
        src="${url}"
      />
    `;
  }

  // videos
  else if (["mp4", "webm"].includes(type)) {
    container.innerHTML = `
      <video
        class="file-preview-video"
        controls
        preload="metadata"
      >
        <source src="${url}">
      </video>
    `;
  }

  // text/json
  else if (["txt", "json", "log"].includes(type)) {
    fetch(url)
      .then((r) => r.text())
      .then((text) => {
        container.innerHTML = `
          <pre class="file-preview-text"></pre>
        `;

        container.querySelector("pre").textContent = text;
      });
  } else {
    container.innerHTML = `<div>No preview available</div>`;
  }
}

function renderFiles() {
  fileList.innerHTML = "";

  let filtered = [...allFiles];

  // search filter
  if (currentSearch.trim() !== "") {
    const q = currentSearch.toLowerCase();

    filtered = filtered.filter((file) => file.name.toLowerCase().includes(q));
  }

  // sorting
  filtered.sort((a, b) => {
    let result = 0;

    switch (currentSort) {
      case "name":
        result = a.name.localeCompare(b.name);
        break;

      case "type":
        result = a.type.localeCompare(b.type);

      case "size":
        result = a.size - b.size;
        break;

      case "modified":
        result = a.modified - b.modified;
        break;
    }

    return sortAscending ? result : -result;
  });

  for (const file of filtered) {
    const div = document.createElement("div");
    div.className = "file-entry";

    div.innerHTML = `
      <div class="file-info">
        <div class="file-name">
          ${file.name}
        </div>

        <div class="file-meta">
          ${file.type.toUpperCase()}
          •
          ${formatFileSize(file.size)}
          •
          ${formatTimestamp(file.modified)}
        </div>

        <div class="file-preview-container"></div>
      </div>

      <button class="preview-btn">
        Preview
      </button>

      <a class="download-link"
        href="/download/${encodeURIComponent(file.name)}">
        <button>
          Download
        </button>
      </a>

      <button class="delete-btn">
        Delete
      </button>
    `;

    const previewBtn = div.querySelector(".preview-btn");
    const deleteBtn = div.querySelector(".delete-btn");

    previewBtn.addEventListener("click", () => {
      togglePreview(previewBtn, file.name, file.type);
    });

    deleteBtn.addEventListener("click", async () => {
      await deleteFile(file.name);
    });

    fileList.appendChild(div);
  }
}

/**
 * Delete an uploaded server file
 */
async function deleteFile(file) {
  const response = await fetch(`/api/files/${encodeURIComponent(file)}`, {
    method: "DELETE",
  });

  loadFiles();
}

/**
 * Get all uploaded server files and create WebUI entries with metadata + download links for each
 */
async function loadFiles() {
  const response = await fetch("/api/files");

  if (!response.ok) {
    alert("Request failed");
    return;
  }

  allFiles = await response.json();

  renderFiles();
}

/**
 * Handles a single file upload with progress bar updates
 * Produces separate bars for each queue e.g.
 * - upload 3 files together => 1. queue
 * - add one more file to download separately => 2. queue
 */
async function uploadFile(file) {
  return new Promise((resolve, reject) => {
    const formData = new FormData();

    formData.append("file", file);

    const xhr = new XMLHttpRequest();

    xhr.open("POST", "/api/upload");

    // create upload ui item
    const queue = document.getElementById("upload-queue");

    const item = document.createElement("div");
    item.className = "upload-item";

    const nameElem = document.createElement("div");
    nameElem.className = "upload-name";
    nameElem.textContent = file.name;

    const progressBg = document.createElement("div");
    progressBg.className = "upload-progress-bg";

    const progressFill = document.createElement("div");
    progressFill.className = "upload-progress-fill";

    const actions = document.createElement("div");
    actions.className = "upload-actions";

    const cancelBtn = document.createElement("button");
    cancelBtn.className = "upload-cancel-btn";
    cancelBtn.textContent = "Cancel";

    actions.appendChild(cancelBtn);

    progressBg.appendChild(progressFill);

    item.appendChild(nameElem);
    item.appendChild(progressBg);
    item.appendChild(actions);

    queue.appendChild(item);

    // progress updates
    xhr.upload.addEventListener("progress", (e) => {
      if (!e.lengthComputable) return;

      const pct = (e.loaded / e.total) * 100;

      progressFill.style.width = pct + "%";
    });

    xhr.addEventListener("load", () => {
      progressFill.style.width = "100%";

      setTimeout(() => {
        item.remove();
      }, 1000);

      resolve();
    });

    xhr.addEventListener("error", () => {
      item.remove();

      alert("Upload failed: " + file.name);

      reject();
    });

    // upload cancel
    cancelBtn.addEventListener("click", () => {
      xhr.abort();

      item.remove();
    });

    xhr.addEventListener("abort", () => {
      console.log("Upload cancelled:", file.name);
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
    loadFiles(); // this could get expensive with lots of files, but it can stay for now
  }
}

/**
 * Execute a server-side script
 * Server can run each script as a separate job via threading
 */
async function runScript(name, payload = null) {
  const outputElem = document.getElementById("script-output");
  const terminal = document.getElementById("terminal-container");

  // create job element
  const jobElem = document.createElement("div");
  jobElem.className = "job-output";

  // separate header and body
  const headerElem = document.createElement("div");
  headerElem.className = "job-header";

  const bodyElem = document.createElement("pre");
  bodyElem.className = "job-body";

  jobElem.appendChild(headerElem);
  jobElem.appendChild(bodyElem);

  outputElem.appendChild(jobElem);

  const startedAt = getTimestamp();

  headerElem.textContent = `[${startedAt}] $ ${name}\n` + `[ STARTING ]`;

  terminal.scrollTop = terminal.scrollHeight;

  if (!payload) payload = {};

  payload.script = name;

  const options = {
    method: "POST",
    body: JSON.stringify(payload),
  };

  try {
    const response = await fetch("/api/scripts/run", options);
    const job = await response.json();

    let finished = false;
    let offset = 0;

    while (!finished) {
      const statusRes = await fetch(`/api/jobs/status/${job.job_id}`);
      const status = await statusRes.json();

      const outputRes = await fetch(
        `/api/jobs/output/${job.job_id}?offset=${offset}`,
      );
      const output = await outputRes.json();

      if (output.data.length > 0) {
        bodyElem.textContent += output.data;

        offset = output.next_offset;

        terminal.scrollTop = terminal.scrollHeight;
      }

      headerElem.textContent =
        `[${startedAt}] $ ${name}\n` +
        `[ JOB ${job.job_id} | ${status.status.toUpperCase()} ]`;

      if (status.status === "completed" || status.status === "failed") {
        const label =
          status.status === "completed" ? "[ SUCCESS ]" : "[ FAILED ]";

        bodyElem.textContent += `\n${label}\n`;
        bodyElem.textContent += "\n--------------------------------\n";

        finished = true;
      }

      await new Promise((r) => setTimeout(r, 300));
    }
  } catch (err) {
    bodyElem.textContent += `\n[ FAILED ] ${err}\n`;
  }

  terminal.scrollTop = terminal.scrollHeight;

  loadFiles();
}

/**
 * Create script args interface
 */
function renderScriptForm(script) {
  const container = document.getElementById("script-form-container");

  // compare with current title: if same, flush contents + stored dataset. Otherwise flush just html
  // with this, clicking at same script button closes the form and clicking another one just overrides its contents
  if (container.dataset.title === script.name) {
    container.innerHTML = "";
    container.style = "";
    container.dataset.title = "";
    return;
  }

  container.innerHTML = "";
  container.style =
    "border: 1px solid black; box-sizing: border-box; padding: 5px; margin-top: 10px";

  const title = document.createElement("h3");
  title.textContent = script.name;
  title.style = "color: darkblue";

  container.appendChild(title);
  container.dataset.title = script.name; // save current title

  const scriptDescription = document.createElement("h4");
  scriptDescription.textContent = script.description;
  scriptDescription.style = "padding-bottom: 10px; color: greenyellow";

  container.appendChild(scriptDescription);

  const form = document.createElement("div");

  const inputs = {};

  for (const field of script.fields) {
    const wrapper = document.createElement("div");
    wrapper.style =
      "display: grid; grid-template-columns: 150px 150px; align-items: center";

    const label = document.createElement("label");
    label.textContent = field.name;
    label.style = "padding-right: 30px; font-weight: bold";
    wrapper.appendChild(label);

    let input;

    if (field.type === "select") {
      input = document.createElement("select");

      for (const option of field.options || []) {
        const opt = document.createElement("option");

        opt.value = option;
        opt.textContent = option;

        input.appendChild(opt);
      }
    } else {
      input = document.createElement("input");
      input.type = "text";
    }

    input.style = "border-radius: 5px";

    inputs[field.name] = input;

    wrapper.appendChild(input);

    if (field.description) {
      const desc = document.createElement("small");
      desc.textContent = field.description;
      desc.style = "padding-bottom: 10px";
      wrapper.appendChild(desc);
    }

    form.appendChild(wrapper);
  }

  const runBtn = document.createElement("button");
  runBtn.textContent = "Run Script";
  runBtn.style = "margin-top: 15px";

  runBtn.onclick = () => {
    const payload = {};

    for (const key in inputs) {
      payload[key] = inputs[key].value;
    }

    // input field validation
    for (const field of script.fields) {
      const value = inputs[field.name].value;

      if (field.required && !value.trim()) {
        alert(`${field.name} is required`);
        return;
      }
    }

    runScript(script.name, payload);
  };

  form.appendChild(runBtn);

  container.appendChild(form);
}

/**
 * Load all server-side scripts and creates buttons which open a form for input args for each
 */
async function loadScripts() {
  const res = await fetch("/api/scripts");
  const scripts = await res.json();
  const container = document.getElementById("script-container");

  container.innerHTML = "";

  for (const script of scripts) {
    const btn = document.createElement("button");

    btn.className = "script-button";
    btn.textContent = script.name;

    btn.onclick = () => renderScriptForm(script);

    container.appendChild(btn);
  }
}

window.runScript = runScript;

loadFiles();
loadScripts();
