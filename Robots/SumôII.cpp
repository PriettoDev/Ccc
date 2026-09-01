// ============================================================================
//  MEPER_YOKOZUNA  -  Mini Sumo 500 g / 10x10 cm
//  MCU: ESP32 | Driver: L298N | Motores: 2x N20 c/ redutor
//  Sensores: 1x Sharp GP2Y0A21 (frontal) + 2x modulo IR digital (linha)
//
//  Arquitetura: Maquina de Estados Finita (FSM) NAO BLOQUEANTE
//  Versao: 2.0
//
//  ---------------------------------------------------------------------------
//  ORDEM DE USO (nao pule etapas):
//    1) OPERATION_MODE = MODE_MOTOR_TEST   -> confere a fiacao dos motores
//    2) OPERATION_MODE = MODE_CALIBRATION  -> descobre os limiares dos sensores
//    3) OPERATION_MODE = MODE_COMBAT       -> luta
//  ---------------------------------------------------------------------------
// ============================================================================

#include <Arduino.h>

// ============================================================================
//  SECAO 1 - MODO DE OPERACAO
// ============================================================================
#define MODE_COMBAT       0
#define MODE_CALIBRATION  1
#define MODE_MOTOR_TEST   2

#define OPERATION_MODE  MODE_MOTOR_TEST   // <<< COMECE AQUI

#define DEBUG_SERIAL    1                 // 1 = imprime telemetria no Serial


// ============================================================================
//  SECAO 2 - MAPEAMENTO DE PINOS
// ============================================================================
// 'constexpr' > '#define': tem tipo, respeita escopo e o compilador
// consegue avisar quando voce usa errado. #define e substituicao cega de texto.

// ---- Motores (L298N) ----
constexpr uint8_t PIN_ENA = 25;   // PWM  motor ESQUERDO  (canal A)
constexpr uint8_t PIN_IN1 = 26;   // dir  motor ESQUERDO
constexpr uint8_t PIN_IN2 = 27;   // dir  motor ESQUERDO
constexpr uint8_t PIN_ENB = 14;   // PWM  motor DIREITO   (canal B)
constexpr uint8_t PIN_IN3 = 32;   // dir  motor DIREITO
constexpr uint8_t PIN_IN4 = 33;   // dir  motor DIREITO

// Se um motor girar ao contrario, mude aqui em vez de trocar os fios.
constexpr bool INVERT_LEFT  = false;
constexpr bool INVERT_RIGHT = false;

// ---- Sharp GP2Y0A21 frontal (unico sensor de distancia) ----
// GPIO 36 = ADC1_CH0. ADC1 e obrigatorio: o ADC2 do ESP32 fica inutilizavel
// quando o WiFi esta ligado. Voce acertou nisso no prototipo.
constexpr uint8_t PIN_SHARP_F = 36;

// ---- Sensores de linha (modulos IR digitais) ----
// ATENCAO GPIO 39: e input-only e NAO possui pull-up/pull-down interno.
// Se o modulo tiver saida open-collector, o pino fica flutuando.
// Verifique no modo CALIBRATION: se o valor oscilar sozinho, use outro
// GPIO (ex.: 19, 21, 22, 23) ou um resistor de pull-up externo de 10k.
constexpr uint8_t PIN_LINE_L = 39;
constexpr uint8_t PIN_LINE_R = 18;

// Nivel logico lido quando o sensor esta sobre a BORDA BRANCA.
// Confirme no modo CALIBRATION e ajuste aqui.
constexpr int LINE_ON_WHITE = LOW;


// ============================================================================
//  SECAO 3 - PARAMETROS DE COMPORTAMENTO  (todo ajuste fino mora aqui)
// ============================================================================

// ---- Velocidades (0 a 255) ----
constexpr int16_t SPD_ATTACK = 255;  // ataque: potencia maxima, sem meio termo
constexpr int16_t SPD_BACK   = 230;  // recuo da borda: rapido, e emergencia
constexpr int16_t SPD_TURN   = 210;  // giro apos recuo
constexpr int16_t SPD_SEARCH = 175;  // busca: mais lento = varredura mais confiavel

