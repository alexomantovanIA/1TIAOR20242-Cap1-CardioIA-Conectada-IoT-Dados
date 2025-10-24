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
Esta etapa do projeto **CardioIA** transforma o conceito de monitoramento cardíaco contínuo em um ecossistema IoT completo. O protótipo combina **ESP32**, **sensores biométricos simulados**, **LittleFS**, **MQTT**, **Node-RED** e integração com **Grafana** para demonstrar o fluxo ponta a ponta: captura dos sinais vitais, resiliência offline no Edge, sincronização com a nuvem e visualização interativa com alertas automáticos.

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
2. **Desenho do Edge Resiliente**: uso de LittleFS com buffer circular, controle de lotação e sincronização pós-falha (no Wokwi o firmware ativa automaticamente uma camada compatível com SPIFFS).
3. **Backbone MQTT Seguro**: conexão TLS com HiveMQ Cloud, tópicos versionados e QoS alinhado ao risco clínico.
4. **Visualização & Alertas**: dashboards em Node-RED (tempo real) e Grafana (tendências históricas), com thresholds configuráveis.
5. **Governança de Dados**: segmentation por paciente, logs de auditoria e diretrizes de LGPD aplicadas a um cenário médico.

---

## ⚙️ Como Executar

### 0. Requisitos
- **Python 3.9+** (para o script de replay) – `pip install -r requirements.txt`.
- **Node.js 18+** com **Node-RED 4.x** instalados globalmente (`npm install -g --unsafe-perm node-red`).
- Conta no **HiveMQ Cloud** (ou broker MQTT equivalente) com TLS habilitado.
- Conta no **InfluxDB Cloud** (bucket `cardioia-influx`) e no **Grafana Cloud**.
- Opcional: **PlatformIO** para compilar o firmware localmente.

> **Importante:** renomeie `wokwi/secrets-template.h` para `wokwi/secrets.h` e preencha com suas credenciais. O arquivo real é ignorado pelo Git para evitar vazamentos.

### 1. Edge – ESP32 (Wokwi ou PlatformIO)
**Simulação no Wokwi**
1. Abra [wokwi.com](https://wokwi.com/) → *Import Project* → cole o conteúdo de `wokwi/diagram.json`.
2. Faça upload de `wokwi/src/main.cpp` e de `wokwi/secrets.h` com SSID/MQTT reais.
3. Inicie a simulação. Use o monitor serial para acompanhar logs `EDGE`, `SYNC` e `MQTT`.
4. O slide-switch virtual emula perda de conectividade (pin 13). O LED vermelho (pin 2) acende em alertas (>120 bpm ou >38 °C).

**Execução Física (opcional)**
1. Crie um novo projeto PlatformIO com board `esp32dev` e copie `wokwi/src/main.cpp` para `src/main.cpp`.
2. Adicione ao `platformio.ini` as dependências `ArduinoJson`, `PubSubClient`, `DHT sensor library` e `Adafruit Unified Sensor`.
3. Crie `include/secrets.h` (mesmo formato do template) e rode `platformio run -t upload` para gravar no ESP32.
4. `platformio device monitor` permite inspecionar o serial em 115200 bps.

### 2. Broker MQTT (HiveMQ Cloud)
1. Crie um par usuário/senha na aba *Access Management* e permita `PUBLISH_SUBSCRIBE` no tópico `cardioia/v1/pacientes/#`.
2. No Wokwi/ESP32, configure host, porta 8883 e credenciais.
3. No Node-RED (passo a seguir), use o mesmo par de credenciais.

### 3. Node-RED – Pipeline Fog
1. Instale as dependências no workspace local (`node-red/workspace`):
   ```bash
   cd node-red/workspace
   npm install node-red-dashboard node-red-contrib-ui-led node-red-node-ui-table node-red-contrib-influxdb
   ```
2. Copie o fluxo de referência (repita este passo sempre que atualizar o repositório):
   ```bash
   cp ../flow-cardioia.json flows.json
   ```
3. Execute `node-red --userDir $(pwd)`.
4. No editor (http://127.0.0.1:1880/):
   - Abra o nó **Vital Signs (HiveMQ)** e configure host, porta `8883`, usuário e senha. Utilize TLS com a configuração `HiveMQ TLS` (CA incluída).
   - No nó **Publicar Alerta** repita as credenciais.
   - Abra **InfluxDB Cloud** e informe URL `https://us-east-1-1.aws.cloud2.influxdata.com`, Organization `mccortex`, Bucket `cardioia-influx` e o token gerado no Influx.
   - (Opcional) em `settings.js` defina `credentialSecret: "sua-chave"` para evitar o aviso de chave temporária e proteger `flows_cred.json`.
   - Clique em **Deploy**.
5. O dashboard web fica disponível em `http://127.0.0.1:1880/ui`.

### 4. Persistência – InfluxDB Cloud
1. Crie o bucket `cardioia-influx` e um API Token com acesso de escrita/leitura.
2. Garanta que o Node-RED está enviando pontos (menu *Data Explorer* > bucket > `cardioia_vitals`).

### 5. Data Replay / Testes Locais
Use o utilitário Python para simular três pacientes com diferentes perfis:
```bash
python3 scripts/replay_mqtt.py \
  --broker 3dcf472eb0354f378dd6e2fb084a72c8.s1.eu.hivemq.cloud \
  --username cardioia-node \
  --password 'SUA_SENHA' \
  --loops 3 \
  --delay 1.0
```
- O parâmetro `--topic-template` permite customizar o tópico (por padrão `cardioia/v1/pacientes/{patientId}/vitals`).
- Para reusar dados gravados, informe `--file caminho/para/dados.jsonl`.

### 6. Visualização – Grafana Cloud
1. Em *Connections → Data sources*, adicione uma fonte InfluxDB (Flux):
   - URL: `https://us-east-1-1.aws.cloud2.influxdata.com`
   - Organization: `mccortex`
   - Bucket: `cardioia-influx`
   - Authorization header: `Token <seu_token>`
   - Salve com o nome `CardioIA Influx`.
2. Importe `grafana/dashboard-cardioia.json` e selecione `CardioIA Influx` quando solicitado. O arquivo já utiliza consultas Flux compatíveis com InfluxDB Cloud.
3. Ajuste a variável `patient` no topo do painel para filtrar cada paciente ou escolha `All`.

Após executar o script de replay (ou o ESP32 real), o dashboard exibirá séries de BPM, temperatura e bateria, além de alertas no Node-RED.

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
├─ requirements.txt
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
