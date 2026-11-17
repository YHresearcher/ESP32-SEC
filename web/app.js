const localeButton = document.getElementById("locale");
const version = document.getElementById("version");
const firmwareFile = document.getElementById("firmware-file");
const buildTime = document.getElementById("build-time");
const firmwareDownload = document.getElementById("firmware-download");
const installer = document.getElementById("installer");
const status = document.getElementById("status");

let language = localStorage.getItem("esp32_sec_lang") === "en" ? "en" : "vi";
let firmwareVersion = "";
let selectedFirmware = "";
let available = false;

const strings = {
  vi: {
    missing: "Không tải được firmware. Thử tải lại trang.",
    ready: "Nút Nạp sử dụng chính xác bản firmware hiển thị phía trên.",
    unsupported: "Dùng Chrome hoặc Edge trên máy tính để nạp qua USB.",
    notAllowed: "Cần HTTPS để truy cập USB.",
    connect: "Kết nối & Nạp firmware"
  },
  en: {
    missing: "Firmware unavailable. Reload the page.",
    ready: "The Flash button will install exactly the firmware shown above.",
    unsupported: "Use desktop Chrome or Edge to flash over USB.",
    notAllowed: "HTTPS is required for USB access.",
    connect: "Connect & Flash firmware"
  }
};

function render() {
  document.documentElement.lang = language;
  localeButton.textContent = language === "vi" ? "EN" : "VI";
  document.querySelectorAll("[data-vi][data-en]").forEach(node => {
    node.textContent = node.dataset[language];
  });
  const match = firmwareVersion.match(/_v([^_]+)_(\\d{2}-\\d{2}-\\d{4})_(\\d{2}-\\d{2})$/);
  version.textContent = match ? "ESP32-SEC v" + match[1] : (firmwareVersion || "ESP32-SEC · Alpha");
  buildTime.textContent = match ?
    (language === "vi" ? "Ngày build: " : "Built: ") + match[2] + " " + match[3].replace("-", ":") + " (VN)" : "";
  firmwareFile.textContent = selectedFirmware || (language === "vi" ? "Chưa chọn firmware" : "No firmware selected");
  firmwareFile.title = selectedFirmware;
  firmwareDownload.hidden = !available || !selectedFirmware;
  status.textContent = available ? strings[language].ready : strings[language].missing;
  installer.hidden = !available;
}

localeButton.addEventListener("click", () => {
  language = language === "vi" ? "en" : "vi";
  localStorage.setItem("esp32_sec_lang", language);
  render();
});

// ESP Web Tools must not be initialized before the firmware version is known.
// The HTML has no static install-button: we create exactly one with a unique,
// immutable per-build manifest, so its internal fetch cannot use a stale pointer.
async function loadFirmware() {
  try {
    const latestResponse = await fetch("./manifest.json?nocache=" + Date.now(), {cache: "no-store"});
    if (!latestResponse.ok) throw new Error("Latest manifest HTTP " + latestResponse.status);
    const latest = await latestResponse.json();
    const label = String(latest.version || "");
    if (!/^ESP32-SEC_v[0-9A-Za-z.-]+_\\d{2}-\\d{2}-\\d{4}_\\d{2}-\\d{2}$/.test(label)) {
      throw new Error("Unexpected firmware version");
    }
    const manifestPath = "./manifest-" + label + ".json";
    const manifestResponse = await fetch(manifestPath + "?check=" + Date.now(), {cache:"no-store"});
    if (!manifestResponse.ok) throw new Error("Version-specific manifest not deployed yet");
    const fixed = await manifestResponse.json();
    if (fixed.version !== label) throw new Error("Firmware manifests differ");
    const build = (fixed.builds || []).find(item =>
      item.chipFamily === "ESP32-S3" && Array.isArray(item.parts) &&
      item.parts.length === 1 && item.parts[0].offset === 0 &&
      typeof item.parts[0].path === "string" &&
      /^firmware\\/ESP32-SEC_v[0-9A-Za-z.-]+_\\d{2}-\\d{2}-\\d{4}_\\d{2}-\\d{2}_merged\\.bin$/.test(item.parts[0].path)
    );
    if (!build || build.parts[0].path !== "firmware/" + label + "_merged.bin") {
      throw new Error("Firmware binary does not match version");
    }
    const imageUrl = new URL(build.parts[0].path, location.href);
    const head = await fetch(imageUrl.href, {method: "HEAD", cache: "no-store"});
    if (!head.ok) throw new Error("Firmware binary missing (HTTP " + head.status + ")");

    firmwareVersion = label;
    selectedFirmware = build.parts[0].path.split("/").pop();
    firmwareDownload.href = imageUrl.href;

    // Mount component only after all checks. It fetches the immutable manifest.
    const button = document.createElement("esp-web-install-button");
    button.setAttribute("manifest", manifestPath);
    const activate = document.createElement("button");
    activate.slot = "activate";
    activate.type = "button";
    activate.textContent = strings[language].connect;
    activate.dataset.vi = strings.vi.connect;
    activate.dataset.en = strings.en.connect;
    activate.style.cssText = "width:100%;background:#3ed6b7;border:0;border-radius:11px;padding:14px;color:#07251d;font-weight:750;cursor:pointer";
    button.append(activate);
    for (const [slot, langKey] of [["unsupported","unsupported"],["not-allowed","notAllowed"]]) {
      const span = document.createElement("span");
      span.slot = slot;
      span.dataset.vi = strings.vi[langKey];
      span.dataset.en = strings.en[langKey];
      span.textContent = strings[language][langKey];
      button.append(span);
    }
    installer.replaceChildren(button);
    available = true;
  } catch (error) {
    console.error("Firmware manifest synchronization failed:", error);
    available = false;
    installer.replaceChildren();
  }
  render();
}

render();
loadFirmware();