// ---- Tempos (ms) ----
constexpr uint32_t START_DELAY_MS  = 5000; // regra do sumo: 5 s parado
constexpr uint32_t EDGE_BACK_MS    = 220;  // duracao do recuo
constexpr uint32_t EDGE_TURN_MS    = 260;  // duracao do giro pos-recuo
constexpr uint32_t SEARCH_SPIN_MS  = 480;  // fase de giro da busca
constexpr uint32_t SEARCH_PROBE_MS = 300;  // fase de avanco da busca
constexpr uint32_t ENEMY_LATCH_MS  = 350;  // memoria quando o Sharp "cega"

// ---- Limiares do Sharp, em CONTAGENS BRUTAS DE ADC (0..4095) ----
//
// POR QUE NAO CONVERTER PARA CENTIMETROS?
// A conversao usa pow(), custa tempo, e introduz o erro do ajuste de curva.
// Para decidir "tem inimigo ou nao" voce so precisa comparar dois numeros.
// Converter para cm nao adiciona nenhuma informacao - so ruido e latencia.
// (A funcao de conversao existe la embaixo, mas so para o modo calibracao.)
//
// HISTERESE: dois limiares em vez de um. Evita que o robo fique piscando
// entre ATACAR e BUSCAR quando o inimigo esta exatamente na fronteira.
constexpr uint16_t ADC_ENEMY_ON  = 1250;  // acima disso -> detectou
constexpr uint16_t ADC_ENEMY_OFF = 1000;  // abaixo disso -> perdeu
// >>> Estes valores sao CHUTES INICIAIS. Calibre com MODE_CALIBRATION. <<<


// ============================================================================
//  SECAO 4 - CAMADA DE PWM  (compatibilidade entre versoes do core ESP32)
// ============================================================================
// O core Arduino-ESP32 mudou a API de PWM na versao 3.x. Este bloco isola
// essa diferenca: o resto do codigo chama pwmSetup()/pwmWrite() e nao precisa
// saber em qual versao esta rodando. Isso se chama "camada de abstracao".

constexpr uint32_t PWM_FREQ_HZ = 8000;  // 8 kHz: acima do audivel, mas nao tao
                                        // alto a ponto de o L298N (Darlington,
                                        // lento) perder eficiencia chaveando.
constexpr uint8_t  PWM_BITS = 8;        // 8 bits -> duty de 0 a 255
constexpr int16_t  PWM_MAX  = 255;

constexpr uint8_t CH_L = 0;   // usados apenas no core 2.x
constexpr uint8_t CH_R = 1;

static void pwmSetup(uint8_t pin, uint8_t channel) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  (void)channel;                          // (void) silencia o warning de
  ledcAttach(pin, PWM_FREQ_HZ, PWM_BITS); // "parametro nao utilizado"
#else
  ledcSetup(channel, PWM_FREQ_HZ, PWM_BITS);
  ledcAttachPin(pin, channel);
#endif
}

static void pwmWrite(uint8_t pin, uint8_t channel, uint16_t duty) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
  (void)channel;
  ledcWrite(pin, duty);
#else
  (void)pin;
  ledcWrite(channel, duty);
#endif
}


// ============================================================================
//  SECAO 5 - CONTROLE DOS MOTORES
// ============================================================================
//
// COAST vs BRAKE - a diferenca que ganha luta:
//
//   COAST (roda livre): EN = LOW. As saidas da ponte ficam em alta impedancia.
//                       O motor gira por inercia. Frenagem lenta.
//
//   BRAKE (freio):      EN = HIGH e IN1 == IN2. Os dois terminais do motor
//                       ficam curto-circuitados entre si. A energia cinetica
//                       vira corrente que se dissipa no proprio enrolamento.
//                       Frenagem quase instantanea.
//
// O prototipo usava IN1=LOW, IN2=LOW e PWM=0 -> isso e COAST, nao freio.
// A 20 cm da borda com 500 g em movimento, essa diferenca decide a luta.

enum class Stop : uint8_t { COAST, BRAKE };

