#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <time.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

// Motor control pins
#define IN1 26
#define IN2 27
#define ENA 14

// Network credentials
const char* ssid = "ESP32_Motor_Control";
const char* password = "123456789";

// Web server
WebServer server(80);

// Motor status
bool motorStatus = false;

// Schedule structure
struct Schedule {
  unsigned long id;
  int startHour;
  int startMinute;
  int durationMinutes;
  bool active;
};

// Vector to store schedules
std::vector<Schedule> schedules;

// Time sync
struct tm timeinfo;

void setup() {
  Serial.begin(115200);

  // Initialize motor control pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);

  // Turn off motor initially
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(ENA, LOW);

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  // Load schedules from SPIFFS
  loadSchedules();

  // Setup Access Point
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // Set up mDNS responder
  if (!MDNS.begin("motorcontrol")) {
    Serial.println("Error setting up MDNS responder!");
  } else {
    Serial.println("mDNS responder started");
  }

  // Setup time
  // configTime(0, 0, "pool.ntp.org");

  // Configure web server routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/on", HTTP_GET, handleMotorOn);
  server.on("/off", HTTP_GET, handleMotorOff);
  server.on("/status", HTTP_GET, handleStatus);
  server.on("/schedule", HTTP_POST, handleSchedule);
  server.on("/delete-schedule", HTTP_GET, handleDeleteSchedule);
  server.on("/set-time", HTTP_POST, handleSetTime);

  // Start server
  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  // Get current time
  if (!getLocalTime(&timeinfo)) {
    Serial.println("Failed to obtain time");
  } else {
    // Check schedules
    checkSchedules();
  }

  delay(1000);
}

void handleSetTime() {
  if (server.hasArg("timestamp")) {
    // Get timestamp from phone (in seconds since epoch)
    unsigned long timestamp = strtoul(server.arg("timestamp").c_str(), NULL, 10);

    // Set ESP32 time
    struct timeval tv;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);

    if (getLocalTime(&timeinfo)) {
      char timeString[30];
      strftime(timeString, sizeof(timeString), "%H:%M:%S %d-%m-%Y", &timeinfo);
      Serial.print("Time set to: ");
      Serial.println(timeString);
      server.send(200, "text/plain", "Time set successfully");
    } else {
      server.send(500, "text/plain", "Failed to set time");
    }
  } else {
    server.send(400, "text/plain", "Missing timestamp parameter");
  }
}

// Handle root request (serve the HTML page)
void handleRoot() {
  File file = SPIFFS.open("/index.html", "r");

  if (!file) {
    // If file doesn't exist, send the HTML content directly
    server.send(200, "text/html", getHTMLContent());
  } else {
    server.streamFile(file, "text/html");
    file.close();
  }
}

// Return the HTML content as a string
String getHTMLContent() {
  // Get the HTML content from your artifact
  return "<!DOCTYPE html><html>...</html>";  // This would be your full HTML content
}

// Turn motor on
void handleMotorOn() {
  motorStatus = true;
  controlMotor(true);
  sendMotorStatus();
}

// Turn motor off
void handleMotorOff() {
  motorStatus = false;
  controlMotor(false);
  sendMotorStatus();
}

// Get motor status
void handleStatus() {
  sendMotorStatus();
}

// Send current motor status as JSON
void sendMotorStatus() {
  String response = "{\"motorStatus\":" + String(motorStatus ? "true" : "false") + "}";
  server.send(200, "application/json", response);
}

// Control the motor
void controlMotor(bool on) {
  if (on) {
    digitalWrite(IN1, HIGH);
    digitalWrite(IN2, LOW);
    digitalWrite(ENA, HIGH);
    Serial.println("Motor turned ON");
  } else {
    digitalWrite(IN1, LOW);
    digitalWrite(IN2, LOW);
    digitalWrite(ENA, LOW);
    Serial.println("Motor turned OFF");
  }
}

