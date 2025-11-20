#include <WiFi.h>
#include <Wire.h>
#include <TinyGPS++.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_HMC5883_U.h>
#include <Stepper.h>

const char* ssid = "murilinho";
const char* password = "senhajoia";
WiFiServer server(80);

const int PINO_MOTOR_1 = 19;
const int PINO_MOTOR_2 = 18;
const int PINO_MOTOR_3 = 5;
const int PINO_MOTOR_4 = 23;
const int PASSOS_POR_VOLTA = 2048;

TinyGPSPlus gps;
Adafruit_HMC5883_Unified bussola = Adafruit_HMC5883_Unified(12345);
Stepper motor(PASSOS_POR_VOLTA, PINO_MOTOR_1, PINO_MOTOR_3, PINO_MOTOR_2, PINO_MOTOR_4);

float lat_atual = -25.4284;
float lon_atual = -49.2733;
bool gps_ok = false;

float direcao_atual = 0.0;
float angulo_alvo = 0.0;
float angulo_motor = 0.0;

float declinacao = -21.0;
float offset_x = -20.185;
float offset_y = 4.18;

const float alpha = 0.3;
static float smooth_x = -1.0;
static float smooth_y = -1.0;

float lat_alvo = 0.0;
float lon_alvo = 0.0;
bool alvo_ok = false;

bool motor_esta_girando = false;

unsigned long t_bussola_leitura = 0;
int passo_atual = 0;
int passo_alvo = 0;

// --- Buffer de char para conversao de float (para economizar RAM) ---
char char_buffer[12];

