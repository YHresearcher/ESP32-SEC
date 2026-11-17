let locale = localStorage.getItem("sec_lang") === "en" ? "en" : "vi";
const localeButton = document.getElementById("locale");
const status = document.getElementById("install-status");
const log = document.getElementById("log");
const connect = document.getElementById("connect");
const disconnect = document.getElementById("disconnect");
let port = null, reader = null, reading = false;
const consent = document.getElementById("flash-consent");
const installer = document.getElementById("installer");
const firmwareName = document.getElementById("firmware-name");
let manifestAvailable = false;
let publishedVersion = "";
function updateFlasher() {
  installer.hidden = !(manifestAvailable && consent.checked);
  firmwareName.textContent = publishedVersion ? (locale === "vi" ? "Bản build: " : "Build: ") + publishedVersion : "";
  status.textContent = !manifestAvailable
    ? (locale === "vi" ? "Chưa có firmware trên website; kiểm tra GitHub Releases." : "No installable firmware is hosted here; see GitHub Releases.")
    : !consent.checked
      ? (locale === "vi" ? "Xác nhận sao lưu và đúng board để mở nút Nạp firmware." : "Confirm backup and target hardware to unlock Install.")
      : (locale === "vi" ? "Đã mở nạp bản alpha. Chọn CONNECT và xác nhận thiết bị trong trình duyệt." : "Experimental alpha install unlocked. Select CONNECT and confirm your device.");
}
function render() {
  document.documentElement.lang = locale;
  localeButton.textContent = locale === "vi" ? "EN" : "VI";
  document.querySelectorAll("[data-vi][data-en]").forEach(el => { el.textContent = el.dataset[locale]; });
  updateFlasher();
}
localeButton.addEventListener("click", () => {
  locale = locale === "vi" ? "en" : "vi"; localStorage.setItem("sec_lang", locale); render();
});
// Firmware binaries and the chip-specific manifest are published to this same HTTPS origin.
consent.addEventListener("change", updateFlasher);
(async () => {
  try {
    const r = await fetch("./manifest.json", {cache:"no-store"});
    if (!r.ok) return;
    const m = await r.json();
    const valid = Array.isArray(m?.builds) && m.builds.some(b =>
      b.chipFamily === "ESP32-S3" &&
      Array.isArray(b.parts) && b.parts.length === 3 &&
      b.parts.every(p => typeof p.path === "string" && p.path.startsWith("firmware/") &&
        Number.isInteger(p.offset) && p.offset >= 0));
    if (!valid) throw new Error("Manifest invalid for ESP32-S3");
    manifestAvailable = true;
    publishedVersion = String(m.version || "");
  } catch (e) {
    log.value += "Firmware manifest: " + String(e) + "\\n";
  }
  updateFlasher();
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
