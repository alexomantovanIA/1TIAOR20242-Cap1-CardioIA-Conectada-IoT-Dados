#!/usr/bin/env python3
"""
Replay de eventos CardioIA via MQTT com cenários clínicos variados.

Exemplos:
    python scripts/replay_mqtt.py --broker hive.example.com --username user --password pass
    python scripts/replay_mqtt.py --broker hive.example.com --topic-template "cardioia/v1/{patientId}/vitals"
"""

from __future__ import annotations

import argparse
import json
import math
import random
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Dict, Iterable, Iterator, List, Optional

try:
    from paho.mqtt import client as mqtt
except ImportError as exc:  # pragma: no cover
    raise SystemExit("Instale 'paho-mqtt' (pip install paho-mqtt).") from exc


# ---------------------------------------------------------------------------
# Perfis clínicos sintéticos
# ---------------------------------------------------------------------------


@dataclass
class PatientProfile:
    patient_id: str
    device_id: str
    base_hr: int
    base_temp: float
    diagnosis: str
    battery: int = 100

    def scenario_events(self) -> Iterator[Dict[str, object]]:
        """
        Gera uma sequência de eventos (8 amostras) representando fases distintas:
        baseline, reabilitação, teste de estresse e recuperação.
        """
        timestamp = int(time.time() * 1000)
        for minute, phase in enumerate(self._phase_schedule()):
            hr, temp, movement = self._apply_phase(phase, minute)
            self.battery = max(40, self.battery - random.randint(0, 1))
            yield {
                "deviceId": self.device_id,
                "patientId": self.patient_id,
                "timestamp": timestamp + minute * 3000,
                "temperature": round(temp, 1),
                "humidity": round(52 + random.uniform(-6, 6), 1),
                "heartRate": hr,
                "movement": movement,
                "battery": self.battery,
                "diagnosis": self.diagnosis,
            }

    def _phase_schedule(self) -> List[str]:
        return [
            "baseline",
            "baseline",
            "rehab",
            "rehab",
            "stress-test",
            "stress-test",
            "recovery",
            "recovery",
        ]

    def _apply_phase(self, phase: str, minute: int) -> tuple[int, float, bool]:
        jitter = random.randint(-3, 3)
        if phase == "baseline":
            hr = self.base_hr + jitter
            temp = self.base_temp + random.uniform(-0.1, 0.1)
            return hr, temp, False
        if phase == "rehab":
            hr = self.base_hr + 10 + jitter
            temp = self.base_temp + 0.2
            return hr, temp, True
        if phase == "stress-test":
            hr = min(165, self.base_hr + 30 + random.randint(5, 15))
            temp = self.base_temp + 0.6
            return hr, temp, True
        if phase == "recovery":
            decay = int(12 * math.exp(-0.8 * minute))
            hr = max(self.base_hr, self.base_hr + decay + jitter)
            temp = self.base_temp + max(0, 0.3 - 0.15 * minute)
            return hr, temp, False
        return self.base_hr + jitter, self.base_temp, False


DEFAULT_PROFILES: List[PatientProfile] = [
    PatientProfile(
        "paciente01",
        "ESP32-EDGE-001",
        base_hr=78,
        base_temp=36.6,
        diagnosis="Pós-angioplastia",
    ),
    PatientProfile(
        "paciente02",
        "ESP32-EDGE-002",
        base_hr=88,
        base_temp=36.8,
        diagnosis="Insuficiência cardíaca leve",
    ),
    PatientProfile(
        "paciente03",
        "ESP32-EDGE-003",
        base_hr=70,
        base_temp=37.0,
        diagnosis="Reabilitação pós-AVC",
    ),
]


# ---------------------------------------------------------------------------
# Geração de payloads
# ---------------------------------------------------------------------------


