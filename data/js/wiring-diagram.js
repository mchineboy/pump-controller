import { getSettings } from "./api.js";

const WIRE = {
  vmot: { color: "#e74c3c", label: "12 V / VMOT" },
  v5: { color: "#ffb300", label: "5 V" },
  vio: { color: "#cddc39", label: "3.3 V logic (VIO)" },
  gnd: { color: "#b0bec5", label: "GND" },
  phase: { color: "#8d6e63", label: "Motor phases" },
  step: { color: "#f1c40f", label: "STEP" },
  dir: { color: "#3498db", label: "DIR" },
  enable: { color: "#2ecc71", label: "ENABLE" },
  uartTx: { color: "#9b59b6", label: "UART TX" },
  uartRx: { color: "#1abc9c", label: "UART RX" },
  valve: { color: "#e67e22", label: "Valve control" },
  estop: { color: "#e91e63", label: "ESTOP" },
  sense: { color: "#00bcd4", label: "Sensor signal" }
};

const PUMPS = [
  {
    id: "pump_1",
    name: "Pump 1",
    address: "0b00",
    step: 26,
    dir: 27,
    enable: 25,
    valve: 33,
    valveKey: "valve_hardware_present"
  },
  {
    id: "pump_2",
    name: "Pump 2",
    address: "0b01",
    step: 5,
    dir: 13,
    enable: 14,
    valve: 21,
    valveKey: "pump2_valve_hardware_present"
  },
  {
    id: "pump_3",
    name: "Pump 3",
    address: "0b10",
    step: 22,
    dir: 15,
    enable: 2,
    valve: 12,
    valveKey: "pump3_valve_hardware_present"
  }
];

function escapeXml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;");
}

function box(x, y, w, h, title, lines = []) {
  const text = [
    `<text x="${x + 10}" y="${y + 22}" class="wd-title">${escapeXml(title)}</text>`,
    ...lines.map((line, index) =>
      `<text x="${x + 10}" y="${y + 42 + index * 16}" class="wd-line">${escapeXml(line)}</text>`
    )
  ].join("");
  return `
    <rect x="${x}" y="${y}" width="${w}" height="${h}" rx="6" class="wd-box"/>
    ${text}
  `;
}

function compactRow(x, y, w, h, label) {
  return `
    <rect x="${x}" y="${y}" width="${w}" height="${h}" rx="6" class="wd-box"/>
    <text x="${x + 10}" y="${y + h / 2 + 4}" class="wd-line">${escapeXml(label)}</text>
  `;
}

function elbow(x1, y1, x2, y2, kind, label) {
  const { color } = WIRE[kind];
  const midX = (x1 + x2) / 2;
  const labelY = Math.min(y1, y2) - 6;
  return `
    <path d="M ${x1} ${y1} H ${midX} V ${y2} H ${x2}"
      fill="none" stroke="${color}" stroke-width="2.5" stroke-linecap="round"/>
    <circle cx="${x1}" cy="${y1}" r="3" fill="${color}"/>
    <circle cx="${x2}" cy="${y2}" r="3" fill="${color}"/>
    ${label
      ? `<text x="${midX}" y="${labelY}" text-anchor="middle" class="wd-wire-label" fill="${color}">${escapeXml(label)}</text>`
      : ""}
  `;
}

function straight(x1, x2, y, kind, label) {
  const { color } = WIRE[kind];
  const midX = (x1 + x2) / 2;
  return `
    <line x1="${x1}" y1="${y}" x2="${x2}" y2="${y}"
      stroke="${color}" stroke-width="2.5" stroke-linecap="round"/>
    <circle cx="${x1}" cy="${y}" r="3" fill="${color}"/>
    <circle cx="${x2}" cy="${y}" r="3" fill="${color}"/>
    ${label
      ? `<text x="${midX}" y="${y - 8}" text-anchor="middle" class="wd-wire-label" fill="${color}">${escapeXml(label)}</text>`
      : ""}
  `;
}

