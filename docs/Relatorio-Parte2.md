# CardioIA – Monitoramento Contínuo (Parte 2)

## 1. Visão Geral
A segunda fase da entrega evidencia a integração entre **Edge, Fog e Cloud Computing**, consolidando um ecossistema completo de monitoramento cardíaco. A partir do firmware desenvolvido na Parte 1, implementamos a transmissão via **MQTT**, construção de dashboards **Node-RED** e opção de análises históricas com **Grafana**. Este relatório descreve o fluxo de dados, decisões técnicas, requisitos de segurança e orientações práticas para replicação.

## 2. Arquitetura de Comunicação

### 2.1. Camadas
1. **Dispositivo Edge (ESP32)**  
   - Captura sinais vitais e lida com resiliência offline usando SPIFFS.  
   - Possui cliente MQTT que publica os dados (QoS 1) no tópico `cardioia/v1/pacientes/<id>/vitals`.  
   - Consome alertas no tópico `cardioia/v1/pacientes/<id>/alerts`, possibilitando atuação local (ex.: vibração ou notificação sonora).

2. **Broker MQTT (Fog/Cloud)**  
   - Utilizamos o **HiveMQ Cloud** (instância gratuita), com TLS ativado.  
   - Just Enough Fog: o broker funciona como camada intermediária desacoplando os produtores (wearables) dos consumidores (dashboards, alert services).  
   - Política de QoS 1 garante confirmação do recebimento pelo broker, reduzindo chances de perda em ambientes de rede instáveis.

3. **Node-RED (Fog/Edge Server)**  
   - Responsável por tratar os dados em tempo real, gerar alertas, alimentar interfaces e reencaminhar notificações críticas.  
   - Node-RED pode residir em um gateway hospitalar ou servidor em nuvem com baixa latência.

4. **Grafana + Banco de Séries Temporais (Cloud)**  
   - Armazena histórico para análises de tendências, relatórios de plantão e investigações clínicas.  
   - O painel importável proposto assume backend InfluxDB/Timescale; adaptações para outras fontes são diretas.

### 2.2. Topologia de Tópicos
- `cardioia/v1/pacientes/<id>/vitals` — stream primária de sinais vitais;
- `cardioia/v1/pacientes/<id>/alerts` — canal para alertas emitidos pelo Node-RED (feedback ao dispositivo ou aplicação móvel);
- Possibilidade de expansão para `cardioia/v1/pacientes/<id>/diagnostics` (resultados de IA) e `cardioia/v1/ops/logs` (observabilidade).

### 2.3. Mensagem Padrão
```json
{
  "deviceId": "ESP32-EDGE-001",
  "patientId": "paciente01",
  "timestamp": 1716300000000,
  "temperature": 36.7,
  "humidity": 55.2,
  "heartRate": 84,
  "movement": false,
  "battery": 93
}
```
O formato é compatível com serialização JSON Lines no Edge, facilitando a reprocessamento e ingestão por múltiplos consumidores.

## 3. Firmware com MQTT
O arquivo `wokwi/src/main.cpp` evolui para suportar conectividade segura:
- **Gestão de Credenciais**: secrets isolados em `secrets.h`, ausentes do controle de versão.  
- **Wi-Fi condicional**: o dispositivo só tenta se conectar quando a chave deslizante indica disponibilidade.  
- **TLS simplificado**: `WiFiClientSecure` com `setInsecure()` por padrão (ambiente acadêmico), pronto para ser substituído por CA real.  
- **MQTT resiliente**: reconexão automática, publicação de cada linha do buffer e assinatura dos alertas.  
- **Logs ricos**: prefixos `INFO`, `WARN`, `ERR`, `EDGE`, `SYNC` facilitam troubleshooting e auditoria clínica.

## 4. Node-RED Dashboard

### 4.1. Estrutura do Flow
O arquivo `node-red/flow-cardioia.json` pode ser importado diretamente pela interface do Node-RED. Componentes principais:
- **MQTT In** (`Vital Signs`) – recebe dados do broker; ajustar host, porta, usuário e senha.
- **JSON Parser** – converte a carga textual em objeto.
- **Função “Avaliar Alerta”** – aplica limites (120 bpm, 38 °C) e carimba o estado de alerta.
- **Dashboard Widgets**:
  - `ui_chart` (batimentos por minuto).
  - `ui_gauge` (temperatura).
  - `ui_led` (alerta visual) + `ui_text` (timestamp).
  - `ui_table` (log consolidado).
- **MQTT Out (`Publicar Alerta`)** – envia eventos críticos ao tópico de alertas, com retenção habilitada (`retain=true`) para que novos clientes recebam o último estado imediatamente.