// Handle schedule request
void handleSchedule() {
  if (server.hasArg("startTime") && server.hasArg("duration") && server.hasArg("id")) {
    String startTimeStr = server.arg("startTime");
    int duration = server.arg("duration").toInt();
    unsigned long id = server.arg("id").toInt();

    // Parse time (format HH:MM)
    int startHour = startTimeStr.substring(0, 2).toInt();
    int startMinute = startTimeStr.substring(3, 5).toInt();

    // Create schedule
    Schedule newSchedule;
    newSchedule.id = id;
    newSchedule.startHour = startHour;
    newSchedule.startMinute = startMinute;
    newSchedule.durationMinutes = duration;
    newSchedule.active = true;

    // Add to schedules vector
    schedules.push_back(newSchedule);

    // Save schedules to SPIFFS
    saveSchedules();

    server.send(200, "text/plain", "Schedule added");
    Serial.println("New schedule added: " + startTimeStr + " for " + String(duration) + " minutes");
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

// Handle delete schedule
void handleDeleteSchedule() {
  if (server.hasArg("id")) {
    unsigned long id = server.arg("id").toInt();

    // Find and remove schedule with matching ID
    for (size_t i = 0; i < schedules.size(); i++) {
      if (schedules[i].id == id) {
        schedules.erase(schedules.begin() + i);
        break;
      }
    }

    // Save updated schedules
    saveSchedules();

    server.send(200, "text/plain", "Schedule deleted");
  } else {
    server.send(400, "text/plain", "Missing ID parameter");
  }
}

// Check schedules against current time
void checkSchedules() {
  int currentHour = timeinfo.tm_hour;
  int currentMinute = timeinfo.tm_min;
  int currentTimeInMinutes = currentHour * 60 + currentMinute;

  for (auto& schedule : schedules) {
    int startTimeInMinutes = schedule.startHour * 60 + schedule.startMinute;
    int endTimeInMinutes = startTimeInMinutes + schedule.durationMinutes;

    // Check if current time is within schedule time
    if (currentTimeInMinutes >= startTimeInMinutes && currentTimeInMinutes < endTimeInMinutes) {
      // Turn on motor if it's not already on
      if (!motorStatus) {
        motorStatus = true;
        controlMotor(true);
        Serial.println("Motor turned ON by schedule");
      }
    } else if (currentTimeInMinutes == endTimeInMinutes && motorStatus) {
      // Turn off motor when schedule ends
      motorStatus = false;
      controlMotor(false);
      Serial.println("Motor turned OFF by schedule end");
    }
  }
}

// Save schedules to SPIFFS
void saveSchedules() {
  File file = SPIFFS.open("/schedules.json", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  DynamicJsonDocument doc(2048);
  JsonArray array = doc.to<JsonArray>();

  for (const auto& schedule : schedules) {
    JsonObject obj = array.createNestedObject();
    obj["id"] = schedule.id;
    obj["startHour"] = schedule.startHour;
    obj["startMinute"] = schedule.startMinute;
    obj["durationMinutes"] = schedule.durationMinutes;
    obj["active"] = schedule.active;
  }

  if (serializeJson(doc, file) == 0) {
    Serial.println("Failed to write to file");
  }

  file.close();
}

// Load schedules from SPIFFS
void loadSchedules() {
  File file = SPIFFS.open("/schedules.json", "r");
  if (!file) {
    Serial.println("No schedules file found");
    return;
  }

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);

  if (error) {
    Serial.println("Failed to parse file");
  } else {
    schedules.clear();

    JsonArray array = doc.as<JsonArray>();
    for (JsonObject obj : array) {
      Schedule schedule;
      schedule.id = obj["id"].as<unsigned long>();
      schedule.startHour = obj["startHour"].as<int>();
      schedule.startMinute = obj["startMinute"].as<int>();
      schedule.durationMinutes = obj["durationMinutes"].as<int>();
      schedule.active = obj["active"].as<bool>();

      schedules.push_back(schedule);
    }

    Serial.println("Loaded " + String(schedules.size()) + " schedules");
  }

  file.close();
}