function vline(x, y1, y2, kind, label) {
  const { color } = WIRE[kind];
  return `
    <line x1="${x}" y1="${y1}" x2="${x}" y2="${y2}"
      stroke="${color}" stroke-width="2.5" stroke-linecap="round"/>
    ${label
      ? `<text x="${x + 5}" y="${(y1 + y2) / 2}" class="wd-wire-label" fill="${color}">${escapeXml(label)}</text>`
      : ""}
  `;
}

function legendItems(activeKinds) {
  return activeKinds.map((kind) => {
    const item = WIRE[kind];
    return `
      <div class="wiring-legend-item">
        <span class="wiring-swatch" style="background:${item.color}"></span>
        <span>${escapeXml(item.label)}</span>
      </div>
    `;
  }).join("");
}

export function buildWiringDiagramModel(settings = {}) {
  const pumpCount = Math.min(3, Math.max(1, Number(settings.pump_count) || 1));
  const pumps = PUMPS.slice(0, pumpCount).map((pump) => ({
    ...pump,
    valveEnabled: Boolean(settings[pump.valveKey])
  }));
  const options = {
    uart: Boolean(settings.driver_uart_enabled),
    estop: Boolean(settings.emergency_stop_enabled),
    reservoir: Boolean(settings.reservoir_sensor_enabled),
    loadCell: Boolean(settings.load_cell_enabled),
    temperature: Boolean(settings.temperature_sensor_enabled),
    flow: Boolean(settings.flow_sensor_enabled)
  };
  return { pumpCount, pumps, options };
}

