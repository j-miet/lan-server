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
    const response = await fetch("/auth/verify", {
      headers: { Authorization: "Bearer " + token },
    });

    if (!response.ok) {
      alert("Invalid token");
      return;
    }
  } catch {
    alert("Server unreachable");
    return;
  }

  localStorage.setItem("token", token);
  window.location.replace("/index.html");
}
