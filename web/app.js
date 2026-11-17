const localeButton = document.getElementById("locale");
const version = document.getElementById("version");
const installer = document.getElementById("installer");
const status = document.getElementById("status");
let language = localStorage.getItem("esp32_sec_lang") === "en" ? "en" : "vi";
let firmwareVersion = "";
let available = false;

const strings = {
  vi: { build: "Bản build", missing: "Chưa có bản firmware để nạp.", ready: "Kết nối thiết bị ESP32-S3 qua USB và nhấn nút nạp." },
  en: { build: "Build", missing: "No firmware available to flash.", ready: "Connect your ESP32-S3 over USB and press Flash." }
};

function render() {
  document.documentElement.lang = language;
  localeButton.textContent = language === "vi" ? "EN" : "VI";
  document.querySelectorAll("[data-vi][data-en]").forEach(node => {
    node.textContent = node.dataset[language];
  });
  version.textContent = firmwareVersion ? strings[language].build + ": " + firmwareVersion : "ESP32-SEC · Alpha";
  status.textContent = available ? strings[language].ready : strings[language].missing;
  installer.hidden = !available;
}

localeButton.addEventListener("click", () => {
  language = language === "vi" ? "en" : "vi";
  localStorage.setItem("esp32_sec_lang", language);
  render();
});

async function loadManifest() {
  try {
    const response = await fetch("./manifest.json", {cache: "no-store"});
    if (!response.ok) throw new Error("Manifest missing");
    const manifest = await response.json();
    available = Array.isArray(manifest.builds) && manifest.builds.some(build =>
      build.chipFamily === "ESP32-S3" &&
      Array.isArray(build.parts) && build.parts.length === 1 &&
      build.parts.every(part => typeof part.path === "string" &&
        part.path.startsWith("firmware/") && Number.isInteger(part.offset) && part.offset === 0 && part.path.endsWith("_merged.bin"))
    );
    if (available) firmwareVersion = String(manifest.version || "");
  } catch {
    available = false;
  }
  render();
}

render();
loadManifest();