### 4.2. Deploy e Customizações
1. Instale o Node-RED (>= 3.x) e o pacote `node-red-dashboard`.  
2. Importe o fluxo em *Menu → Import → Clipboard*.  
3. Edite os nós MQTT para apontar ao broker do projeto (TLS, usuário e senha do HiveMQ Cloud).  
4. Publique o fluxo e acesse o dashboard via `http://<host>:1880/ui`.  
5. Ajuste limites na função “Avaliar Alerta” conforme protocolos do time médico.

### 4.3. Estratégias de Alerta
- **Semáforo**: LED verde/vermelho em destaque.  
- **Retenção**: garante que alarmes persistam até revisão manual.  
- **Integração futura**: o mesmo fluxo pode chamar webhooks (Teams/Slack), enviar e-mails ou acionar sistemas de chamados hospitalares.

## 5. Grafana e Histórico de Dados

### 5.1. Pipeline Sugerido
1. **Node-RED** → **InfluxDB**: adicionar nó `influxdb out` para persistir cada mensagem.  
2. Importar `grafana/dashboard-cardioia.json` e mapear a fonte de dados (`DS_CARDIOIA`).  
3. Painéis incluídos:
   - Série temporal de batimentos agrupados por paciente.
   - Gauge de temperatura com thresholds clínicos.
   - Indicador de bateria, útil para manutenção preventiva do wearable.

### 5.2. Benefícios Operacionais
- Visualização 24h/7d das tendências.
- Identificação de pacientes com variação anormal de sinais vitais.
- Suporte a auditorias, estudos clínicos e análise de eficácia terapêutica.

## 6. Segurança & LGPD
1. **Criptografia em Trânsito**: TLS obrigatório no HiveMQ Cloud e recomendado em qualquer broker em produção.  
2. **Autenticação & Autorização**: cada dispositivo pode possuir credenciais exclusivas e ACLs restritivas (`paciente01/#`).  
3. **Segregação de Ambientes**: isolar tópicos de teste vs. produção; implementar quotas para evitar DDoS acidental.  
4. **Monitoramento de Logs**: Node-RED e broker devem registrar conexões e tentativas falhas para fins de auditoria.

> *Conformidade LGPD*: os dados no protótipo são sintéticos, mas a arquitetura contempla boas práticas – minimização, pseudonimização, rastreabilidade e possibilidade de exclusão.

## 7. Validação e Testes

### 7.1. Cenários Exercitados
- **Operação Offline Prolongada**: chave Wi-Fi desligada por 30 min, acumulando ~360 amostras. Ao reativar, o buffer é sincronizado em sequência e limpo.  
- **Limite de Armazenamento**: injeção de >5 000 registros para validar a rotação; resultado esperado: preservação dos dados mais recentes e log de aviso.  
- **Alertas**: simulação de febre (ajuste do DHT22 no Wokwi para 39 °C) resultou em LED vermelho local e publicação de alerta no Node-RED.  
- **Broker Indisponível**: desconexão forçada; mensagens permanecem em SPIFFS e são reenviadas após reconexão.

### 7.2. Ferramentas de Apoio
- `scripts/replay_mqtt.py`: envia dados gravados (JSON Lines) para o broker, útil para testar dashboards sem o ESP32.  
- Monitor serial Arduino / PlatformIO para inspeção em tempo real.  
- `mqtt-cli` ou `MQTT Explorer` para inspecionar tópicos e payloads.

## 8. Próximos Passos
1. **Edge Analytics**: aplicar filtros FIR/Kalman e detecção de arritmia local para reduzir volume de dados e gerar insights no próprio vestível.  
2. **Integração com IA**: encaminhar dados para modelos de previsão (fase “Coração sob Controle”) e emitir alertas preditivos.  
3. **Eventos Bidirecionais**: permitir que médicos ajustem limites em tempo real via tópico de comandos (`cardioia/v1/pacientes/<id>/commands`).  
4. **Compliance Avançado**: integrar a logs imutáveis (blockchain privado) para rastreabilidade de alertas críticos.  
5. **Escalabilidade**: estudar arquiteturas com Kubernetes + HiveMQ Cluster ou AWS IoT Core, garantindo elasticidade para milhares de pacientes.

## 9. Conclusão
A Parte 2 conclui a jornada desta fase do CardioIA demonstrando um pipeline completo de **captura → processamento local → transmissão MQTT → visualização e alertas**. A solução alinha práticas de engenharia de software, IoT médica e governança de dados, mantendo foco em confiabilidade e observabilidade. Com a base estabelecida, a equipe está pronta para evoluir para os módulos de previsão com IA, diagnóstico assistido e orquestração de respostas clínicas em larga escala.
