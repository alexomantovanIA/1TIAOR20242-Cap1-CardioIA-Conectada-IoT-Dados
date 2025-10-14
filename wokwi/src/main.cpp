#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <SPIFFS.h>
#include <DHT.h>
#include <ArduinoJson.h>

#if __has_include("../secrets.h")
#include "../secrets.h"
#else
#include "../secrets-template.h"
#warning "Usando secrets-template.h. Crie um arquivo secrets.h com as credenciais reais."
#endif

// ---- Configurações de hardware ----
constexpr uint8_t DHT_PIN = 15;
constexpr uint8_t HEART_SENSOR_PIN = 34;
constexpr uint8_t WIFI_SWITCH_PIN = 13;
constexpr uint8_t ALERT_LED_PIN = 2;

constexpr unsigned long SAMPLE_INTERVAL_MS = 5000;
constexpr const char* BUFFER_PATH = "/buffer.jsonl";
constexpr size_t MAX_RECORDS = 5000;

constexpr float ALERT_TEMP_THRESHOLD = 38.0f;
constexpr uint16_t ALERT_HR_THRESHOLD = 120;

DHT dht(DHT_PIN, DHT22);
WiFiClientSecure secureClient;
PubSubClient mqttClient(secureClient);

struct VitalSample {
  unsigned long timestamp;
  float temperature;
  float humidity;
  uint16_t heartRate;
  bool movement;
  uint8_t battery;
};

uint16_t previousHeartRate = 75;
uint8_t batteryLevel = 98;
unsigned long lastSampleAt = 0;

// ---- Funções utilitárias ----

bool wifiSwitchOn() {
  return digitalRead(WIFI_SWITCH_PIN) == HIGH;
}

void logAvailableMemory() {
#if defined(ESP32)
  Serial.printf("DBG: RAM livre ~ %d bytes\n", esp_get_free_heap_size());
#endif
}

void mountFileSystem() {
  if (!SPIFFS.begin(true)) {
    Serial.println("ERR: Falha ao montar SPIFFS");
  } else {
    Serial.println("INFO: SPIFFS montado com sucesso");
  }
}

String serializeSample(const VitalSample& sample) {
  StaticJsonDocument<256> doc;
  doc["deviceId"] = "ESP32-EDGE-001";
  doc["patientId"] = "paciente01";
  doc["timestamp"] = sample.timestamp;
  doc["temperature"] = sample.temperature;
  doc["humidity"] = sample.humidity;
  doc["heartRate"] = sample.heartRate;
  doc["movement"] = sample.movement;
  doc["battery"] = sample.battery;

  String payload;
  serializeJson(doc, payload);
  return payload;
}

size_t countStoredSamples() {
  if (!SPIFFS.exists(BUFFER_PATH)) {
    return 0;
  }

  File file = SPIFFS.open(BUFFER_PATH, FILE_READ);
  if (!file) {
    return 0;
  }

  size_t count = 0;
  while (file.available()) {
    file.readStringUntil('\n');
    count++;
  }
  file.close();
  return count;
}

void trimBufferIfNeeded() {
  size_t records = countStoredSamples();
  if (records <= MAX_RECORDS) {
    return;
  }

  Serial.printf("WARN: Limite de %u registros atingido. Mantendo os %u mais recentes.\n",
                static_cast<unsigned>(records), static_cast<unsigned>(MAX_RECORDS));

  File input = SPIFFS.open(BUFFER_PATH, FILE_READ);
  File temp = SPIFFS.open("/tmp.jsonl", FILE_WRITE);
  if (!input || !temp) {
    Serial.println("ERR: Não foi possível aparar o buffer");
    return;
  }

  size_t skip = records - MAX_RECORDS;
  while (input.available()) {
    String line = input.readStringUntil('\n');
    if (skip > 0) {
      skip--;
      continue;
    }
    temp.println(line);
  }
  input.close();
  temp.close();

  SPIFFS.remove(BUFFER_PATH);
  SPIFFS.rename("/tmp.jsonl", BUFFER_PATH);
}

void appendSampleToBuffer(const String& payload) {
  File file = SPIFFS.open(BUFFER_PATH, FILE_APPEND);
  if (!file) {
    Serial.println("ERR: Não foi possível abrir o buffer SPIFFS");
    return;
  }
  file.println(payload);
  file.close();
  Serial.printf("EDGE: Amostra salva localmente. Total armazenado: %u\n",
                static_cast<unsigned>(countStoredSamples()));
}

void updateAlertLed(float temperature, uint16_t heartRate) {
  bool alert = (temperature >= ALERT_TEMP_THRESHOLD) || (heartRate >= ALERT_HR_THRESHOLD);
  digitalWrite(ALERT_LED_PIN, alert ? HIGH : LOW);
}

uint16_t readHeartRate() {
  int raw = analogRead(HEART_SENSOR_PIN);
  if (raw <= 0) {
    raw = 2048 + random(-200, 200);  // fallback caso o sensor não responda
  }
  return map(raw, 0, 4095, 48, 150);
}

VitalSample captureSample() {
  VitalSample sample{};
  sample.timestamp = millis();
  sample.temperature = dht.readTemperature();
  sample.humidity = dht.readHumidity();
  sample.heartRate = readHeartRate();
  sample.movement = abs(static_cast<int>(sample.heartRate) - static_cast<int>(previousHeartRate)) > 12;

  previousHeartRate = sample.heartRate;

  if (batteryLevel > 5) {
    batteryLevel -= random(0, 2);  // taxa de drenagem suave
  } else {
    batteryLevel = 100;            // recarga simulada
  }
  sample.battery = batteryLevel;

  if (isnan(sample.temperature) || isnan(sample.humidity)) {
    Serial.println("WARN: Falha na leitura do DHT22. Reutilizando últimos valores conhecidos.");
    sample.temperature = 36.5f + random(-5, 5) * 0.1f;
    sample.humidity = 55.0f + random(-10, 10) * 0.1f;
  }

  return sample;
}