static void motorWrite(uint8_t inA, uint8_t inB, uint8_t pwmPin, uint8_t ch,
                       int16_t speed, bool invert, Stop stopMode) {
  if (invert) speed = -speed;
  speed = constrain(speed, (int16_t)-PWM_MAX, (int16_t)PWM_MAX);

  if (speed > 0) {                       // frente
    digitalWrite(inA, HIGH);
    digitalWrite(inB, LOW);
    pwmWrite(pwmPin, ch, (uint16_t)speed);
  } else if (speed < 0) {                // re
    digitalWrite(inA, LOW);
    digitalWrite(inB, HIGH);
    pwmWrite(pwmPin, ch, (uint16_t)(-speed));
  } else if (stopMode == Stop::BRAKE) {  // freio ativo
    digitalWrite(inA, HIGH);
    digitalWrite(inB, HIGH);
    pwmWrite(pwmPin, ch, PWM_MAX);
  } else {                               // roda livre
    digitalWrite(inA, LOW);
    digitalWrite(inB, LOW);
    pwmWrite(pwmPin, ch, 0);
  }
}

// Interface unica de movimento. Velocidade COM SINAL: -255 (re) a +255 (frente).
// Toda a logica do robo passa a ser "quanto em cada roda", que e o modelo
// mental correto para tracao diferencial - e o que voce vai precisar quando
// implementar controle proporcional.
static void drive(int16_t left, int16_t right, Stop stopMode = Stop::BRAKE) {
  motorWrite(PIN_IN1, PIN_IN2, PIN_ENA, CH_L, left,  INVERT_LEFT,  stopMode);
  motorWrite(PIN_IN3, PIN_IN4, PIN_ENB, CH_R, right, INVERT_RIGHT, stopMode);
}


// ============================================================================
//  SECAO 6 - SENSOR DE DISTANCIA (Sharp GP2Y0A21)
// ============================================================================
//
// FILTRO DE MEDIANA AMORTIZADO:
// Guardamos as ultimas 5 leituras num buffer circular e devolvemos a mediana.
// Custa APENAS UM analogRead por iteracao do loop (nao cinco), porque o buffer
// e reaproveitado. A mediana e superior a media aqui porque ela DESCARTA
// completamente valores absurdos (spikes eletricos dos motores), enquanto a
// media apenas os dilui.

constexpr uint8_t SHARP_BUF_N = 5;
static uint16_t sharpBuf[SHARP_BUF_N] = {0};
static uint8_t  sharpIdx = 0;

static uint16_t sharpReadFiltered() {
  sharpBuf[sharpIdx] = (uint16_t)analogRead(PIN_SHARP_F);
  sharpIdx = (uint8_t)((sharpIdx + 1) % SHARP_BUF_N);

  uint16_t t[SHARP_BUF_N];
  memcpy(t, sharpBuf, sizeof(t));

  // Insertion sort: para N=5 e mais rapido que qualquer algoritmo "esperto",
  // porque nao tem overhead de recursao nem de chamada de funcao.
  for (uint8_t i = 1; i < SHARP_BUF_N; i++) {
    uint16_t key = t[i];
    int8_t j = (int8_t)i - 1;
    while (j >= 0 && t[j] > key) { t[j + 1] = t[j]; j--; }
    t[j + 1] = key;
  }
  return t[SHARP_BUF_N / 2];   // elemento do meio = mediana
}

// Conversao para centimetros - USADA SOMENTE NA CALIBRACAO.
// Curva empirica do GP2Y0A21. Vale entre ~10 e ~80 cm.
// ABAIXO DE 10 cm A LEITURA NAO E CONFIAVEL (ver comentario do estado ATTACK).
static float sharpToCm(uint16_t adc) {
  float v = adc * (3.3f / 4095.0f);
  if (v < 0.35f) return 80.0f;
  float d = 29.988f * powf(v, -1.173f);
  return (d > 80.0f) ? 80.0f : d;
}

// ---- Deteccao de inimigo com histerese + latch (trava temporal) ----
static bool     enemyVisible = false;
static uint32_t enemyLastSeenMs = 0;
static uint16_t enemyRaw = 0;

static void updateEnemy(uint32_t now) {
  enemyRaw = sharpReadFiltered();

  if (!enemyVisible) {
    if (enemyRaw >= ADC_ENEMY_ON) {
      enemyVisible = true;
      enemyLastSeenMs = now;
    }
  } else {
    if (enemyRaw >= ADC_ENEMY_OFF) {
      enemyLastSeenMs = now;                       // continua vendo
    } else if (now - enemyLastSeenMs > ENEMY_LATCH_MS) {
      enemyVisible = false;                        // perdeu de vez
    }
    // Entre esses dois casos: perdeu o sinal MAS o latch ainda segura.
    // E exatamente isso que resolve a zona cega abaixo de 10 cm.
  }
}


