#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <Preferences.h>


// Pin definition for ESP32-C3 Super Mini (L298N tanpa ENA/ENB)
#define PIN_MOTOR_IN1 3
#define PIN_MOTOR_IN2 4
#define PIN_HEATER    5
#define PIN_BTN_POWER 6
#define PIN_BTN_SPEED 7
#define PIN_BTN_HEAT  8


WebServer server(80);
Preferences prefs;

String adminPass = "admin";
int autoOffMinutes = 10;
bool session = false;


String wifiSSID = "";
String wifiPwd  = "";

unsigned long lastAction = 0;
// Motor speed values: 100%, 75%, 50%, 25%
const int SPEED_VALUES[4] = {255, 191, 128, 64};
bool motorForward = true;
int speedIndex = 0;
bool heaterOn = false;
unsigned long powerPressStart = 0;
bool lastSpeedState = HIGH;
bool lastHeaterState = HIGH;


void savePrefs() {
  prefs.putString("pass", adminPass);
  prefs.putInt("autooff", autoOffMinutes);
}

void setMotor(bool forward, int idx) {

  int pwm = SPEED_VALUES[idx];
  if (forward) {
    analogWrite(PIN_MOTOR_IN1, pwm);
    analogWrite(PIN_MOTOR_IN2, 0);
  } else {
    analogWrite(PIN_MOTOR_IN1, 0);
    analogWrite(PIN_MOTOR_IN2, pwm);
  }
}

void goToSleep() {
  // stop all outputs before sleeping
  analogWrite(PIN_MOTOR_IN1, 0);
  analogWrite(PIN_MOTOR_IN2, 0);
  digitalWrite(PIN_HEATER, LOW);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_BTN_POWER, 0);

  Serial.println("Entering deep sleep");
  Serial.flush();

  esp_deep_sleep_start();
}

String style() {


  return F(
      "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>"
      "body{margin:0;font-family:Arial,sans-serif;background:#1abc9c;color:#fff;display:flex;justify-content:center;align-items:flex-start;min-height:100vh;padding:20px;}"
      ".card{background:rgba(255,255,255,0.2);padding:20px;border-radius:8px;width:100%;max-width:420px;box-shadow:0 2px 6px rgba(0,0,0,0.2);}" 
      "h1{text-align:center;margin-top:0;}"
      "form{margin-bottom:20px;}"
      "label{font-weight:bold;display:block;margin-top:10px;}"
      "input,button{width:100%;padding:10px;margin-top:8px;border:none;border-radius:4px;}"
      "button{background:#00695c;color:#fff;font-weight:bold;}"
      "</style></head><body><div class='card'>");
}

String footer() {
  return F("</div></body></html>");
}

String loginPage() {
  return style() +
         F("<h1>Login</h1><form method='POST' action='/login'>"
           "<input type='password' name='pass' placeholder='Password'>"
           "<button type='submit'>Masuk</button></form>") +
         footer();

}

String controlPage() {
  String html = style();
  html += F("<h1>Massager</h1>");
  html += F("<form method='POST' action='/motor'><label>Motor Direction</label><div style='display:flex;gap:10px;'><button name='dir' value='f'>Forward</button><button name='dir' value='b'>Reverse</button></div><label>Speed</label><input type='number' name='spd' min='0' max='3' value='");
  html += String(speedIndex);
  html += F("'><button type='submit'>Set</button></form>");
  html += F("<form method='POST' action='/heater'><label>Heater</label><div style='display:flex;gap:10px;'><button name='h' value='1'>ON</button><button name='h' value='0'>OFF</button></div></form>");
  html += F("<form method='POST' action='/autooff'><label>Auto Off (minutes)</label><input type='number' name='m' value='");
  html += String(autoOffMinutes);
  html += F("'><button type='submit'>Save</button></form>");
  html += F("<form method='POST' action='/wifi'><label>WiFi SSID</label><input name='ssid' value='");
  html += wifiSSID;
  html += F("' placeholder='SSID'><label>Password</label><input name='pwd' type='password' value='");
  html += wifiPwd;
  html += F("' placeholder='WiFi Password'><button type='submit'>Save WiFi</button></form>");
  html += F("<form method='POST' action='/changepass'><label>New Admin Password</label><input type='password' name='np' placeholder='New Password'><button type='submit'>Change Password</button></form>");
  html += F("<form method='POST' enctype='multipart/form-data' action='/update'><label>OTA Update</label><input type='file' name='update'><button type='submit'>Upload</button></form>");
  html += F("<form method='POST' action='/logout'><button type='submit'>Logout</button></form>");
  html += F("<form method='POST' action='/poweroff'><button type='submit'>Power Off</button></form>");
  html += footer();
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

    Serial.println("Login success");
  } else {
    server.send(403, "text/plain", "Wrong password");
    Serial.println("Login failed");

  }
}

