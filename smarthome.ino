// using esp-32
// 12 relay input low

#include <WiFi.h>
#include <WebServer.h>
#include <NTPClient.h>
#include <WiFiUDP.h>

const char* ssid = "Stb"; // nama wifi atau access point
const char* password = "11111111"; // passwrd wifi

// Definisi Pin Lampu
#define LAMP1_PIN  13  // Lampu Gerbang
#define LAMP2_PIN  12  // Lampu Teras 1
#define LAMP3_PIN  14  // Lampu Teras 2
#define LAMP4_PIN  27  // Ruang Tengah 1
#define LAMP5_PIN  26  // Ruang Tengah 2
#define LAMP6_PIN  25  // Ruang Kiri 1
#define LAMP7_PIN  15  // Ruang Kiri 2
#define LAMP8_PIN  2   // Ruang Kanan 1
#define LAMP9_PIN  4   // Ruang Kanan 2
#define LAMP10_PIN 16  // Lampu Dapur 1
#define LAMP11_PIN 17  // Lampu Dapur 2
#define LAMP12_PIN 5   // Lampu Dapur 3

WebServer server(80);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 28800, 60000);

struct Lamp {
    int pin;
    int onHour = -1, onMinute = -1, offHour = -1, offMinute = -1;
    bool state = false; // false = MATI, true = NYALA
} lamps[12] = {
    {LAMP1_PIN}, {LAMP2_PIN}, {LAMP3_PIN}, {LAMP4_PIN},
    {LAMP5_PIN}, {LAMP6_PIN}, {LAMP7_PIN}, {LAMP8_PIN},
    {LAMP9_PIN}, {LAMP10_PIN}, {LAMP11_PIN}, {LAMP12_PIN}
};

const char* lampNames[] = {
    "Lampu Gerbang", "Lampu Teras 1", "Lampu Teras 2", "Ruang Tengah 1",
    "Ruang Tengah 2", "Ruang Kiri 1", "Ruang Kiri 2", "Ruang Kanan 1",
    "Ruang Kanan 2", "Lampu Dapur 1", "Lampu Dapur 2", "Lampu Dapur 3"
};