// ============================================================================
//  SECAO 7 - SENSORES DE LINHA
// ============================================================================
// Confirmacao por contagem: exige LINE_CONFIRM leituras iguais seguidas antes
// de aceitar a mudanca. Como o loop roda em ~1-2 ms, 2 leituras custam ~3 ms
// de latencia - irrelevante - mas eliminam o glitch eletrico que faria o robo
// dar re no meio de um ataque.

constexpr uint8_t LINE_CONFIRM = 2;

static bool    lineL = false, lineR = false;
static uint8_t cntL = 0, cntR = 0;

static void updateLineSensors() {
  bool rawL = (digitalRead(PIN_LINE_L) == LINE_ON_WHITE);
  bool rawR = (digitalRead(PIN_LINE_R) == LINE_ON_WHITE);

  if (rawL == lineL) { cntL = 0; }
  else if (++cntL >= LINE_CONFIRM) { lineL = rawL; cntL = 0; }

  if (rawR == lineR) { cntR = 0; }
  else if (++cntR >= LINE_CONFIRM) { lineR = rawR; cntR = 0; }
}


// ============================================================================
//  SECAO 8 - MAQUINA DE ESTADOS
// ============================================================================
// 'enum class' em vez de 'enum' simples: os valores ficam num escopo proprio
// (State::ATTACK) e NAO convertem implicitamente para int. Isso impede o bug
// classico de comparar um estado com um numero por acidente.

enum class State : uint8_t {
  WAIT_START,   // 5 s parado no inicio da luta (regra)
  SEARCH,       // procurando o adversario
  ATTACK,       // empurrando com tudo
  EDGE_BACK,    // recuando da borda branca
  EDGE_TURN     // girando apos o recuo
};

static State    state = State::WAIT_START;
static uint32_t stateEnteredMs = 0;

static int8_t edgeSide  = 0;   // -1 = linha na esquerda, +1 = direita, 0 = ambas
static int8_t searchDir = 1;   // +1 = gira horario, -1 = anti-horario

static void setState(State s) {
  state = s;
  stateEnteredMs = millis();
}

static inline uint32_t inStateMs() { return millis() - stateEnteredMs; }

static const char* stateName(State s) {
  switch (s) {
    case State::WAIT_START: return "WAIT";
    case State::SEARCH:     return "SEARCH";
    case State::ATTACK:     return "ATTACK";
    case State::EDGE_BACK:  return "EDGE_BACK";
    case State::EDGE_TURN:  return "EDGE_TURN";
  }
  return "?";
}


// ============================================================================
//  SECAO 9 - SETUP
// ============================================================================
void setup() {
#if DEBUG_SERIAL
  Serial.begin(115200);
  delay(300);                    // unico delay aceitavel: so no boot
  Serial.println();
  Serial.println(F("=== YOKOZUNA v2.0 ==="));
#endif

  pinMode(PIN_IN1, OUTPUT); pinMode(PIN_IN2, OUTPUT);
  pinMode(PIN_IN3, OUTPUT); pinMode(PIN_IN4, OUTPUT);
  pwmSetup(PIN_ENA, CH_L);
  pwmSetup(PIN_ENB, CH_R);

  pinMode(PIN_LINE_L, INPUT);
  pinMode(PIN_LINE_R, INPUT);

  analogReadResolution(12);                          // 0..4095
  analogSetPinAttenuation(PIN_SHARP_F, ADC_11db);    // faixa util ~0..3.1 V

  // Preenche o buffer da mediana para que a primeira decisao ja seja valida.
  for (uint8_t i = 0; i < SHARP_BUF_N; i++) sharpReadFiltered();

  drive(0, 0, Stop::COAST);
  setState(State::WAIT_START);

#if DEBUG_SERIAL
  #if   OPERATION_MODE == MODE_MOTOR_TEST
    Serial.println(F("MODO: TESTE DE MOTORES"));
  #elif OPERATION_MODE == MODE_CALIBRATION
    Serial.println(F("MODO: CALIBRACAO"));
    Serial.println(F("ADC\tcm\tL\tR"));
  #else
    Serial.println(F("MODO: COMBATE"));
    Serial.println(F("ms\tEstado\tADC\tVis\tL\tR"));
  #endif
#endif
}