void handleLogout() {
  session = false;
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");

  Serial.println("Logged out");

}

void handleChangePass() {
  if (server.arg("np").length() > 0) {
    adminPass = server.arg("np");
    savePrefs();

    Serial.println("Admin password changed");

  }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handleWifi() {
  String ssid = server.arg("ssid");
  String pwd = server.arg("pwd");
  prefs.putString("ssid", ssid);
  prefs.putString("pwd", pwd);

  wifiSSID = ssid;
  wifiPwd  = pwd;
  Serial.print("WiFi saved: ");
  Serial.println(wifiSSID);

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

  Serial.print("Motor ");
  Serial.print(motorForward ? "forward" : "reverse");
  Serial.print(" speed index ");
  Serial.println(speedIndex);

}

void handleHeater() {
  heaterOn = server.arg("h") == "1";
  digitalWrite(PIN_HEATER, heaterOn ? HIGH : LOW);
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
  lastAction = millis();

  Serial.println(heaterOn ? "Heater ON" : "Heater OFF");

}

void handleAutoOff() {
  autoOffMinutes = server.arg("m").toInt();
  if (autoOffMinutes < 1) autoOffMinutes = 1;
  savePrefs();
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");

  Serial.print("Auto-off set to ");
  Serial.print(autoOffMinutes);
  Serial.println(" minutes");

}

void handleOTA() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {

    Serial.println("OTA start");

    Update.begin();
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    Update.write(upload.buf, upload.currentSize);
  } else if (upload.status == UPLOAD_FILE_END) {
    Update.end(true);

    Serial.println("OTA end");

  }
  server.sendHeader("Location", "/");
  server.send(302, "text/plain", "");
}

void handlePowerOff() {
  server.send(200, "text/plain", "Going to sleep");

  Serial.println("Power off via web");

  delay(100);
  goToSleep();
}

void notFound() {
  server.send(404, "text/plain", "Not Found");
}

void setup() {

  Serial.begin(115200);
  Serial.println("Booting...");
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_HEATER, OUTPUT);

  pinMode(PIN_BTN_POWER, INPUT_PULLUP);
  pinMode(PIN_BTN_SPEED, INPUT_PULLUP);
  pinMode(PIN_BTN_HEAT, INPUT_PULLUP);


  prefs.begin("massager", false);
  adminPass = prefs.getString("pass", "admin");
  autoOffMinutes = prefs.getInt("autooff", 10);


  wifiSSID = prefs.getString("ssid", "");
  wifiPwd  = prefs.getString("pwd", "");

  if (wifiSSID.length() > 0) {
    Serial.print("Connecting to ");
    Serial.println(wifiSSID);
    WiFi.begin(wifiSSID.c_str(), wifiPwd.c_str());
    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("WiFi connect failed, starting AP");
      WiFi.mode(WIFI_AP);
      WiFi.softAP("MassagerSetup");
      Serial.print("AP IP: ");
      Serial.println(WiFi.softAPIP());
    } else {
      Serial.print("WiFi connected, IP: ");
      Serial.println(WiFi.localIP());

    }
  } else {
    WiFi.mode(WIFI_AP);
    WiFi.softAP("MassagerSetup");
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

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
  // start motor at full speed when powered on
  setMotor(motorForward, speedIndex);
  Serial.println("Motor started at full speed");

  lastAction = millis();
}

void loop() {
  server.handleClient();


  // power button long press to sleep
  if (digitalRead(PIN_BTN_POWER) == LOW) {
    if (powerPressStart == 0) {
      powerPressStart = millis();
    } else if (millis() - powerPressStart > 3000) {

      Serial.println("Power button long press");

      goToSleep();
    }
  } else {
    powerPressStart = 0;
  }

  // cycle motor speed
  bool sp = digitalRead(PIN_BTN_SPEED);
  if (lastSpeedState == HIGH && sp == LOW) {
    speedIndex = (speedIndex + 1) % 4;
    setMotor(motorForward, speedIndex);
    Serial.print("Speed button -> index ");
    Serial.println(speedIndex);

    lastAction = millis();
  }
  lastSpeedState = sp;

  // toggle heater
  bool hb = digitalRead(PIN_BTN_HEAT);
  if (lastHeaterState == HIGH && hb == LOW) {
    heaterOn = !heaterOn;
    digitalWrite(PIN_HEATER, heaterOn ? HIGH : LOW);
    Serial.println(heaterOn ? "Heater ON" : "Heater OFF");

    lastAction = millis();
  }
  lastHeaterState = hb;

  if (millis() - lastAction > (unsigned long)autoOffMinutes * 60000UL) {
    Serial.println("Auto-off timeout");

    goToSleep();
  }
}

