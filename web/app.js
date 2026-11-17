let locale = localStorage.getItem("sec_lang") === "en" ? "en" : "vi";
const localeButton = document.getElementById("locale");
const status = document.getElementById("install-status");
const log = document.getElementById("log");
const connect = document.getElementById("connect");
const disconnect = document.getElementById("disconnect");
let port = null, reader = null, reading = false;
function render() {
  document.documentElement.lang = locale;
  localeButton.textContent = locale === "vi" ? "EN" : "VI";
  document.querySelectorAll("[data-vi][data-en]").forEach(el => { el.textContent = el.dataset[locale]; });
  if (document.getElementById("installer").hidden) {
    status.textContent = locale === "vi" ? "Chưa có firmware Stable được kiểm chứng để nạp." : "No verified Stable firmware is available for installation.";
  }
}
localeButton.addEventListener("click", () => {
  locale = locale === "vi" ? "en" : "vi"; localStorage.setItem("sec_lang", locale); render();
});
// Require an explicit manifest in a deployed release. Never blindly flash a CI artifact.
(async () => {
  try {
    const r = await fetch("./manifest.json", {cache:"no-store"});
    if (!r.ok) return;
    const m = await r.json();
    if (m?.chipFamily !== "ESP32-S3" || !Array.isArray(m?.builds) || !m.builds.length) return;
    // Manifest publishing is a release-approval step, separate from CI compilation.
    document.getElementById("installer").hidden = false;
    status.textContent = locale === "vi" ? "Firmware được phát hành. Kiểm tra model trước khi nạp." : "Firmware published. Verify the hardware model before flashing.";
  } catch (_) { /* no release yet */ }
})();
connect.addEventListener("click", async () => {
  if (!("serial" in navigator)) { log.value += "Web Serial unavailable. Use desktop Chrome or Edge.\n"; return; }
  try {
    port = await navigator.serial.requestPort();
    await port.open({baudRate:115200});
    connect.disabled = true; disconnect.disabled = false; reading = true;
    while (port?.readable && reading) {
      reader = port.readable.getReader();
      try { while(reading) {
        const {value,done} = await reader.read();
        if(done) break;
        log.value += new TextDecoder().decode(value);
        if(log.value.length > 50000) log.value = log.value.slice(-40000);
        log.scrollTop = log.scrollHeight;
      }} finally { reader.releaseLock(); reader = null; }
    }
  } catch (e) { log.value += "Serial: " + String(e) + "\n"; }
  finally {
    reading = false; connect.disabled = false; disconnect.disabled = true;
    if (port) { try {await port.close();}catch(_){} port = null; }
  }
});
disconnect.addEventListener("click", async () => {
  reading = false;
  if (reader) { try {await reader.cancel();}catch(_){} }
});
render();