// ============================================================================
//  SECAO 10 - MODOS AUXILIARES
// ============================================================================

#if OPERATION_MODE == MODE_MOTOR_TEST
// Executa uma sequencia e imprime o que DEVERIA acontecer.
// Se o comportamento fisico nao bater, ajuste INVERT_LEFT / INVERT_RIGHT
// ou reveja a fiacao IN1..IN4. NUNCA passe daqui sem isso 100% correto:
// um motor invertido faz TODA a logica de busca e de fuga da borda falhar.
static void runMotorTest() {
  struct Step { int16_t l, r; uint16_t ms; const char* label; };
  static const Step seq[] = {
    {  160,  160, 1200, "FRENTE  (as duas rodas p/ frente)" },
    {    0,    0,  600, "PARADO" },
    { -160, -160, 1200, "RE      (as duas rodas p/ tras)"   },
    {    0,    0,  600, "PARADO" },
    {  160, -160, 1200, "GIRA DIREITA (horario)"            },
    {    0,    0,  600, "PARADO" },
    { -160,  160, 1200, "GIRA ESQUERDA (anti-horario)"      },
    {    0,    0, 1500, "PARADO" },
  };
  static uint8_t  i = 0;
  static uint32_t t0 = 0;
  static bool     announced = false;

  if (!announced) {
  #if DEBUG_SERIAL
    Serial.print(F("-> ")); Serial.println(seq[i].label);
  #endif
    announced = true;
    t0 = millis();
  }
  drive(seq[i].l, seq[i].r);

  if (millis() - t0 >= seq[i].ms) {
    i = (uint8_t)((i + 1) % (sizeof(seq) / sizeof(seq[0])));
    announced = false;
  }
}
#endif


#if OPERATION_MODE == MODE_CALIBRATION
// Motores DESLIGADOS. Imprime os sensores a 10 Hz.
//
// ROTEIRO:
//  A) Sem nada na frente, dentro do dohyo -> anote o ADC. Chame de A_LIVRE.
//  B) Adversario a ~25 cm  -> anote. Chame de A_LONGE.
//  C) Adversario a ~12 cm  -> anote. Chame de A_PERTO.
//  D) Adversario a ~5 cm   -> ANOTE E OBSERVE: o valor CAI. Isso e a zona
//     cega do GP2Y0A21, nao e defeito. E o motivo do latch existir.
//  E) Passe cada IR sobre o preto e depois sobre o branco. Ajuste o trimpot
//     ate a coluna L/R mudar de forma limpa (sem oscilar).
//
//  Depois: ADC_ENEMY_ON  ~= meio do caminho entre A_LIVRE e A_LONGE
//          ADC_ENEMY_OFF ~= ADC_ENEMY_ON menos ~15%
static void runCalibration() {
  drive(0, 0, Stop::COAST);
  static uint32_t last = 0;
  if (millis() - last < 100) return;
  last = millis();

  uint16_t adc = sharpReadFiltered();
#if DEBUG_SERIAL
  Serial.print(adc);                        Serial.print('\t');
  Serial.print(sharpToCm(adc), 1);          Serial.print('\t');
  Serial.print(digitalRead(PIN_LINE_L));    Serial.print('\t');
  Serial.println(digitalRead(PIN_LINE_R));
#endif
}
#endif


// ============================================================================
//  SECAO 11 - TELEMETRIA (nao bloqueante)
// ============================================================================
#if DEBUG_SERIAL && (OPERATION_MODE == MODE_COMBAT)
static void telemetry(uint32_t now) {
  static uint32_t last = 0;
  if (now - last < 120) return;   // limita a ~8 Hz: Serial.print e LENTO
  last = now;
  Serial.print(now);              Serial.print('\t');
  Serial.print(stateName(state)); Serial.print('\t');
  Serial.print(enemyRaw);         Serial.print('\t');
  Serial.print(enemyVisible);     Serial.print('\t');
  Serial.print(lineL);            Serial.print('\t');
  Serial.println(lineR);
}
#endif


