#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Preferences.h>

// Pin definition for ESP32-C3 Super Mini (adjust as needed)
#define PIN_MOTOR_IN1 3
#define PIN_MOTOR_IN2 4
#define PIN_MOTOR_PWM 5
#define PIN_HEATER    6
#define PIN_BUTTON    7

WebServer server(80);
Preferences prefs;

String adminPass = "admin";
int autoOffMinutes = 10;
bool session = false;

unsigned long lastAction = 0;
const int SPEED_VALUES[4] = {0, 85, 170, 255};
bool motorForward = true;
int speedIndex = 0;
bool heaterOn = false;

void savePrefs() {
  prefs.putString("pass", adminPass);
  prefs.putInt("autooff", autoOffMinutes);
}

void setMotor(bool forward, int idx) {
  digitalWrite(PIN_MOTOR_IN1, forward ? HIGH : LOW);
  digitalWrite(PIN_MOTOR_IN2, forward ? LOW : HIGH);
  analogWrite(PIN_MOTOR_PWM, SPEED_VALUES[idx]);
}

void goToSleep() {
  setMotor(true, 0);
  digitalWrite(PIN_HEATER, LOW);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BUTTON, 0);
  esp_deep_sleep_start();
}

String style() {
  return F("<style>body{font-family:sans-serif;background:#e0f2f1;color:#004d40;}\n"
           "input,button{padding:8px;margin:4px;border-radius:4px;border:1px solid #1abc9c;}\n"
           "h1{color:#1abc9c;}\n"\
           "</style>");
}

String loginPage() {
  return style() + F("<h1>Login</h1><form method='POST' action='/login'>"
                     "<input type='password' name='pass' placeholder='Password'><br>"
                     "<button type='submit'>Masuk</button></form>");
}

String controlPage() {
  String html = style();
  html += F("<h1>Massager</h1>");
  html += "<p>Motor: <form method='POST' action='/motor'><button name='dir' value='f'>Forward</button><button name='dir' value='b'>Reverse</button><br>";
  html += "<input type='number' name='spd' min='0' max='3' value='" + String(speedIndex) + "'>"
          "<button type='submit'>Set Speed</button></form></p>";
  html += "<p>Heater: <form method='POST' action='/heater'><button name='h' value='1'>ON</button><button name='h' value='0'>OFF</button></form></p>";
  html += "<p>Auto Off (minutes):<form method='POST' action='/autooff'><input type='number' name='m' value='" + String(autoOffMinutes) + "'>";
  html += "<button type='submit'>Save</button></form></p>";
  html += "<p><form method='POST' action='/wifi'><input name='ssid' placeholder='SSID'><br><input name='pwd' placeholder='WiFi Password'><br><button type='submit'>Save WiFi</button></form></p>";
  html += "<p><form method='POST' action='/changepass'><input type='password' name='np' placeholder='New Password'><br><button type='submit'>Change Password</button></form></p>";
  html += "<p><form method='POST' enctype='multipart/form-data' action='/update'><input type='file' name='update'><button type='submit'>OTA Update</button></form></p>";
  html += "<p><form method='POST' action='/logout'><button type='submit'>Logout</button></form></p>";
  html += "<p><form method='POST' action='/poweroff'><button type='submit'>Power Off</button></form></p>";
  return html;
}

void handleRoot() {
  if (!session) {
    server.send(200, "text/html", loginPage());
    return;
  }
  server.send(200, "text/html", controlPage());
}

void handleLogin() {
  if (server.arg("pass") == adminPass) {
    session = true;
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
    lastAction = millis();
  } else {
    server.send(403, "text/plain", "Wrong password");
  }
}

void handleLogout() {
  session = false;
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleChangePass() {
  if (server.arg("np").length() > 0) {
    adminPass = server.arg("np");
    savePrefs();
  }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleWifi() {
  String ssid = server.arg("ssid");
  String pwd = server.arg("pwd");
  prefs.putString("ssid", ssid);
  prefs.putString("pwd", pwd);
  server.send(200, "text/plain", "WiFi saved, reboot to apply");
}

void handleMotor() {
  motorForward = server.arg("dir") != "b";
  int spd = server.arg("spd").toInt();
  if (spd < 0) spd = 0;
  if (spd > 3) spd = 3;
  speedIndex = spd;
  setMotor(motorForward, speedIndex);
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
  lastAction = millis();
}

void handleHeater() {
  heaterOn = server.arg("h") == "1";
  digitalWrite(PIN_HEATER, heaterOn ? HIGH : LOW);
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
  lastAction = millis();
}

void handleAutoOff() {
  autoOffMinutes = server.arg("m").toInt();
  if (autoOffMinutes < 1) autoOffMinutes = 1;
  savePrefs();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleOTA() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Update.begin();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Update.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    Update.end(true);
  }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handlePowerOff() {
  server.send(200, "text/plain", "Going to sleep");
  delay(100);
  goToSleep();
}

void notFound() {
  server.send(404, "text/plain", "Not Found");
}

void setup() {
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_MOTOR_PWM, OUTPUT);
  pinMode(PIN_HEATER, OUTPUT);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  prefs.begin("massager", false);
  adminPass = prefs.getString("pass", "admin");
  autoOffMinutes = prefs.getInt("autooff", 10);

  String ssid = prefs.getString("ssid", "");
  String pwd = prefs.getString("pwd", "");

  if (ssid.length() > 0) {
    WiFi.begin(ssid.c_str(), pwd.c_str());
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      WiFi.mode(WIFI_AP);
      WiFi.softAP("MassagerSetup");
    }
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("MassagerSetup");
  }

  server.on("/", HTTP_GET, handleRoot);
  server.on("/login", HTTP_POST, handleLogin);
  server.on("/logout", HTTP_POST, handleLogout);
  server.on("/changepass", HTTP_POST, handleChangePass);
  server.on("/wifi", HTTP_POST, handleWifi);
  server.on("/motor", HTTP_POST, handleMotor);
  server.on("/heater", HTTP_POST, handleHeater);
  server.on("/autooff", HTTP_POST, handleAutoOff);
  server.on("/update", HTTP_POST, []() { server.send(200, "text/plain", "OTA OK"); }, handleOTA);
  server.on("/poweroff", HTTP_POST, handlePowerOff);
  server.onNotFound(notFound);

  server.begin();
  lastAction = millis();
}

void loop() {
  server.handleClient();
  if (digitalRead(PIN_BUTTON) == LOW) {
    delay(50);
    if (digitalRead(PIN_BUTTON) == LOW) {
      goToSleep();
    }
  }
  if (millis() - lastAction > (unsigned long)autoOffMinutes * 60000UL) {
    goToSleep();
  }
}