void ensureWifiDisconnected() {
  if (mqttClient.connected()) {
    mqttClient.disconnect();
  }
  if (WiFi.isConnected()) {
    Serial.println("INFO: Desconectando Wi-Fi (simulação offline).");
    WiFi.disconnect(true, true);
  }
}

bool secretsConfigured() {
  return strcmp(WIFI_SSID, "SEU_WIFI") != 0 && strlen(WIFI_SSID) > 0;
}

void ensureWifiConnected() {
  if (!wifiSwitchOn()) {
    ensureWifiDisconnected();
    return;
  }

  if (!secretsConfigured()) {
    Serial.println("WARN: secrets.h não configurado. Operando apenas com logs locais.");
    return;
  }

  if (WiFi.isConnected()) {
    return;
  }

  Serial.printf("INFO: Conectando ao Wi-Fi %s ...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttempt = millis();
  while (!WiFi.isConnected() && (millis() - startAttempt) < 10000) {
    delay(200);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.isConnected()) {
    Serial.printf("INFO: Wi-Fi conectado. IP: %s\n", WiFi.localIP().toString().c_str());
  } else {
    Serial.println("ERR: Não foi possível conectar ao Wi-Fi dentro do tempo limite.");
  }
}

void ensureMqttConnected() {
  if (!WiFi.isConnected() || !wifiSwitchOn()) {
    return;
  }

  if (mqttClient.connected()) {
    return;
  }

  secureClient.setInsecure();  // Ambiente acadêmico
  mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);

  String clientId = String("CardioIA-") + String(WiFi.macAddress());
  Serial.printf("INFO: Conectando ao broker MQTT (%s:%u)...\n", MQTT_BROKER_HOST, MQTT_BROKER_PORT);

  if (mqttClient.connect(clientId.c_str(), MQTT_USERNAME, MQTT_PASSWORD)) {
    Serial.println("INFO: MQTT conectado com sucesso.");
    mqttClient.subscribe(MQTT_TOPIC_ALERTS);
  } else {
    Serial.printf("ERR: Falha MQTT rc=%d. Tente novamente mais tarde.\n", mqttClient.state());
  }
}

bool publishPayload(const String& payload) {
  if (mqttClient.connected()) {
    bool ok = mqttClient.publish(MQTT_TOPIC_VITALS, payload.c_str());
    if (ok) {
      Serial.printf("MQTT: Enviado -> %s\n", payload.c_str());
    } else {
      Serial.println("ERR: Falha ao publicar no MQTT.");
    }
    return ok;
  }

  Serial.printf("CLOUD: Conectado (simulado). Payload -> %s\n", payload.c_str());
  Serial.println("INFO: Dados impressos no Serial por indisponibilidade de MQTT.");
  return true;
}

void syncBufferToCloud() {
  if (!SPIFFS.exists(BUFFER_PATH)) {
    return;
  }

  File input = SPIFFS.open(BUFFER_PATH, FILE_READ);
  if (!input) {
    Serial.println("ERR: Não foi possível abrir o buffer para leitura.");
    return;
  }

  File temp = SPIFFS.open("/tmp.jsonl", FILE_WRITE);
  if (!temp) {
    Serial.println("ERR: Não foi possível abrir buffer temporário.");
    input.close();
    return;
  }

  bool publishFailure = false;
  while (input.available()) {
    String line = input.readStringUntil('\n');
    line.trim();
    if (line.isEmpty()) {
      continue;
    }

    if (!publishPayload(line)) {
      publishFailure = true;
      temp.println(line);
      while (input.available()) {
        String tail = input.readStringUntil('\n');
        tail.trim();
        if (!tail.isEmpty()) {
          temp.println(tail);
        }
      }
      break;
    }
  }

  input.close();
  temp.close();

  if (publishFailure) {
    SPIFFS.remove(BUFFER_PATH);
    SPIFFS.rename("/tmp.jsonl", BUFFER_PATH);
    Serial.println("WARN: Nem todos os dados foram sincronizados. Restante preservado.");
  } else {
    SPIFFS.remove(BUFFER_PATH);
    SPIFFS.remove("/tmp.jsonl");
    Serial.println("SYNC: Buffer sincronizado e limpo.");
  }
}

// ---- Arduino lifecycle ----

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== CardioIA Edge Node ===");

  pinMode(WIFI_SWITCH_PIN, INPUT_PULLDOWN);
  pinMode(ALERT_LED_PIN, OUTPUT);
  digitalWrite(ALERT_LED_PIN, LOW);

  dht.begin();
  mountFileSystem();
  logAvailableMemory();
}

void loop() {
  unsigned long now = millis();

  if (wifiSwitchOn()) {
    ensureWifiConnected();
    ensureMqttConnected();
  } else {
    ensureWifiDisconnected();
  }

  mqttClient.loop();

  if (now - lastSampleAt >= SAMPLE_INTERVAL_MS) {
    lastSampleAt = now;
    VitalSample sample = captureSample();
    String payload = serializeSample(sample);

    updateAlertLed(sample.temperature, sample.heartRate);
    appendSampleToBuffer(payload);
    trimBufferIfNeeded();

    Serial.printf("EDGE: t=%.2f°C, h=%.2f%%, bpm=%u, move=%d, bat=%u%%\n",
                  sample.temperature,
                  sample.humidity,
                  sample.heartRate,
                  sample.movement,
                  sample.battery);
  }

  if (wifiSwitchOn() && (mqttClient.connected() || !secretsConfigured())) {
    syncBufferToCloud();
  }
}