const char* htmlPage = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Kontrol Lampu</title>
    <style>
        body { font-family: Arial, sans-serif; text-align: center; background: #f4f4f4; margin: 20px; }
        .container { background: white; padding: 15px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); max-width: 400px; margin: auto; margin-bottom: 20px; }
        .lamp-section { margin-bottom: 20px; }
        .header { display: flex; justify-content: space-between; font-size: 18px; font-weight: bold; }
        .button-group { display: flex; justify-content: space-between; margin: 10px 0; }
        button { flex: 1; padding: 10px; border: none; font-size: 14px; cursor: pointer; border-radius: 5px; margin: 5px; }
        .on { background: #28a745; color: white; }
        .off { background: #dc3545; color: white; }
        .set-time { background: #007bff; color: white; width: 100%; }
        input { padding: 8px; font-size: 14px; border-radius: 5px; border: 1px solid #ccc; text-align: center; }
        .time-box { display: flex; justify-content: space-between; align-items: center; margin: 10px 0; }
    </style>
</head>
<body>
    <div class="container">
        <h2>Waktu Sekarang</h2>
        <h3 id="currentTime">%TIME%</h3>
    </div>
    
    %LAMP_SECTIONS%
    
    <script>
        function updateTimeDisplay() {
            fetch('/get-time').then(response => response.text()).then(data => {
                document.getElementById('currentTime').innerText = data;
            });
            for (let i = 0; i < 12; i++) {
                fetch(`/lamp-status?lamp=${i}`).then(response => response.text()).then(data => {
                    document.getElementById(`lampStatus${i}`).innerText = data;
                });
            }
        }
        setInterval(updateTimeDisplay, 5000);

        function setTime(lampId) {
            let onTime = document.getElementById(`onTime${lampId}`).value;
            let offTime = document.getElementById(`offTime${lampId}`).value;
            fetch(`/set-time?lamp=${lampId}&onTime=${onTime}&offTime=${offTime}`)
                .then(response => response.text())
                .then(data => alert(data));
        }
    </script>
</body>
</html>
)rawliteral";

void handleRoot() {
    String page = htmlPage;
    page.replace("%TIME%", timeClient.getFormattedTime());
    String lampSections = "";
    for (int i = 0; i < 12; i++) {
        lampSections += "<div class='container lamp-section'>";
        lampSections += "<div class='header'><span>" + String(lampNames[i]) + "</span>";
        lampSections += "<span>Status: <span id='lampStatus" + String(i) + "'>%STATUS%" + String(i) + "</span></span></div>";
        lampSections += "<div class='button-group'>";
        lampSections += "<button class='on' onclick=\"fetch('/on?lamp=" + String(i) + "')\">HIDUPKAN</button>";
        lampSections += "<button class='off' onclick=\"fetch('/off?lamp=" + String(i) + "')\">MATIKAN</button></div>";
        lampSections += "<div class='time-box'>Jadwal Nyala <input type='time' id='onTime" + String(i) + "'>";
        lampSections += "Jadwal Mati <input type='time' id='offTime" + String(i) + "'></div>";
        lampSections += "<button class='set-time' onclick='setTime(" + String(i) + ")'>Simpan Jadwal</button></div>";
    }
    page.replace("%LAMP_SECTIONS%", lampSections);
    server.send(200, "text/html", page);
}

void setup() {
    Serial.begin(115200);
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) { delay(1000); }
    timeClient.begin();

    // Inisialisasi pin relay ke HIGH (non-aktif)
    for (int i = 0; i < 12; i++) {
        pinMode(lamps[i].pin, OUTPUT);
        digitalWrite(lamps[i].pin, HIGH); // Relay aktif LOW, jadi non-aktif HIGH
        lamps[i].state = false; // Lampu mati
    }

    server.on("/", handleRoot);
    server.on("/on", []() {
        int lamp = server.arg("lamp").toInt();
        digitalWrite(lamps[lamp].pin, LOW); // Relay aktif LOW
        lamps[lamp].state = true;
        server.send(200, "text/plain", "Lampu Menyala");
    });
    server.on("/off", []() {
        int lamp = server.arg("lamp").toInt();
        digitalWrite(lamps[lamp].pin, HIGH); // Relay non-aktif HIGH
        lamps[lamp].state = false;
        server.send(200, "text/plain", "Lampu Mati");
    });
    server.on("/get-time", []() { server.send(200, "text/plain", timeClient.getFormattedTime()); });
    server.on("/lamp-status", []() {
        int lamp = server.arg("lamp").toInt();
        server.send(200, "text/plain", lamps[lamp].state ? "NYALA" : "MATI");
    });
    server.on("/set-time", []() {
        int lamp = server.arg("lamp").toInt();
        String onTime = server.arg("onTime");
        String offTime = server.arg("offTime");
        lamps[lamp].onHour = onTime.substring(0, 2).toInt();
        lamps[lamp].onMinute = onTime.substring(3, 5).toInt();
        lamps[lamp].offHour = offTime.substring(0, 2).toInt();
        lamps[lamp].offMinute = offTime.substring(3, 5).toInt();
        server.send(200, "text/plain", "Jadwal Disimpan");
    });
    server.begin();
}

void checkAndUpdateLamps() {
    int currentHour = timeClient.getHours();
    int currentMinute = timeClient.getMinutes();
    for (int i = 0; i < 12; i++) {
        if (lamps[i].onHour != -1 && lamps[i].onMinute != -1 &&
            currentHour == lamps[i].onHour && currentMinute == lamps[i].onMinute) {
            digitalWrite(lamps[i].pin, LOW); // Relay aktif LOW
            lamps[i].state = true;
        }
        if (lamps[i].offHour != -1 && lamps[i].offMinute != -1 &&
            currentHour == lamps[i].offHour && currentMinute == lamps[i].offMinute) {
            digitalWrite(lamps[i].pin, HIGH); // Relay non-aktif HIGH
            lamps[i].state = false;
        }
    }
}

void loop() {
    server.handleClient();
    timeClient.update();
    checkAndUpdateLamps();
}