// ============================================================================
//  SECAO 12 - LOOP PRINCIPAL
// ============================================================================
//
// REGRA DE OURO: este loop NUNCA bloqueia. Nao existe delay() aqui dentro.
// Cada iteracao le todos os sensores, decide uma acao e retorna em ~1-2 ms.
// Isso significa que o robo esta "enxergando" ~600 vezes por segundo.
//
// No prototipo, um delay(500) deixava o robo cego por meio segundo. Um
// adversario a 1 m/s percorre 50 cm nesse tempo - mais de metade do dohyo.

void loop() {

#if OPERATION_MODE == MODE_MOTOR_TEST
  runMotorTest();
  return;
#elif OPERATION_MODE == MODE_CALIBRATION
  runCalibration();
  return;
#else

  const uint32_t now = millis();

  // ---- 1. Leitura de sensores (sempre, todo ciclo) ----
  updateLineSensors();
  updateEnemy(now);

  // ---- 2. Prioridade absoluta: nao cair do dohyo ----
  // Este teste vem ANTES do switch. Nao importa o que o robo esteja fazendo:
  // se viu branco, a reacao de borda interrompe tudo. Isso e uma "transicao
  // de alta prioridade", padrao classico de FSM em sistemas criticos.
  if (state != State::WAIT_START &&
      state != State::EDGE_BACK  &&
      state != State::EDGE_TURN  &&
      (lineL || lineR)) {
    edgeSide = (lineL && lineR) ? 0 : (lineL ? -1 : +1);
    setState(State::EDGE_BACK);
  }

  // ---- 3. Maquina de estados ----
  switch (state) {

    case State::WAIT_START:
      drive(0, 0, Stop::COAST);
      if (inStateMs() >= START_DELAY_MS) setState(State::SEARCH);
      break;

    case State::SEARCH: {
      if (enemyVisible) { setState(State::ATTACK); break; }

      // Padrao de varredura: gira -> avanca -> gira -> avanca...
      // O avanco entre giros faz o robo cobrir area em vez de apenas rodar
      // no proprio eixo, que e o erro mais comum em sumo iniciante.
      const uint32_t cycle = SEARCH_SPIN_MS + SEARCH_PROBE_MS;
      const uint32_t phase = inStateMs() % cycle;

      if (phase < SEARCH_SPIN_MS) {
        drive((int16_t)(SPD_SEARCH * searchDir),
              (int16_t)(-SPD_SEARCH * searchDir));
      } else {
        drive(SPD_SEARCH, SPD_SEARCH);
      }
      break;
    }

    case State::ATTACK:
      // Sem rampa de aceleracao e sem meia potencia: em sumo, empurrar e
      // uma disputa de forca. Qualquer folga de PWM e forca entregue de
      // graca para o adversario.
      //
      // O latch em updateEnemy() sustenta este estado mesmo quando o Sharp
      // perde a leitura por estar perto demais (<10 cm) - situacao em que
      // o adversario esta, justamente, colado na sua rampa.
      if (!enemyVisible) { setState(State::SEARCH); break; }
      drive(SPD_ATTACK, SPD_ATTACK);
      break;

    case State::EDGE_BACK:
      drive(-SPD_BACK, -SPD_BACK);
      if (inStateMs() >= EDGE_BACK_MS) setState(State::EDGE_TURN);
      break;

    case State::EDGE_TURN: {
      // Linha detectada a ESQUERDA -> gira para a DIREITA (e vice-versa).
      // Ambas -> meia volta (giro com o dobro do tempo).
      if      (edgeSide < 0) drive(SPD_TURN, -SPD_TURN);
      else if (edgeSide > 0) drive(-SPD_TURN, SPD_TURN);
      else                   drive(SPD_TURN, -SPD_TURN);

      const uint32_t dur = (edgeSide == 0) ? (EDGE_TURN_MS * 2) : EDGE_TURN_MS;
      if (inStateMs() >= dur) {
        searchDir = (int8_t)(-searchDir);   // alterna o sentido da busca
        setState(State::SEARCH);
      }
      break;
    }
  }

  // ---- 4. Telemetria ----
  #if DEBUG_SERIAL
    telemetry(now);
  #endif

#endif  // OPERATION_MODE
}