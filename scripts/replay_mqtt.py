#!/usr/bin/env python3
"""
Replay de eventos CardioIA via MQTT.

Este utilitário publica em lote mensagens JSON (formato JSON Lines) para o tópico de vitais,
simulando o comportamento do ESP32 ou validando o dashboard sem o hardware.

Exemplo:
    python scripts/replay_mqtt.py --broker broker.hivemq.com --port 1883 \
        --topic cardioia/v1/pacientes/paciente01/vitals \
        --file dados/exemplo.jsonl --delay 2
"""

from __future__ import annotations

import argparse
import json
import sys
import time
from pathlib import Path
from typing import Iterable, Optional

try:
    from paho.mqtt import client as mqtt
except ImportError as exc:
    raise SystemExit(
        "paho-mqtt não encontrado. Instale com 'pip install paho-mqtt'."
    ) from exc


def iter_payloads(source: Optional[Path]) -> Iterable[str]:
    """Lê mensagens a partir de um arquivo JSONL ou usa valores padrão."""
    if source is None:
        base = {
            "deviceId": "ESP32-EDGE-001",
            "patientId": "paciente01",
            "temperature": 36.7,
            "humidity": 54.2,
            "heartRate": 82,
            "movement": False,
            "battery": 95,
        }
        for offset in range(20):
            base["timestamp"] = int(time.time() * 1000) + offset * 1000
            base["heartRate"] = 80 + (offset % 5) * 5
            yield json.dumps(base)
        return

    with source.open("r", encoding="utf-8") as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            # valida se é JSON
            json.loads(line)
            yield line


def publish_messages(
    client: mqtt.Client,
    topic: str,
    payloads: Iterable[str],
    delay: float,
) -> None:
    """Publica mensagens com QoS 1."""
    for idx, payload in enumerate(payloads, start=1):
        result = client.publish(topic, payload, qos=1)
        status = result[0]
        if status != mqtt.MQTT_ERR_SUCCESS:
            print(f"[ERRO] Falha ao publicar mensagem #{idx}: {mqtt.error_string(status)}")
            break
        print(f"[OK] Mensagem #{idx} enviada: {payload}")
        time.sleep(delay)


def main() -> int:
    parser = argparse.ArgumentParser(description="Replay de dados CardioIA via MQTT.")
    parser.add_argument("--broker", required=True, help="Host do broker MQTT")
    parser.add_argument("--port", type=int, default=8883, help="Porta TCP do broker (default: 8883/TLS)")
    parser.add_argument("--topic", required=True, help="Tópico de publicação")
    parser.add_argument("--username", help="Usuário MQTT")
    parser.add_argument("--password", help="Senha MQTT")
    parser.add_argument("--file", type=Path, help="Arquivo JSONL com as mensagens (opcional)")
    parser.add_argument("--delay", type=float, default=1.5, help="Delay entre mensagens (s)")
    parser.add_argument("--insecure", action="store_true", help="Desabilita verificação TLS (ambiente de testes)")

    args = parser.parse_args()

    client = mqtt.Client(client_id="CardioIA-Replay", protocol=mqtt.MQTTv311)
    if args.username and args.password:
        client.username_pw_set(args.username, args.password)

    if args.port == 8883 and args.insecure:
        client.tls_set()
        client.tls_insecure_set(True)
    elif args.port == 8883:
        client.tls_set()

    print(f"[INFO] Conectando a {args.broker}:{args.port}...")
    client.connect(args.broker, args.port, keepalive=60)
    client.loop_start()

    try:
        payloads = iter_payloads(args.file)
        publish_messages(client, args.topic, payloads, args.delay)
    finally:
        time.sleep(1.0)
        client.loop_stop()
        client.disconnect()
        print("[INFO] Conexão encerrada.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
