#include <Arduino.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WiFi.h>

#if __has_include("secrets.h")
#include "secrets.h"
#else
#error "Copy include/secrets.example.h to include/secrets.h and fill in your Wi-Fi and web login."
#endif

#include "pelco_d.h"
#include "web_ui.h"

#ifndef PTZ_TX_PIN
#define PTZ_TX_PIN 17
#endif
#ifndef PTZ_RX_PIN
#define PTZ_RX_PIN 16
#endif
#ifndef PTZ_DE_PIN
#define PTZ_DE_PIN 4
#endif
#ifndef PTZ_BAUD
#define PTZ_BAUD 2400
#endif
#ifndef PTZ_ADDRESS
#define PTZ_ADDRESS 1
#endif

namespace {

constexpr bool sameString(const char* a, const char* b) {
    return *a == *b && (*a == '\0' || sameString(a + 1, b + 1));
}

constexpr size_t stringLength(const char* s) {
    return *s == '\0' ? 0 : 1 + stringLength(s + 1);
}

static_assert(!sameString(WEB_PASSWORD, "change-me"),
              "Set WEB_PASSWORD in include/secrets.h; the default is not allowed.");
static_assert(stringLength(WEB_PASSWORD) >= 8, "WEB_PASSWORD must be at least 8 characters.");

// Stop the camera if the browser stops sending keep-alives (lost Wi-Fi, closed tab...).
constexpr uint32_t kMoveTimeoutMs = 800;

HardwareSerial ptzSerial(1);
WebServer server(80);

bool moving = false;
uint32_t lastMoveMs = 0;

void sendFrame(const uint8_t frame[pelco::kFrameSize]) {
    if (PTZ_DE_PIN >= 0) digitalWrite(PTZ_DE_PIN, HIGH);
    ptzSerial.write(frame, pelco::kFrameSize);
    ptzSerial.flush();  // wait until the last byte has left the UART
    if (PTZ_DE_PIN >= 0) digitalWrite(PTZ_DE_PIN, LOW);

    Serial.print("TX:");
    for (size_t i = 0; i < pelco::kFrameSize; ++i) Serial.printf(" %02X", frame[i]);
    Serial.println();
}

void sendStop() {
    uint8_t frame[pelco::kFrameSize];
    pelco::buildStop(PTZ_ADDRESS, frame);
    sendFrame(frame);
    moving = false;
}

bool authorised() {
    if (server.authenticate(WEB_USER, WEB_PASSWORD)) return true;
    server.requestAuthentication(BASIC_AUTH, "PTZ");
    return false;
}

bool requirePost() {
    if (server.method() == HTTP_POST) return true;
    server.send(405, "text/plain", "Method Not Allowed");
    return false;
}

int8_t axisArg(const char* name) {
    const long v = server.arg(name).toInt();
    return v > 0 ? 1 : (v < 0 ? -1 : 0);
}

void handleIndex() {
    if (!authorised()) return;
    server.send_P(200, "text/html", kIndexHtml);
}

void handleMove() {
    if (!authorised() || !requirePost()) return;

    pelco::Motion m;
    m.pan = axisArg("pan");
    m.tilt = axisArg("tilt");
    m.zoom = axisArg("zoom");
    m.focus = axisArg("focus");
    m.iris = axisArg("iris");
    const long speed = constrain(server.arg("speed").toInt(), 1L, static_cast<long>(pelco::kMaxSpeed));
    m.panSpeed = m.tiltSpeed = static_cast<uint8_t>(speed);

    if (!m.pan && !m.tilt && !m.zoom && !m.focus && !m.iris) {
        sendStop();
    } else {
        uint8_t frame[pelco::kFrameSize];
        pelco::buildMotion(PTZ_ADDRESS, m, frame);
        sendFrame(frame);
        moving = true;
        lastMoveMs = millis();
    }
    server.send(204);
}

void handleStop() {
    if (!authorised() || !requirePost()) return;
    sendStop();
    server.send(204);
}

void handlePreset() {
    if (!authorised() || !requirePost()) return;

    const String action = server.arg("action");
    const long id = server.arg("id").toInt();
    if (id < 1 || id > 255) {
        server.send(400, "text/plain", "id must be 1-255");
        return;
    }

    pelco::PresetAction a;
    if (action == "go") a = pelco::PresetAction::Go;
    else if (action == "set") a = pelco::PresetAction::Set;
    else if (action == "clear") a = pelco::PresetAction::Clear;
    else {
        server.send(400, "text/plain", "action must be go, set or clear");
        return;
    }

    uint8_t frame[pelco::kFrameSize];
    pelco::buildPreset(PTZ_ADDRESS, a, static_cast<uint8_t>(id), frame);
    sendFrame(frame);
    moving = false;
    server.send(204);
}

void handleStatus() {
    if (!authorised()) return;
    char json[128];
    snprintf(json, sizeof(json), "{\"address\":%d,\"baud\":%d,\"moving\":%s,\"rssi\":%d}",
             PTZ_ADDRESS, PTZ_BAUD, moving ? "true" : "false", WiFi.RSSI());
    server.send(200, "application/json", json);
}

void connectWifi() {
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(MDNS_HOSTNAME);
    WiFi.setAutoReconnect(true);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.printf("Connecting to %s", WIFI_SSID);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print('.');
    }
    Serial.printf("\nConnected: http://%s  (http://%s.local)\n",
                  WiFi.localIP().toString().c_str(), MDNS_HOSTNAME);
}

}  // namespace

void setup() {
    Serial.begin(115200);

    if (PTZ_DE_PIN >= 0) {
        pinMode(PTZ_DE_PIN, OUTPUT);
        digitalWrite(PTZ_DE_PIN, LOW);
    }
    ptzSerial.begin(PTZ_BAUD, SERIAL_8N1, PTZ_RX_PIN, PTZ_TX_PIN);
    sendStop();

    connectWifi();
    if (MDNS.begin(MDNS_HOSTNAME)) MDNS.addService("http", "tcp", 80);

    server.on("/", HTTP_GET, handleIndex);
    server.on("/api/move", handleMove);
    server.on("/api/stop", handleStop);
    server.on("/api/preset", handlePreset);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.onNotFound([] { server.send(404, "text/plain", "Not found"); });
    server.begin();
}

void loop() {
    server.handleClient();

    if (moving && millis() - lastMoveMs > kMoveTimeoutMs) {
        Serial.println("Keep-alive timeout, stopping");
        sendStop();
    }
}
