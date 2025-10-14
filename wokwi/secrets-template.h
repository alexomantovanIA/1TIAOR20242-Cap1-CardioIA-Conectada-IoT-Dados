#pragma once

/**
 * Renomeie este arquivo para 'secrets.h' e ajuste as credenciais
 * antes de compilar no ESP32 real ou em outro ambiente conectado.
 *
 * Este arquivo não deve ser versionado com dados sensíveis.
 */

static constexpr const char* WIFI_SSID = "SEU_WIFI";
static constexpr const char* WIFI_PASSWORD = "SUA_SENHA";

static constexpr const char* MQTT_BROKER_HOST = "broker.hivemq.com";
static constexpr uint16_t MQTT_BROKER_PORT = 8883;  // TLS
static constexpr const char* MQTT_USERNAME = "seu-usuario";
static constexpr const char* MQTT_PASSWORD = "sua-senha";

static constexpr const char* MQTT_TOPIC_VITALS = "cardioia/v1/pacientes/paciente01/vitals";
static constexpr const char* MQTT_TOPIC_ALERTS = "cardioia/v1/pacientes/paciente01/alerts";