export function buildWiringDiagramSvg(settings = {}) {
  const model = buildWiringDiagramModel(settings);
  const pumpBlockH = model.pumps.some((pump) => pump.valveEnabled) ? 156 : 132;
  const pumpGap = 20;
  const pumpsHeight =
    model.pumps.length * pumpBlockH +
    Math.max(0, model.pumps.length - 1) * pumpGap;

  const sensors = [];
  if (model.options.estop) {
    sensors.push({ title: "Emergency stop · active low", kind: "estop", gpio: "32" });
  }
  if (model.options.reservoir) {
    sensors.push({ title: "Empty-container sensor", kind: "sense", gpio: "34" });
  }
  if (model.options.loadCell) {
    sensors.push({ title: "HX711 load cell DT/SCK", kind: "sense", gpio: "19 / 18" });
  }
  if (model.options.temperature) {
    sensors.push({ title: "DS18B20 temperature", kind: "sense", gpio: "23" });
  }
  if (model.options.flow) {
    sensors.push({ title: "Pulse flow sensor", kind: "sense", gpio: "4" });
  }

  const sensorsHeight = sensors.length ? 36 + sensors.length * 34 : 0;
  const width = 810;
  const espX = 200;
  const espY = 96;
  const espW = 150;
  const espH = Math.max(200, pumpsHeight);
  const tmcX = 440;
  const motorX = 620;
  const height = espY + espH + sensorsHeight + 24;

  const vmotRailY = 24;
  const gndRailY = 44;
  const railLeft = 150;
  const railRight = 560;
  const vmotDropX = tmcX + 24;
  const gndDropX = tmcX + 52;
  const lastPumpTop =
    espY + (model.pumps.length - 1) * (pumpBlockH + pumpGap);

  const parts = [];

  // Power source blocks.
  parts.push(box(20, 66, 120, 52, "12 V supply", ["~3 A fused"]));
  parts.push(box(20, 150, 120, 50, "5 V buck", ["12 V → 5 V"]));

  // 12 V and GND distribution rails across the top.
  parts.push(straight(railLeft, railRight, vmotRailY, "vmot", "12 V rail"));
  parts.push(straight(railLeft, railRight, gndRailY, "gnd", "GND rail"));

  // Feed the rails from the supply and buck.
  parts.push(elbow(140, 80, railLeft, vmotRailY, "vmot", ""));
  parts.push(vline(80, 118, 150, "vmot", "12 V"));
  parts.push(elbow(140, 100, railLeft, gndRailY, "gnd", ""));
  parts.push(elbow(140, 190, railLeft, gndRailY, "gnd", ""));

  parts.push(box(espX, espY, espW, espH, "ESP32", [
    "Dev board",
    `Active pumps: ${model.pumpCount}`
  ]));

  // ESP32 power: 5 V in from buck, GND from the rail, 3.3 V logic out.
  parts.push(elbow(140, 175, espX, espY + 24, "v5", "5 V"));
  parts.push(vline(espX + 40, gndRailY, espY, "gnd", "GND"));

  model.pumps.forEach((pump, index) => {
    const y = espY + index * (pumpBlockH + pumpGap);
    const tmcH = pump.valveEnabled ? 120 : 100;
    parts.push(box(tmcX, y, 150, tmcH, `TMC2209 · ${pump.name}`, [
      `UART ${pump.address}`,
      "EN active low"
    ]));
    parts.push(box(motorX, y + 10, 150, 78, "NEMA 17", [
      pump.name,
      "Peristaltic"
    ]));

    // Driver power and ground drops from the top rails.
    parts.push(vline(vmotDropX, vmotRailY, y, "vmot", index === 0 ? "VMOT" : ""));
    parts.push(vline(gndDropX, gndRailY, y, "gnd", index === 0 ? "GND" : ""));

    // Logic signals and 3.3 V reference from the ESP32.
    parts.push(elbow(espX + espW, y + 32, tmcX, y + 32, "step", `STEP ${pump.step}`));
    parts.push(elbow(espX + espW, y + 52, tmcX, y + 52, "dir", `DIR ${pump.dir}`));
    parts.push(elbow(espX + espW, y + 72, tmcX, y + 72, "enable", `EN ${pump.enable}`));
    parts.push(elbow(
      espX + espW,
      y + 92,
      tmcX,
      y + 92,
      "vio",
      index === 0 ? "VIO 3.3 V" : ""
    ));
    parts.push(straight(tmcX + 150, motorX, y + 48, "phase", "A/B phases"));

    if (pump.valveEnabled) {
      parts.push(compactRow(motorX, y + 100, 150, 34, `Valve · GPIO ${pump.valve}`));
      parts.push(elbow(
        espX + espW,
        y + 116,
        motorX,
        y + 117,
        "valve",
        `VALVE ${pump.valve}`
      ));
    }
  });

  // Extend the GND drop down the driver column so every TMC shares it.
  if (model.pumps.length > 1) {
    parts.push(vline(gndDropX, gndRailY, lastPumpTop, "gnd", ""));
    parts.push(vline(vmotDropX, vmotRailY, lastPumpTop, "vmot", ""));
  }

  if (model.options.uart) {
    const txY = espY + espH - 40;
    const rxY = espY + espH - 22;
    parts.push(straight(espX + espW, tmcX + 140, txY, "uartTx", "TX GPIO 17"));
    parts.push(straight(espX + espW, tmcX + 140, rxY, "uartRx", "RX GPIO 16"));
    model.pumps.forEach((pump, index) => {
      const y = espY + index * (pumpBlockH + pumpGap) + 88;
      const dropX = tmcX + 95 + index * 16;
      parts.push(`
        <line x1="${dropX}" y1="${txY}" x2="${dropX}" y2="${y}"
          stroke="${WIRE.uartTx.color}" stroke-width="2" stroke-dasharray="4 3"/>
        <line x1="${dropX + 10}" y1="${rxY}" x2="${dropX + 10}" y2="${y + 8}"
          stroke="${WIRE.uartRx.color}" stroke-width="2" stroke-dasharray="4 3"/>
      `);
    });
  }

  sensors.forEach((sensor, index) => {
    const y = espY + espH + 24 + index * 34;
    parts.push(compactRow(espX, y, 240, 28, sensor.title));
    parts.push(elbow(
      espX + 240,
      y + 14,
      motorX + 20,
      y + 14,
      sensor.kind,
      `GPIO ${sensor.gpio}`
    ));
  });

  const activeKinds = [
    "vmot",
    "v5",
    "vio",
    "gnd",
    "phase",
    "step",
    "dir",
    "enable"
  ];
  if (model.pumps.some((pump) => pump.valveEnabled)) {
    activeKinds.push("valve");
  }
  if (model.options.uart) {
    activeKinds.push("uartTx", "uartRx");
  }
  if (model.options.estop) {
    activeKinds.push("estop");
  }
  if (
    model.options.reservoir ||
    model.options.loadCell ||
    model.options.temperature ||
    model.options.flow
  ) {
    activeKinds.push("sense");
  }

  return {
    svg: `
      <svg viewBox="0 0 ${width} ${height}" role="img"
        aria-label="Wiring diagram for ${model.pumpCount} active pump path${model.pumpCount === 1 ? "" : "s"}"
        class="wiring-svg">
        <style>
          .wd-box { fill: #16201a; stroke: #3a4a3e; stroke-width: 1.5; }
          .wd-title { fill: #e8efe6; font: 600 13px "Avenir Next", "Segoe UI", sans-serif; }
          .wd-line { fill: #9aab9d; font: 11px "Avenir Next", "Segoe UI", sans-serif; }
          .wd-wire-label { font: 600 10px "Avenir Next", "Segoe UI", sans-serif; }
        </style>
        ${parts.join("\n")}
      </svg>
    `,
    legendHtml: legendItems(activeKinds),
    caption:
      `${model.pumpCount} pump path${model.pumpCount === 1 ? "" : "s"}` +
      (model.options.uart ? " · UART enabled" : " · STEP/DIR only") +
      (model.pumps.some((pump) => pump.valveEnabled)
        ? " · valve hardware shown"
        : "")
  };
}

