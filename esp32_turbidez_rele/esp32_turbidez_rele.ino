#include <WiFi.h>
#include "ESPAsyncWebServer.h"
#include <string.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <HTTPClient.h>


#define WIFI_NOME "ESP_32_PROJECT"
#define WIFI_SENHA "123456789"
#define TEMPO_ESPERA_MAXIMO_WIFI 20000
#define TEMPO_DELAY_TASK 3000

#define API_KEY "AIzaSyARa9LjX-_oE7nGRihgHg893UEw2YNAyGw"
#define DATABASE_URL "https://caixa-d-agua-esp32-default-rtdb.firebaseio.com/"  //<databaseName>.firebaseio.com or <databaseName>.<region>.firebasedatabase.app
#define FIREBASE_EMAIL "esp32@iot.com"
#define FIREBASE_SENHA "esp32iot"
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
AsyncWebServer server(80);

IPAddress local_IP(192, 168, 4, 100);
// Set your Gateway IP address
IPAddress gateway(192, 168, 1, 1);

IPAddress subnet(255, 255, 0, 0);




#define TURB 35

// GLOBAIS
int nivel_turbidez = 0;
bool estado_rele = false;
const char* serverNameRele = "http://192.168.4.1/estado_rele";


void setup() {
  Serial.begin(115200);
  pinMode(TURB, INPUT);

  conectar_wifi();

  criar_server();
}

void loop() {
  receber_nivel_turbidez();
  if (WiFi.status() == WL_CONNECTED) {


    receber_estado_rele();
    verificar_estado_rele();
  } else {
    emergencia();
    conectar_wifi();
  }
  delay(100);
}

void emergencia() {
  Serial.println("Wifi esta desconectado.");
  estado_rele = false;
}
void receber_estado_rele() {
  String input = httpGETRequest(serverNameRele);
  if (input == "0") {
    estado_rele = false;
  } else if (input == "1") {
    estado_rele = true;
  }
  Serial.println(estado_rele);
}

void verificar_estado_rele() {
  if (estado_rele) {
    Serial.println("Liga a bomba");
  } else {
    Serial.println("Desliga a bomba");
  }
}

void receber_nivel_turbidez() {
  int media_turb = 0;
  for (int i = 0; i < 100; i++) {
    media_turb += analogRead(TURB);
  }
  media_turb /= 100;

  if (media_turb >= 400 && media_turb <= 2100) {
    nivel_turbidez = map(media_turb, 400, 2100, 100, 0);
  } else if (media_turb < 400) {
    nivel_turbidez = map(400, 400, 2100, 100, 0);
  } else if (media_turb > 2100) {
    nivel_turbidez = map(2100, 400, 2100, 100, 0);
  }
}

void conectar_wifi() {
  WiFi.disconnect();
  Serial.print("Conectando-se ao WiFi");
  if (!WiFi.config(local_IP, gateway, subnet)) {
    Serial.println("STA Failed to configure");
  }
  WiFi.begin(WIFI_NOME, WIFI_SENHA);

  unsigned long TEMPO_ESPERA_ATUAL_WIFI = millis();
  // Essa função while pode ficar em loop, tentar limitar com um tempo limite
  while (WiFi.status() != WL_CONNECTED && (millis() - TEMPO_ESPERA_ATUAL_WIFI < TEMPO_ESPERA_MAXIMO_WIFI)) {
    delay(1000);
    Serial.print('.');
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Não foi possivel conectar!!");
    return;
  }
  Serial.println("Conectado!!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

void criar_server() {
  server.on("/nivel_turbidez", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", String(nivel_turbidez).c_str());
  });
  server.begin();
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;

  // Your Domain name with URL path or IP address with path
  http.begin(client, serverName);

  // Send HTTP POST request
  int httpResponseCode = http.GET();

  String payload = "--";

  if (httpResponseCode > 0) {

    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();

  return payload;
}
