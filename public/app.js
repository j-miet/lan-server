"use strict";

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
 * Get current date + timestamp as a string "YYYY-MM-DD HH.MM.SS"
 */
function getTimestamp() {
  const now = new Date();
  const date = now.toISOString().slice(0, 10);
  const time = now.toLocaleTimeString();
  return `${date} ${time}`;
}

/**
 * Download a file from server
 */
async function downloadFile(file) {
  const a = document.createElement("a");
  a.href = `/download/${encodeURIComponent(file)}`;
  a.download = file;
  a.click();
}

/**
 * Delete a uploaded server file
 */
async function deleteFile(file) {
  const response = await fetch(`/api/files/${file}`, { method: "DELETE" });

  loadFiles();
}

/**
 * Get all uploaded server files and create WebUI entries with download links for each
 */
async function loadFiles() {
  const response = await fetch("/api/files");

  if (!response.ok) {
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

    uploadFileName.innerHTML = file.name;

    xhr.upload.addEventListener("progress", (e) => {
      if (!e.lengthComputable) return;

      const pct = (e.loaded / e.total) * 100;

      progressBar.style.width = pct + "%";
    });

    xhr.addEventListener("load", () => {
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
  outputElem.appendChild(jobElem);

  const startedAt = getTimestamp();
  jobElem.textContent = `[${startedAt}] $ ${name}\n`;

  terminal.scrollTop = terminal.scrollHeight;

  const options = { method: "POST" };

  if (!payload) payload = {};

  payload.script = name;
  options.body = JSON.stringify(payload);

  try {
    const response = await fetch(`/api/scripts/run`, options);
    const job = await response.json();

    jobElem.textContent += `[ JOB ${job.job_id} STARTED ]\n`;
    terminal.scrollTop = terminal.scrollHeight;

    // poll script updates via job id
    while (true) {
      const res = await fetch(`/api/jobs/${job.job_id}`);
      const data = await res.json();

      jobElem.textContent =
        `[${startedAt}] $ ${name}\n` +
        `[ JOB ${job.job_id} ]\n\n` +
        data.output;

      terminal.scrollTop = terminal.scrollHeight;

      if (data.status === "completed" || data.status === "failed") {
        const status =
          data.status === "completed" ? "[ SUCCESS ]" : "[ FAILED ]";

        jobElem.textContent += `\n${status}\n`;
        jobElem.textContent += "\n--------------------------------\n";

        break;
      }

      await new Promise((r) => setTimeout(r, 500)); // polling delay
    }
  } catch (err) {
    jobElem.textContent += `\n[ FAILED ] ${err}\n`;
  }

  terminal.scrollTop = terminal.scrollHeight;
  loadFiles(); // refresh server files
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

    // required field validation
    for (const field of script.fields) {
      const value = inputs[field.name].value;

      if (field.required && !value.trim()) {
        alert(`${field.name} is required`);
        return;
      }
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

    btn.textContent = script.name;

    btn.onclick = () => renderScriptForm(script);

    container.appendChild(btn);
  }
}

window.runScript = runScript;

loadFiles();
loadScripts();