function ensureDialog() {
  let dialog = document.getElementById("wiring-diagram-dialog");
  if (dialog) {
    return dialog;
  }

  dialog = document.createElement("dialog");
  dialog.id = "wiring-diagram-dialog";
  dialog.className = "wiring-dialog";
  dialog.innerHTML = `
    <div class="wiring-dialog-header">
      <div>
        <h2>Wiring diagram</h2>
        <p class="meta wiring-caption"></p>
      </div>
      <button type="button" class="wiring-close" aria-label="Close wiring diagram">Close</button>
    </div>
    <div class="wiring-legend"></div>
    <div class="wiring-svg-host"></div>
    <p class="meta wiring-note">
      Wire colors are for on-screen clarity. Confirm polarity and connectors in
      the project wiring docs before powering the system.
    </p>
  `;
  document.body.appendChild(dialog);

  const close = () => dialog.close();
  dialog.querySelector(".wiring-close").addEventListener("click", close);
  dialog.addEventListener("click", (event) => {
    if (event.target === dialog) {
      close();
    }
  });
  return dialog;
}

export function showWiringDiagram(settings) {
  const dialog = ensureDialog();
  const diagram = buildWiringDiagramSvg(settings);
  dialog.querySelector(".wiring-caption").textContent = diagram.caption;
  dialog.querySelector(".wiring-legend").innerHTML = diagram.legendHtml;
  dialog.querySelector(".wiring-svg-host").innerHTML = diagram.svg;
  if (typeof dialog.showModal === "function") {
    dialog.showModal();
  } else {
    dialog.setAttribute("open", "");
  }
}

export function bindWiringDiagramButton(button, getSettingsFn = getSettings) {
  if (!button) {
    return;
  }
  button.addEventListener("click", async () => {
    button.disabled = true;
    try {
      const settings = await getSettingsFn();
      showWiringDiagram(settings);
    } catch (error) {
      alert(error.message);
    } finally {
      button.disabled = false;
    }
  });
}