def iter_payloads(source: Optional[Path], *, loops: int = 1) -> Iterable[Dict[str, object]]:
    """Obtém eventos a partir de arquivo JSONL ou gera dados sintéticos."""
    if source:
        with source.open("r", encoding="utf-8") as handle:
            for line in handle:
                line = line.strip()
                if not line:
                    continue
                event = json.loads(line)
                if "patientId" not in event:
                    raise ValueError("JSONL deve conter a chave 'patientId'.")
                yield event
        return

    for _ in range(max(1, loops)):
        for profile in DEFAULT_PROFILES:
            for event in profile.scenario_events():
                yield event


# ---------------------------------------------------------------------------
# Publicação
# ---------------------------------------------------------------------------


def resolve_topic(template: str, event: Dict[str, object]) -> str:
    """Substitui placeholders do template pelo conteúdo do evento."""
    if "+" in template or "#" in template:
        raise ValueError(
            "O tópico não pode conter curingas MQTT. Use placeholders como {patientId}."
        )
    if "{" in template and "}" in template:
        try:
            return template.format(**event)
        except KeyError as exc:
            missing = exc.args[0]
            raise KeyError(
                f"Placeholder {{{missing}}} ausente na mensagem. Evento: {event}"
            ) from exc
    return template


def publish_messages(
    client: mqtt.Client,
    topic_template: str,
    payloads: Iterable[Dict[str, object]],
    delay: float,
) -> None:
    for idx, event in enumerate(payloads, start=1):
        topic = resolve_topic(topic_template, event)
        payload = json.dumps(event)
        status = client.publish(topic, payload, qos=1)[0]
        if status != mqtt.MQTT_ERR_SUCCESS:
            print(f"[ERRO] Publicação #{idx} falhou: {mqtt.error_string(status)}")
            break
        print(f"[OK] {topic} <- {payload}")
        time.sleep(delay)


# ---------------------------------------------------------------------------
# CLI
# ---------------------------------------------------------------------------


def main() -> int:
    parser = argparse.ArgumentParser(description="Replay de dados CardioIA via MQTT.")
    parser.add_argument("--broker", required=True, help="Host do broker MQTT")
    parser.add_argument(
        "--port", type=int, default=8883, help="Porta TCP (padrão: 8883 - TLS)"
    )
    parser.add_argument(
        "--topic-template",
        default="cardioia/v1/pacientes/{patientId}/vitals",
        help="Template do tópico (placeholders: {patientId}, {deviceId}, ...).",
    )
    parser.add_argument("--username", help="Usuário MQTT")
    parser.add_argument("--password", help="Senha MQTT")
    parser.add_argument(
        "--file",
        type=Path,
        help="Arquivo JSONL para replay manual (opcional).",
    )
    parser.add_argument(
        "--loops",
        type=int,
        default=1,
        help="Número de ciclos completos a repetir (padrão: 1).",
    )
    parser.add_argument(
        "--delay",
        type=float,
        default=1.5,
        help="Intervalo entre publicações em segundos (padrão: 1.5).",
    )
    parser.add_argument(
        "--insecure",
        action="store_true",
        help="Desabilita verificação TLS (use apenas em ambiente local).",
    )

    args = parser.parse_args()

    client = mqtt.Client(client_id="CardioIA-Replay", protocol=mqtt.MQTTv311)
    if args.username and args.password:
        client.username_pw_set(args.username, args.password)

    if args.port == 8883:
        client.tls_set()
        if args.insecure:
            client.tls_insecure_set(True)

    print(f"[INFO] Conectando a {args.broker}:{args.port}...")
    client.connect(args.broker, args.port, keepalive=60)
    client.loop_start()

    try:
        payloads = iter_payloads(args.file, loops=args.loops)
        publish_messages(client, args.topic_template, payloads, args.delay)
    finally:
        time.sleep(1.0)
        client.loop_stop()
        client.disconnect()
        print("[INFO] Conexão encerrada.")

    return 0


if __name__ == "__main__":
    sys.exit(main())
