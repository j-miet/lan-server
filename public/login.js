/**
 * Verify login attempt
 */
async function login() {
  const token = document.getElementById("tokenInput").value;

  if (!token) {
    alert("Enter token");
    return;
  }

  try {
    const response = await fetch("/api/login", {
      method: "POST",
      body: token,
    });

    if (!response.ok) {
      alert("Invalid token");
      return;
    }
  } catch {
    alert("Server unreachable");
    return;
  }

  window.location.replace("/index.html");
}
