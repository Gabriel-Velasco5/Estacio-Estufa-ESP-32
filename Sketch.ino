#include <WiFi.h>
#include <WebServer.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP32Servo.h>

// ==== CONFIGURAÇÕES GERAIS ====
// Definição dos pinos e tipos de sensores
#define DHTPIN 4
#define DHTTYPE DHT22
#define LDR_PIN 34
#define SERVO_TEMP 23
#define SERVO_UMID 19
#define SERVO_LUZ 18

// ==== OBJETOS ====
// Inicializa os objetos necessários
DHT dht(DHTPIN, DHTTYPE);
LiquidCrystal_I2C lcd(0x27, 20, 4);
Servo servoTemp, servoUmid, servoLuz;
WebServer server(80);

// ==== VARIÁVEIS ====
// Valores iniciais simulados dos sensores
float temperatura = 28.0;
float umidade = 60.0;
int luminosidade = 50000;

// Valor inicial para indicar se cada parâmetro está sendo estabilizado
bool estabilizandoTemp = false;
bool estabilizandoUmid = false;
bool estabilizandoLuz = false;

// ==== LIMITES ====
// Faixas ideais de operação da estufa
const float minTemp = 18.0, maxTemp = 28.0;
const float minUmid = 50.0, maxUmid = 70.0;
const int minLuz = 30000, maxLuz = 70000;

// ==== WI-FI ====
// Configuração da rede Wi-Fi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// ==== HTML ====
String htmlPage() {
  String page = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Estufa Automatizada</title>
  <style>
    body {
      margin: 0;
      font-family: Arial, sans-serif;
      background: linear-gradient(180deg, #38ff59, #4fa1ff);
      color: #222;
      text-align: center;
      min-height: 100vh;
      display: flex;
      flex-direction: column;
      justify-content: space-between;
    }
    header {
      background-color: #2ecc71;
      color: white;
      padding: 20px 0;
      box-shadow: 0 2px 5px rgba(0,0,0,0.1);
    }
    header h1 {
      margin: 0;
      font-size: 1.8rem;
      letter-spacing: 1px;
      text-shadow: 5px 5px 3px #000000;
    }
    main {
      flex: 1;
      display: flex;
      flex-direction: column;
      justify-content: center;
      align-items: center;
      gap: 25px;
    }
    .valor {
      background: white;
      border-radius: 15px;
      box-shadow: 0 2px 6px rgba(0,0,0,0.15);
      width: 250px;
      padding: 15px 0;
      font-size: 1.2rem;
      font-weight: bold;
    }
    .valor label {
      display: block;
      margin-bottom: 8px;
    }
    .valor input {
      width: 80%;
      padding: 6px;
      border-radius: 8px;
      border: 1px solid #ccc;
      font-size: 1rem;
      text-align: center;
    }
    .btn-estabilizar {
      margin-top: 10px;
      padding: 6px 12px;
      border: none;
      border-radius: 8px;
      background-color: #2ecc71;
      color: white;
      font-weight: bold;
      cursor: pointer;
      transition: 0.3s;
    }
    .btn-estabilizar:hover {
      background-color: #27ae60;
    }
    p {
      color: white;
      font-weight: bold;
      font-size: 40px;
      margin-bottom: 10px;
      text-shadow: 5px 5px 3px rgb(0, 0, 0);
    }
    footer {
      background-color: #4fa1ff;
      color: white;
      padding: 20px 0;
      font-size: 1rem;
      box-shadow: 0 -2px 5px rgba(0,0,0,0.15);
    }
    table {
      margin: 0 auto;
      border-collapse: collapse;
      width: 85%;
      background: rgba(255, 255, 255, 0.15);
      border-radius: 12px;
      overflow: hidden;
      box-shadow: 0 2px 5px rgba(0,0,0,0.2);
    }
    td {
      padding: 10px 15px;
      border: 1px solid rgba(255,255,255,0.3);
      font-weight: bold;
      color: #fff;
    }
  </style>
</head>
<body>
  <header>
    <h1>Estufa Automatizada</h1>
  </header>

  <main>
    <div class="valor">
      <label for="luminosidade">Luminosidade:</label>
      <div id="luxValue">--</div>
      <input type="number" id="luxInput" placeholder="Digite e pressione Enter" />
      <button class="btn-estabilizar" onclick="estabilizar('luz')">Estabilizar</button>
    </div>

    <div class="valor">
      <label for="temperatura">Temperatura (°C):</label>
      <div id="tempValue">-- °C</div>
      <input type="number" id="tempInput" placeholder="Digite e pressione Enter" />
      <button class="btn-estabilizar" onclick="estabilizar('temp')">Estabilizar</button>
    </div>

    <div class="valor">
      <label for="umidade">Umidade (%):</label>
      <div id="humValue">-- %</div>
      <input type="number" id="humInput" placeholder="Digite e pressione Enter" />
      <button class="btn-estabilizar" onclick="estabilizar('umid')">Estabilizar</button>
    </div>
  </main>

  <p>Valores de Referência</p>

  <footer>
    <table>
      <tr>
        <td>🌡️ Temperatura: 18 - 28°C</td>
        <td>💧 Umidade: 50 - 70%</td>
        <td>☀️ Luminosidade: 30.000 - 70.000 Lux</td>
      </tr>
    </table>
  </footer>

<script>
async function fetchValues(force=false) {
  try {
    const [t, h, l] = await Promise.all([
      fetch('/temp').then(r=>r.text()),
      fetch('/hum').then(r=>r.text()),
      fetch('/lux').then(r=>r.text())
    ]);
    document.getElementById('tempValue').textContent = parseFloat(t).toFixed(1) + ' °C';
    document.getElementById('humValue').textContent  = parseFloat(h).toFixed(1) + ' %';
    document.getElementById('luxValue').textContent  = l;
    if(force){
      document.getElementById('tempInput').value=t;
      document.getElementById('humInput').value=h;
      document.getElementById('luxInput').value=l;
    }
  } catch(e){ console.error(e); }
}

function setInputParam(key, val){
  if(val==='') return;
  fetch(`/set?${key}=${val}`).then(()=>fetchValues(true));
}

function estabilizar(tipo){
  fetch(`/estabilizar?${tipo}=true`);
}

document.getElementById('tempInput').addEventListener('keydown', e=>{
  if(e.key==='Enter') setInputParam('temp', e.target.value);
});
document.getElementById('humInput').addEventListener('keydown', e=>{
  if(e.key==='Enter') setInputParam('hum', e.target.value);
});
document.getElementById('luxInput').addEventListener('keydown', e=>{
  if(e.key==='Enter') setInputParam('lux', e.target.value);
});

fetchValues(true);
setInterval(fetchValues, 2000);
</script>
</body>
</html>
)rawliteral";
  return page;
}

