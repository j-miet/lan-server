/**
 * Get user's auth headers
 */
export function getAuthHeaders() {
  const token = localStorage.getItem("token");

  if (!token) {
    window.location.href = "/login.html";
    return {};
  }

  return {
    Authorization: "Bearer " + token,
  };
}

/**
 * Log user out on auth failure
 */
export function handleAuthFailure(response) {
  if (response.status === 401) {
    localStorage.removeItem("token");
    window.location.href = "/login.html";
    return true;
  }

  return false;
}
