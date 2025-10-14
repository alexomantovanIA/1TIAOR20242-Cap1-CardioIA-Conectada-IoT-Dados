# FIAP - Faculdade de Informática e Administração Paulista

<p align="center">
<a href="https://www.fiap.com.br/"><img src="assets/logo-fiap.png" alt="FIAP - Faculdade de Informática e Administração Paulista" border="0" width="40%" height="40%"></a>
</p>

---

# CardioIA – Monitoramento Contínuo com IoT & Dados Conectados

### Fase 3

---

## 👨‍🎓 Integrantes
- [Alexandre Oliveira Mantovani](https://www.linkedin.com/in/alexomantovani)
- [Edmar Ferreira Souza](https://www.linkedin.com/in/)
- [Ricardo Lourenço Coube](https://www.linkedin.com/in/ricardolcoube/)
- [Jose Andre Filho](https://www.linkedin.com/in/joseandrefilho)

## 👩‍🏫 Professores
- Tutor: [Leonardo Ruiz Orabona](https://www.linkedin.com/in/leonardoorabona)
- Coordenador: [André Godoi](https://www.linkedin.com/in/profandregodoi)

---

## 📌 Descrição do Projeto
Esta etapa do projeto **CardioIA** transforma o conceito de monitoramento cardíaco contínuo em um ecossistema IoT completo. O protótipo combina **ESP32**, **sensores biométricos simulados**, **SPIFFS**, **MQTT**, **Node-RED** e integração com **Grafana** para demonstrar o fluxo ponta a ponta: captura dos sinais vitais, resiliência offline no Edge, sincronização com a nuvem e visualização interativa com alertas automáticos.

> **Governança & Ética (LGPD)**: todos os dados são simulados/anônimos e destinados ao aprendizado acadêmico. Este conteúdo **não** substitui diagnóstico ou acompanhamento médico.

---

## 📦 Entregáveis

### 🛰️ Parte 1 — Edge Computing & Resiliência Offline
- **Projeto Wokwi (ESP32 + sensores)**: `wokwi/diagram.json`
- **Firmware comentado (C++)**: `wokwi/src/main.cpp`
- **Template de credenciais**: `wokwi/secrets-template.h`
- **Relatório técnico (≥ 1 página)**: `docs/Relatorio-Parte1.md`
- **Capturas/prints do protótipo**: `docs/imagens/`

### ☁️ Parte 2 — MQTT, Dashboard e Inteligência Operacional
- **Publicação MQTT com reenvio confiável** integrado no firmware (`wokwi/src/main.cpp`)
- **Flow do Node-RED com dashboard completo**: `node-red/flow-cardioia.json`
- **Template de dashboard Grafana (opcional)**: `grafana/dashboard-cardioia.json`
- **Scripts utilitários** (ex.: replay de dados MQTT): `scripts/`
- **Relatório detalhado (≥ 2 páginas)**: `docs/Relatorio-Parte2.md`
- **Evidências visuais das telas**: `docs/imagens/`

---

## 🧪 Metodologia
1. **Mapeamento dos Requisitos Clínicos**: seleção dos sinais vitais relevantes (temperatura, umidade, batimentos e movimento).
2. **Desenho do Edge Resiliente**: uso de SPIFFS com buffer circular, controle de lotação e sincronização pós-falha.
3. **Backbone MQTT Seguro**: conexão TLS com HiveMQ Cloud, tópicos versionados e QoS alinhado ao risco clínico.
4. **Visualização & Alertas**: dashboards em Node-RED (tempo real) e Grafana (tendências históricas), com thresholds configuráveis.
5. **Governança de Dados**: segmentation por paciente, logs de auditoria e diretrizes de LGPD aplicadas a um cenário médico.

---

## ⚙️ Como Executar

### Parte 1 – Simulação no Wokwi
1. Abra o projeto no [Wokwi](https://wokwi.com/) e importe `wokwi/diagram.json`.
2. Substitua `wokwi/secrets-template.h` por `secrets.h` contendo SSID e senha (se necessário).
3. Compile `wokwi/src/main.cpp` (Arduino Framework) e inicie a simulação.
4. Utilize o monitor serial para observar:
   - **Coleta periódica** dos sensores (DHT22 + sensor de batimentos/movimento).
   - **Armazenamento em SPIFFS** durante indisponibilidade de rede.
   - **Sincronização automática** quando a variável de conectividade for ligada.

### Parte 2 – Integração MQTT e Dashboard
1. Configure credenciais no `secrets.h` (Wi-Fi e MQTT).
2. Acesse o broker (ex.: HiveMQ Cloud) e crie os tópicos:
   - `cardioia/v1/pacientes/<id>/vitals`
   - `cardioia/v1/pacientes/<id>/alerts`
3. Importe `node-red/flow-cardioia.json` no Node-RED.
4. Ajuste os nós MQTT com as credenciais fornecidas pelo broker.
5. Publique/consuma os dados:
   - Gráfico de batimentos e temperatura em tempo real.
   - Gauge de temperatura com estado operacional.
   - Alerta visual quando limites definidos forem ultrapassados.
6. (Opcional) Importe `grafana/dashboard-cardioia.json` no Grafana para análises históricas.

---

## 📊 Métricas Observadas
- **Latência Edge → Cloud**: sincronização controlada em lote com logs serializados por JSON.
- **Capacidade Offline**: armazenamento configurável (`MAX_RECORDS`) mantendo dados por horas de operação.
- **Dashboards**: alertas imediatos para >120 bpm ou >38 °C, com histórico de 24h no banco Time Series.
- **Segurança**: autenticação MQTT, segregação de tópicos por paciente e diretrizes de encriptação em trânsito.

---

## 🗂️ Estrutura do Projeto
```
📦 1TIAOR20242-Cap1-CardioIA-Conectada-IoT-Dados
│
├─ assets/
│   └─ logo-fiap.png
├─ docs/
│   ├─ Relatorio-Parte1.md
│   ├─ Relatorio-Parte2.md
│   └─ imagens/
│       └─ .gitkeep
├─ grafana/
│   └─ dashboard-cardioia.json
├─ node-red/
│   └─ flow-cardioia.json
├─ scripts/
│   └─ replay_mqtt.py
├─ wokwi/
│   ├─ diagram.json
│   ├─ src/
│   │   └─ main.cpp
│   └─ secrets-template.h
└─ README.md
```

---

## ✅ Requisitos para Execução
- **Arduino IDE** ou **PlatformIO** com suporte ao ESP32.
- Conta em um broker **MQTT** (ex.: HiveMQ Cloud) com TLS.
- Ambiente **Node-RED** ≥ 3.x, dashboard plugin habilitado.
- (Opcional) Conta **Grafana Cloud** ou instância local com InfluxDB.

---

## 📝 Licença
<p xmlns:cc="http://creativecommons.org/ns#" xmlns:dct="http://purl.org/dc/terms/">
Este projeto segue o modelo FIAP e está licenciado sob 
<a href="http://creativecommons.org/licenses/by/4.0/?ref=chooser-v1" target="_blank" rel="license noopener noreferrer">Attribution 4.0 International (CC BY 4.0)</a>.
</p>