// ==== FUNÇÕES ====

// Atualiza o display LCD com os valores dos sensores
void atualizarLCD() {
  static float lastTemp = -1, lastUmid = -1;
  static int lastLuz = -1;

  // Só atualiza o LCD se os valores mudarem
  if (temperatura != lastTemp || umidade != lastUmid || luminosidade != lastLuz) {
    lcd.clear();
    lcd.setCursor(0, 0); lcd.print("Temp: "); lcd.print(temperatura, 1); lcd.print("C");
    lcd.setCursor(0, 1); lcd.print("Umid: "); lcd.print(umidade, 1); lcd.print("%");
    lcd.setCursor(0, 2); lcd.print("Luz: "); lcd.print(luminosidade);
    lastTemp = temperatura; lastUmid = umidade; lastLuz = luminosidade;
  }
}

// Faz o valor voltar para a média após estabilização
void estabilizarParaMedia(float &valor, float min, float max) {
  float media = (min + max) / 2.0;
  if (valor < media) valor += 0.3;
  else if (valor > media) valor -= 0.3;
}
  // Controle de todos os servos durante estabilização
void controlarServos() {
    // Temperatura
  if (estabilizandoTemp) {
    if (temperatura < minTemp || temperatura > maxTemp) {
      servoTemp.write(180);
      temperatura += (temperatura < minTemp) ? 0.5 : -0.5;
    } else {
      servoTemp.write(0);
      estabilizandoTemp = false;
      estabilizarParaMedia(temperatura, minTemp, maxTemp);
    }
  }

  // Umidade
  if (estabilizandoUmid) {
    if (umidade < minUmid || umidade > maxUmid) {
      servoUmid.write(180);
      umidade += (umidade < minUmid) ? 1 : -1;
    } else {
      servoUmid.write(0);
      estabilizandoUmid = false;
      estabilizarParaMedia(umidade, minUmid, maxUmid);
    }
  }

  // Luminosidade
  if (estabilizandoLuz) {
    if (luminosidade < minLuz || luminosidade > maxLuz) {
      servoLuz.write(180);
      luminosidade += (luminosidade < minLuz) ? 2000 : -2000;
    } else {
      servoLuz.write(0);
      estabilizandoLuz = false;
      int mediaL = (minLuz + maxLuz) / 2;
      if (luminosidade < mediaL) luminosidade += 2000;
      else if (luminosidade > mediaL) luminosidade -= 2000;
    }
  }
}

// ==== ROTAS DO SERVIDOR ====
// Página principal
void handleRoot() { server.send(200, "text/html", htmlPage()); }

// Define novos valores manualmente
void handleSet() {
  if (server.hasArg("temp")) temperatura = server.arg("temp").toFloat();
  if (server.hasArg("hum")) umidade = server.arg("hum").toFloat();
  if (server.hasArg("lux")) luminosidade = server.arg("lux").toInt();
  server.send(200, "text/plain", "OK");
}

// Aciona estabilização da Temperatura/Umidade/Luminosidade
void handleEstabilizar() {
  if (server.hasArg("temp")) estabilizandoTemp = true;
  if (server.hasArg("umid")) estabilizandoUmid = true;
  if (server.hasArg("luz")) estabilizandoLuz = true;
  server.send(200, "text/plain", "OK");
}

// ==== CONFIGURAÇÃO INICIAL ====
void setup() {
  Serial.begin(115200);
  dht.begin();
  lcd.init();
  lcd.backlight();

  servoTemp.attach(SERVO_TEMP);
  servoUmid.attach(SERVO_UMID);
  servoLuz.attach(SERVO_LUZ);

  servoTemp.write(0);
  servoUmid.write(0);
  servoLuz.write(0);

  WiFi.begin(ssid, password);
  Serial.print("Conectando ao Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\nConectado: " + WiFi.localIP().toString());

// Rotas do servidor
  server.on("/", handleRoot);
  server.on("/set", handleSet);
  server.on("/estabilizar", handleEstabilizar);
  server.on("/temp", [](){ server.send(200, "text/plain", String(temperatura, 1)); });
  server.on("/hum", [](){ server.send(200, "text/plain", String(umidade, 1)); });
  server.on("/lux", [](){ server.send(200, "text/plain", String(luminosidade)); });

  server.begin();
}

// ==== LOOP PRINCIPAL ====
void loop() {
  controlarServos();
  atualizarLCD();
  server.handleClient();
  delay(500);
}
