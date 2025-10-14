# CardioIA – Monitoramento Contínuo (Parte 1)

## 1. Introdução
A fase **CardioIA – Conectada, IoT & Dados** está inserida no roadmap do projeto CardioIA como evolução natural após o módulo de Batimentos de Dados. Neste estágio, concentramos esforços no **monitoramento contínuo** com foco em resiliência e garantia de operação ininterrupta em cenários críticos. O objetivo é prototipar, por meio do simulador Wokwi e do microcontrolador ESP32, um dispositivo vestível capaz de capturar sinais vitais, armazená-los localmente (Edge Computing) e sincronizá-los com a nuvem assim que a conectividade estiver disponível.

## 2. Escopo e Sensores
- **DHT22**: leitura simultânea de temperatura e umidade do paciente (ex.: sensor incorporado ao colete).
- **Sensor de batimentos / movimento**: utiliza o `Pulse Sensor` para inferir frequência cardíaca e detecção de picos de movimento, importante para identificar situações de esforço súbito ou quedas.
- **LED de alerta**: atua como feedback local caso a temperatura esteja acima de 38 °C ou a frequência cardíaca ultrapasse 120 bpm.
- **Chave deslizante**: simula o estado de conectividade Wi-Fi, permitindo forçar cenários offline.

## 3. Arquitetura Edge
O firmware (Arduino/C++) faz uso intensivo de **SPIFFS** para armazenar cada amostra em formato JSON Lines, preservando os dados mesmo com queda de energia ou ausência de rede. Os principais componentes do software:
1. **Loop de aquisição** (intervalo de 5 s) lê sensores, normaliza valores e registra o timestamp em milissegundos.
2. **Buffer local** (`/buffer.jsonl`) armazena até 5 000 amostras. Ao ultrapassar o limite, a função `trimBufferIfNeeded()` garante que apenas os registros mais recentes sejam mantidos, obedecendo a uma política de retenção alinhada ao SLA clínico.
3. **Sincronização condicional**: ao detectar conectividade, o sistema percorre o buffer e publica cada linha via Serial/MQTT. Mensagens não enviadas retornam para o buffer temporário, preservando integridade.
4. **LED de Alerta**: acionamento imediato (feedback tátil/visual) sem depender de backend, reforçando a autonomia do Edge.

## 4. Fluxo Operacional
1. **Boot**: inicializa sensores, monta SPIFFS e exibe informações de memória livre.
2. **Captura**: o sensor DHT22 fornece temperatura/umidade; a frequência cardíaca é calculada via leitura analógica do `Pulse Sensor` e normalizada (48–150 bpm); a flag de movimento é derivada da variação entre amostras.
3. **Persistência**: dados são serializados com `ArduinoJson` e gravados no buffer.
4. **Modo Offline**: se a chave Wi-Fi estiver desligada, o sistema continua coletando e armazenando sem perda.
5. **Retorno da Conexão**: ao religar a conectividade, o ESP32 tenta estabelecer Wi-Fi/MQTT (quando configurado) e esvazia o buffer, publicando dados em lote.
6. **Feedback**: todas as operações logam mensagens no monitor serial (`EDGE`, `SYNC`, `WARN`, `ERR`) para auditoria e debug.

## 5. Estratégia de Resiliência
- **Capacidade dimensionada**: 5 000 amostras cobrem aproximadamente 7 horas de coleta contínua em intervalos de 5 s, atendendo a cenários de indisponibilidade moderada.
- **Integridade transacional**: buffers temporários garantem que mensagens parcialmente sincronizadas não gerem perda nem duplicidade.
- **Fallback estruturado**: leituras inválidas do DHT22 são substituídas por valores médios com ruído aleatório, evitando dados faltantes.
- **Autonomia funcional**: LED de alerta e logs operam sem depender da nuvem, alinhados ao princípio de **Edge First** para dispositivos médicos.

## 6. Governança e Segurança
- **Dados Sintéticos**: nenhuma informação pessoal real é trafegada; identificadores são pseudônimos (`paciente01`).
- **LGPD**: o dispositivo contempla estratégias de minimização (apenas sinais vitais essenciais) e trilhas de auditoria via logs.
- **Preparação para Criptografia**: o código já suporta TLS no cliente MQTT quando as credenciais forem configuradas em `secrets.h`, respeitando requisitos de confidencialidade.

## 7. Considerações Finais
Esta etapa materializa a fase “Monitoramento Contínuo – IoT no Peito do Paciente” do mapa mental CardioIA. Ao final da Parte 1 possuímos um protótipo resiliente, modular e pronto para integração com camadas superiores (Fog/Cloud), permitindo que a equipe avance para análise em tempo real, dashboards clínicos e algoritmos de previsão de eventos cardíacos. O repositório acrescenta comentários, documentação e scripts de apoio para facilitar a replicação por outros estudantes e equipes multidisciplinares.