void setup() {
  Serial.begin(115200);

  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  Serial.println("Porta Serial 2 (GPS) iniciada.");

  Wire.begin(21, 22);
  if (!bussola.begin()) {
    Serial.println("Erro ao encontrar sensor HMC5883L! Verifique as conexoes.");
    while (1);
  }
  Serial.println("Bussola (HMC5883L) iniciada.");

  motor.setSpeed(5);
  Serial.println("Motor de Passo iniciado.");

  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi conectado!");
  server.begin();
  Serial.println("Servidor iniciado!");
  Serial.print("Acesse a pagina em: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  ler_gps();

  if (!motor_esta_girando) {
    ler_bussola();
  }
  
  apontar_motor();
  servidor_web();
}

void ler_gps() {
  while (Serial2.available() > 0) {
    if (gps.encode(Serial2.read())) {
      if (gps.location.isValid()) {
        lat_atual = gps.location.lat();
        lon_atual = gps.location.lng();
        gps_ok = true;
      } else {
        gps_ok = false;
      }
    }
  }
}

void ler_bussola() {
  if (millis() - t_bussola_leitura > 100) {
    t_bussola_leitura = millis();
    sensors_event_t event;
    bussola.getEvent(&event);

    float x_calibrado = event.magnetic.x - offset_x;
    float y_calibrado = event.magnetic.y - offset_y;

    static bool filtro_iniciado = false;
    if (!filtro_iniciado) {
      smooth_x = x_calibrado;
      smooth_y = y_calibrado;
      filtro_iniciado = true;
    }
    
    smooth_x = (x_calibrado * alpha) + (smooth_x * (1.0 - alpha));
    smooth_y = (y_calibrado * alpha) + (smooth_y * (1.0 - alpha));
    
    float heading_rad = atan2(smooth_y, smooth_x);

    heading_rad += declinacao * M_PI / 180.0;

    if (heading_rad < 0)    heading_rad += 2 * M_PI;
    if (heading_rad > 2 * PI) heading_rad -= 2 * M_PI;

    direcao_atual = heading_rad * 180 / M_PI;
  }
}

void apontar_motor() {
  
  if (gps_ok && alvo_ok && !motor_esta_girando) {
    angulo_alvo = calcular_angulo(lat_atual, lon_atual, lat_alvo, lon_alvo);
    angulo_motor = angulo_alvo - direcao_atual;
    angulo_motor = fmod(angulo_motor + 360.0, 360.0);
    passo_alvo = (int)(angulo_motor / 360.0 * PASSOS_POR_VOLTA);
  } else if (!gps_ok || !alvo_ok) {
    passo_alvo = passo_atual;
  }

  int diff = passo_alvo - passo_atual;
  
  if (diff == 0) {
    motor_esta_girando = false;
    return;
  }

  motor_esta_girando = true;

  if (abs(diff) > (PASSOS_POR_VOLTA / 2)) {
    if (diff > 0) {
      motor.step(-1);
      passo_atual--;
    } else {
      motor.step(1);
      passo_atual++;
    }
  } else {
    if (diff > 0) {
      motor.step(1);
      passo_atual++;
    } else {
      motor.step(-1);
      passo_atual--;
    }
  }

  if (passo_atual >= PASSOS_POR_VOLTA) {
    passo_atual -= PASSOS_POR_VOLTA;
  }
  if (passo_atual < 0) {
    passo_atual += PASSOS_POR_VOLTA;
  }
}

void servidor_web() {
  WiFiClient client = server.available();
  if (client) {
    String currentLine = "";
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        if (c == '\n') {
          if (currentLine.startsWith("GET /settarget")) {
            Serial.println(">>> Recebendo novo alvo!");
            
            int lat_index = currentLine.indexOf("lat=") + 4;
            int lng_index = currentLine.indexOf("&lng=") + 5;
            int http_index = currentLine.indexOf(" HTTP");
            
            String lat_str = currentLine.substring(lat_index, lng_index - 5);
            String lng_str = currentLine.substring(lng_index, http_index);
            
            lat_alvo = lat_str.toFloat();
            lon_alvo = lng_str.toFloat();
            alvo_ok = true;
            
            Serial.print("  Lat Alvo: "); Serial.println(lat_alvo, 6);
            Serial.print("  Lon Alvo: "); Serial.println(lon_alvo, 6);
            
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/plain");
            client.println();
            client.println("Alvo recebido!");
            break;
            
          } else if (currentLine.length() == 0) {
            
            client.println(F("HTTP/1.1 200 OK"));
            client.println(F("Content-type:text/html; charset=UTF-8"));
            client.println(F("Connection: close"));
            client.println();

            client.print(F("<!DOCTYPE html><html lang=\"pt-br\"><head>"));
            client.print(F("<meta charset=\"UTF-8\">"));
            client.print(F("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"));
            client.print(F("<title>Projeto Waypoint 360</title>"));
            client.print(F("<link href=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/css/bootstrap.min.css\" rel=\"stylesheet\">"));
            client.print(F("<link rel=\"stylesheet\" href=\"https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.3/font/bootstrap-icons.min.css\">"));
            client.print(F("<link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.css\" />"));
            client.print(F("<script src=\"https://unpkg.com/leaflet@1.9.4/dist/leaflet.js\"></script>"));
            client.print(F("<link rel=\"stylesheet\" href=\"https://unpkg.com/leaflet-control-geocoder/dist/Control.Geocoder.css\" />"));
            client.print(F("<script src=\"https://unpkg.com/leaflet-control-geocoder/dist/Control.Geocoder.js\"></script>"));
            client.print(F("<style>"));
            client.print(F("html, body { height: 100%; margin: 0; }"));
            client.print(F("body { display: flex; flex-direction: column; }"));
            client.print(F("main { flex-grow: 1; position: relative; }"));
            client.print(F("#map { height: 100%; width: 100%; }"));
            client.print(F(".leaflet-control-container { z-index: 1000; }"));
            client.print(F(".leaflet-control-geocoder-form input { padding: 0.375rem 0.75rem; border-radius: 0.375rem; border: 1px solid #ced4da; box-shadow: 0 1px 2px 0 rgba(0, 0, 0, 0.05); font-size: 1rem; width: 280px; }"));
            client.print(F(".leaflet-control-geocoder-icon { background-image: url('data:image/svg+xml;charset=UTF-8,<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 20 20\" fill=\"%236B7280\"><path fill-rule=\"evenodd\" d=\"M9 3.5a5.5 5.5 0 100 11 5.5 5.5 0 000-11zM2 9a7 7 0 1112.452 4.391l3.328 3.329a.75.75 0 11-1.06 1.06l-3.329-3.328A7 7 0 012 9z\" clip-rule=\"evenodd\" /></svg>'); }"));
            client.print(F("</style></head>"));
            client.print(F("<body class=\"d-flex flex-column vh-100\">"));
            client.print(F("<header class=\"flex-shrink-0 shadow-sm bg-white\">"));
            client.print(F("<div class=\"container-fluid p-3 text-center\">"));
            client.print(F("<h1 class=\"h3 mb-0 text-dark\">Projeto Waypoint 360</h1>"));
            client.print(F("</div>"));
            client.print(F("<div class=\"container-fluid px-3 pb-3\"><div class=\"row g-3\">"));
            
            client.print(F("<div class=\"col-12 col-md-4\">"));
            client.print(F("<div class=\"card h-100 border-success-subtle bg-success-subtle\">"));
            client.print(F("<div class=\"card-body\"><div class=\"d-flex align-items-center\">"));
            client.print(F("<i class=\"bi bi-broadcast fs-3 me-3 text-success-emphasis\"></i>"));
            client.print(F("<div><h6 class=\"card-subtitle mb-2 text-muted text-uppercase\">Minha Posição (GPS)</h6>"));
            if (gps_ok) {
              client.print(F("<p id=\"gps-status\" class=\"card-text fs-5 fw-medium text-success-emphasis\">Sinal OK</p>"));
              client.print(F("<p id=\"gps-coords\" class=\"card-text text-secondary mb-0\">Lat: "));
              dtostrf(lat_atual, 1, 6, char_buffer); // Converte float para char[]
              client.print(char_buffer);
              client.print(F(", Lon: "));
              dtostrf(lon_atual, 1, 6, char_buffer); // Converte float para char[]
              client.print(char_buffer);
              client.print(F("</p>"));
            } else {
              client.print(F("<p id=\"gps-status\" class=\"card-text fs-5 fw-medium text-success-emphasis\">Procurando sinal...</p>"));
              client.print(F("<p id=\"gps-coords\" class=\"card-text text-secondary mb-0\">Lat: --, Lon: --</p>"));
            }
            client.print(F("</div></div></div></div></div>"));

            client.print(F("<div class=\"col-12 col-md-4\">"));
            client.print(F("<div class=\"card h-100 border-primary-subtle bg-primary-subtle\">"));
            client.print(F("<div class=\"card-body\"><div class=\"d-flex align-items-center\">"));
            client.print(F("<i class=\"bi bi-crosshair fs-3 me-3 text-primary-emphasis\"></i>"));
            client.print(F("<div><h6 class=\"card-subtitle mb-2 text-muted text-uppercase\">Alvo Definido</h6>"));
            if (alvo_ok) {
              client.print(F("<p id=\"target-status\" class=\"card-text fs-5 fw-medium text-primary-emphasis\">Alvo Definido!</p>"));
              client.print(F("<p id=\"target-coords\" class=\"card-text text-secondary mb-0\">Lat: "));
              dtostrf(lat_alvo, 1, 6, char_buffer); // Converte float para char[]
              client.print(char_buffer);
              client.print(F(", Lon: "));
              dtostrf(lon_alvo, 1, 6, char_buffer); // Converte float para char[]
              client.print(char_buffer);
              client.print(F("</p>"));
            } else {
              client.print(F("<p id=\"target-status\" class=\"card-text fs-5 fw-medium text-primary-emphasis\">Nenhum alvo definido</p>"));
              client.print(F("<p id=\"target-coords\" class=\"card-text text-secondary mb-0\">Lat: --, Lon: --</p>"));
            }
            client.print(F("</div></div></div></div></div>"));

            client.print(F("<div class=\"col-12 col-md-4\">"));
            client.print(F("<div class=\"card h-100 border-secondary-subtle bg-secondary-subtle\">"));
            client.print(F("<div class=\"card-body\"><div class=\"d-flex align-items-center\">"));
            client.print(F("<i class=\"bi bi-bug fs-3 me-3 text-dark-emphasis\"></i>"));
            client.print(F("<div><h6 class=\"card-subtitle mb-2 text-muted text-uppercase\">Debug da Bússola</h6>"));
            client.print(F("<p id=\"debug-heading\" class=\"card-text text-dark-emphasis mb-0\">Direção: "));
            dtostrf(direcao_atual, 1, 2, char_buffer); // Converte float para char[]
            client.print(char_buffer);
            client.print(F("°</p>"));
            client.print(F("<p id=\"debug-bearing\" class=\"card-text text-dark-emphasis mb-0\">Ângulo Alvo: "));
            dtostrf(angulo_alvo, 1, 2, char_buffer); // Converte float para char[]
            client.print(char_buffer);
            client.print(F("°</p>"));
            client.print(F("<p id=\"debug-pointer\" class=\"card-text fw-bold text-dark-emphasis mb-0\">Ângulo Ponteiro: "));
            dtostrf(angulo_motor, 1, 2, char_buffer); // Converte float para char[]
            client.print(char_buffer);
            client.print(F("°</p>"));
            client.print(F("</div></div></div></div></div>"));
            
            client.print(F("</div></div></header>"));
            client.print(F("<main><div id=\"map\"></div></main>"));
            client.print(F("<script src=\"https://cdn.jsdelivr.net/npm/bootstrap@5.3.3/dist/js/bootstrap.bundle.min.js\"></script>"));
            client.print(F("<script>"));
            client.print(F("document.addEventListener('DOMContentLoaded', function () {"));
            
            client.print(F("const initialLat = "));
            dtostrf(lat_atual, 1, 6, char_buffer); // Converte float para char[]
            client.print(char_buffer);
            client.print(F(";"));
            client.print(F("const initialLon = "));
            dtostrf(lon_atual, 1, 6, char_buffer); // Converte float para char[]
            client.print(char_buffer);
            client.print(F(";"));
            
            client.print(F("var map = L.map('map').setView([initialLat, initialLon], 15);"));
            client.print(F("L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png', { attribution: '© <a href=\"https://www.openstreetmap.org/copyright\">OpenStreetMap</a> contributors' }).addTo(map);"));
            client.print(F("var userMarker = L.marker([initialLat, initialLon]).addTo(map).bindPopup('Posição atual do dispositivo.');"));
            
            if (alvo_ok) {
              client.print(F("var targetMarker = L.marker(["));
              dtostrf(lat_alvo, 1, 6, char_buffer); // Converte float para char[]
              client.print(char_buffer);
              client.print(F(", "));
              dtostrf(lon_alvo, 1, 6, char_buffer); // Converte float para char[]
              client.print(char_buffer);
              client.print(F("]).addTo(map).bindPopup('ALVO ATUAL');"));
            }

            client.print(F("setTimeout(function () { map.invalidateSize(); }, 100);"));
            client.print(F("var geocoder = L.Control.geocoder({ defaultMarkGeocode: false, placeholder: 'Buscar endereço ou local...', geocoder: L.Control.Geocoder.photon() }).on('markgeocode', function(e) {"));
            client.print(F("var lat = e.geocode.center.lat; var lng = e.geocode.center.lng; var name = e.geocode.name;"));
            client.print(F("if (confirm('Definir \"' + name + '\" como novo alvo?')) { sendTarget(lat, lng); }"));
            client.print(F("}).addTo(map);"));
            client.print(F("map.on('click', function(e) {"));
            client.print(F("var lat = e.latlng.lat; var lng = e.latlng.lng;"));
            client.print(F("if (confirm('Definir este ponto como novo alvo?\\nLat: ' + lat.toFixed(6) + '\\nLon: ' + lng.toFixed(6))) { sendTarget(lat, lng); }"));
            client.print(F("});"));
            client.print(F("function sendTarget(lat, lng) {"));
            client.print(F("document.getElementById('target-status').innerText = 'Enviando novo alvo...';"));
            client.print(F("document.getElementById('target-coords').innerText = 'Lat: ' + lat.toFixed(6) + ', Lon: ' + lng.toFixed(6);"));
            client.print(F("fetch('/settarget?lat=' + lat + '&lng=' + lng)"));
            client.print(F(".then(response => response.text())"));
            client.print(F(".then(data => { alert('Resposta do ESP32: ' + data); location.reload(); })"));
            client.print(F(".catch(error => { console.error('Erro ao enviar alvo:', error); alert('Erro ao conectar com o ESP32.'); });"));
            client.print(F("}"));
            client.print(F("});</script></body></html>"));
            
            break;
            
          } else {
            currentLine = "";
          }
        } else if (c != '\r') {
          currentLine += c;
        }
      }
    }
    client.stop();
  }
}

double calcular_angulo(double lat1, double lon1, double lat2, double lon2) {
  lat1 = radians(lat1);
  lon1 = radians(lon1);
  lat2 = radians(lat2);
  lon2 = radians(lon2);

  double dLon = (lon2 - lon1);
  double y = sin(dLon) * cos(lat2);
  double x = cos(lat1) * sin(lat2) - sin(lat1) * cos(lat2) * cos(dLon);
  double brng = atan2(y, x);

  brng = degrees(brng);
  
  brng = fmod(brng + 360.0, 360.0);
  return brng;
}