const elements = {
    uptime: document.getElementById("uptime"),
    temp: document.getElementById("temp"),
    relay: document.getElementById("relay"),
    lastUpdate: document.getElementById("last-update"),
};

let currentRelayState = null;
let isEditing = false; // Flag: Nutzer tippt gerade ins Formular
const SIMULATE = false;

// Hilfsfunktionen
function formatUptime(ms) {
    const totalSeconds = Math.floor(ms / 1000);
    const hours = Math.floor(totalSeconds / 3600);
    const minutes = Math.floor((totalSeconds % 3600) / 60);
    const seconds = totalSeconds % 60;
    return `${hours}h ${minutes}m ${seconds}s`;
}

function showError(message) {
    elements.uptime.innerHTML = `<span class="error">Fehler beim Laden der Uptime</span>`;
    elements.temp.innerHTML = `<span class="error">Fehler beim Laden der Temperatur</span>`;
    elements.relay.innerHTML = `<span class="error">Fehler beim Laden des Relais-Zustands</span>`;
    console.error("Fetch Error:", message);
}

// UI aktualisieren
function updateUI(data) {
    const formattedUptime = formatUptime(data.uptime);
    elements.uptime.textContent = `Uptime: ${formattedUptime}`;

    const tempValue = parseFloat(data.temp);
    const tempClass = tempValue >= parseFloat(data.switch_temp) ? "temp-normal" : "temp-low" ;
    elements.temp.innerHTML = `Aktuelle Temperatur: <span class="${tempClass}">${tempValue} °C</span>`;

    currentRelayState = data.relay;
    const relaySymbol = data.relay
        ? '<span style="color: green;">🟢 EIN</span>'
        : '<span style="color: red;">🔴 AUS</span>';
    elements.relay.innerHTML = `Relais: ${relaySymbol}`;

    elements.lastUpdate.textContent = `Letztes Update: ${new Date().toLocaleTimeString()}`;

    // Formularwerte nur setzen, wenn Nutzer nicht gerade editiert
    if (!isEditing) {
        if (data.start_time) {
            document.getElementById("start-time").value = data.start_time;
        }
        if (data.end_time) {
            document.getElementById("end-time").value = data.end_time;
        }
        if (data.switch_temp !== null && data.switch_temp !== undefined) {
            document.getElementById("switch-temp").value = data.switch_temp;
        }
    }
}

// Daten vom Server holen
async function fetchData() {
    try {
        let data;
        if (SIMULATE) {
            data = {
                uptime: Date.now() - performance.timing.navigationStart,
                temp: (20 + Math.random() * 10).toFixed(2),
                relay: Math.random() > 0.5,
                start_time: "08:00",
                end_time: "20:00",
                switch_temp: 25.5
            };
        } else {
            const response = await fetch("/data");
            if (!response.ok) throw new Error(`Status: ${response.status}`);
            data = await response.json();
        }

        updateUI(data);
    } catch (error) {
        showError(error.message);
    }
}

// Daten an Server schicken
async function updateServer(updateData) {
    const response = await fetch("/update", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify(updateData)
    });

    if (!response.ok) throw new Error(`Status: ${response.status}`);
    const data = await response.json();
    updateUI(data);
}

// Relais toggeln
function toggleRelay() {
    const newRelayState = !currentRelayState;
    updateServer({ relay: newRelayState });
}

elements.relay.addEventListener("click", toggleRelay);

// === Formular ===
const configForm = document.getElementById("config-form");
const configStatus = document.getElementById("config-status");
const configButton = configForm.querySelector("button");

// User-Eingaben erkennen → isEditing setzen
["start-time", "end-time", "switch-temp"].forEach(id => {
    const el = document.getElementById(id);
    el.addEventListener("input", () => { isEditing = true; });
    el.addEventListener("blur", () => { isEditing = false; });
});

configForm.addEventListener("submit", async (e) => {
    e.preventDefault();

    const startTime = document.getElementById("start-time").value;
    const endTime = document.getElementById("end-time").value;
    const switchTemp = parseFloat(document.getElementById("switch-temp").value);

    // einfache Validierung
    if (!startTime || !endTime || isNaN(switchTemp)) {
        configStatus.textContent = "❌ Bitte gültige Werte eingeben.";
        configStatus.style.color = "red";
        return;
    }

    configButton.disabled = true;
    configButton.textContent = "Speichern...";

    try {
        await updateServer({
            start_time: startTime,
            end_time: endTime,
            switch_temp: switchTemp
        });

        configStatus.textContent = "✅ Konfiguration gespeichert!";
        configStatus.style.color = "green";
        isEditing = false; // Eingaben sind nun übernommen
    } catch (err) {
        configStatus.textContent = "❌ Fehler beim Speichern.";
        configStatus.style.color = "red";
    } finally {
        configButton.disabled = false;
        configButton.textContent = "Speichern";
    }
});

// Initial
window.addEventListener("DOMContentLoaded", () => {
    fetchData();
    setInterval(fetchData, 2000);
});

