// Libraries
#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

// Ports
const int pin_sensor_humidity 		  = A0;
const int pin_sensor_temperature	  = A1;
const int pin_sensor_water    		  = A2;
const int pin_sensor_light    		  = A3;
const int pin_pump            		  = D9;
const int pin_LED             		  = D8;

// Variables for HTTP POST request data.
String postData = ""; //--> Variables sent for HTTP POST request data.
String payload = "";  //--> Variable for receiving response from HTTP POST.

// Sensor variables
float value_humidity;
float value_temperature;
float value_water;
float value_light;

// Actuate variables
bool active_pump;
bool active_light;

// Thresholders;
float threshold_humidity;
float threshold_light;
float threshold_light_timer;          	    // [h]

// Time variables
float begginer_timer;						            // [h]
float current_timer;                  	    // [h]
float previous_timer;						            // [h]
float light_timer;                    	    // [h]
int qnt_days;
bool day_change;

// Wi-Fi Connection
const char* WIFI_SSID = "MEO-E50B10";
const char* WIFI_PASSWORD = "af8fe101ab";

// MySQL Connection
IPAddress server_addr(192,168,1,93);       // IP of the MySQL *server* here
IPAddress ip(192,168,1,122);
char user[]                       = "arduino_user"; // MySQL user login username
char password[]                   = "arduino_password"; // MySQL user login password

WiFiClient client;

bool plant_added;
bool new_plant;
int plant_added_id                = -1;

// Queries
char INSERT_DATA[]                = "INSERT INTO db_arduino.tb_sensors (Air_Temperature, Soil_Humidity, Light_Intensity, Water_Level) VALUES (%.2f,%.2f,%.2f,%.2f)";
char VERIFY_PLANT_ADD[]           = "SELECT End_Time FROM db_arduino.tb_config WHERE db_arduino.tb_config.ID = (SELECT MAX(ID) FROM db_arduino.tb_config)";
char VERIFY_NEW_PLANT_ADD[]       = "SELECT ID FROM db_arduino.tb_config WHERE db_arduino.tb_config.ID = (SELECT MAX(ID) FROM db_arduino.tb_config)";
char GET_BEGGINER_TIME[]          = "SELECT Connection_Time FROM db_arduino.tb_config WHERE db_arduino.tb_config.ID = (SELECT MAX(ID) FROM db_arduino.tb_config)";
char GET_THRESHOLDERS[]           = "SELECT Soil_Humidity_Min, Light_Intensity_Min, Light_Exposure_Min FROM db_arduino.v_thresholders WHERE %d BETWEEN Start_Week AND End_Week";

void setup() {
  pinMode(pin_pump, OUTPUT);
  pinMode(pin_LED, OUTPUT);
  
  Serial.begin(9600);
  while (!Serial); 

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  CheckWiFiConnection();
}


void loop() {
  CheckWiFiConnection();
  
  // Verify there is a plant and the program can run
  plant_added                   = VerifyPlantAdded();
  if (plant_added){
    // Verify if it is a new plant
    new_plant                   = VerifyNewPlant();

    if (new_plant){
      // Reset Variables
      current_timer             = 0;
      light_timer               = 0;
      qnt_days                  = 0;
      begginer_timer            = GetBegginerTimer();
    }
  }

  plant_added = true;
  if (plant_added){
    // Verify if it is a new week
    if (qnt_days % 7 == 0) {
      int week                  = qnt_days % 7 + 1;
      GetThresholders(week);
    }

    // Sensors values
    value_humidity 	            = GetHumidity();
    value_light		              = GetLight();
    value_water		              = GetWaterLevel();
    value_temperature	          = GetTemperature();

    // Time variables
    current_timer 	            = CurrentTimeCount(begginer_timer);
    day_change 		              = VerifyDayChange(previous_timer, current_timer);
    if (day_change){
      qnt_days                  = qnt_days + 1;
    }
    
    // Update light timer
    light_timer                 = LightTimeCount(day_change, light_timer, current_timer, previous_timer, value_light, threshold_light);
    
    // Update previous timer
    previous_timer              = current_timer;
    
    // Mensages
    //Serial.println("-- Current timer --");
    //Serial.println(current_timer);
    //Serial.println("-- New values --");
    //Serial.print("Humidity Value:\t\t");
    //Serial.println(value_humidity);
    //Serial.print("Light Value:\t\t");
    //Serial.println(value_light);
   // Serial.print("Water level Value:\t");
   // Serial.println(value_water);
   // Serial.print("Temperature Value:\t");
   // Serial.println(value_temperature);
    
    // Actuators
    active_pump                 = ActPump(value_humidity, threshold_humidity);
    if (active_pump)
    {
      digitalWrite(pin_pump, HIGH);
    }else{
      digitalWrite(pin_pump, LOW);
    }
    
    active_light                = ActLight(value_light, threshold_light, light_timer, threshold_light_timer);
    
    if (active_light)
    {
      digitalWrite(pin_LED, HIGH);
    }else{
      digitalWrite(pin_LED, LOW);
    }
    
    // Insert the sensors data in the database
    InsertData(value_temperature, value_humidity, value_light, value_water);
  }
  delay(10000);
}


// Functions

// Sensors functions
float GetHumidity() {
  int raw_humidity;

  raw_humidity                    = analogRead(pin_sensor_humidity);
  float humidity_percent          = raw_humidity;

  return humidity_percent;
}

float GetLight() {
  int raw_light;

  raw_light                       = analogRead(pin_sensor_light);
  float light_percent             = raw_light;

  return light_percent;
}

float GetWaterLevel() {
  int raw_water;

  raw_water                       = analogRead(pin_sensor_water);
  float water_percent             = raw_water;

  return water_percent;
}

float GetTemperature() {
  int raw_temperature;

  raw_temperature                 = analogRead(pin_sensor_temperature);
  float temperature               = raw_temperature;

  return temperature;
}


// Actuate functions
bool ActPump(float humidity, float threshold){
  
  bool active                     = false;
  
  if (humidity < threshold){
  	active                        = true;
  }
  
  return active;
}

bool ActLight(float light, float threshold, float light_timer, float threshold_timer){
  
  bool active                     = false;
  
  if ((light < threshold) && (light_timer < threshold_timer)){
  	active                        = true;
  }
  return active;
}


// Time functions
float CurrentTimeCount(float begginer_timer) {
  float elapsed_hours             = millis() / (1000.0 * 60.0 * 60.0); // Convert millis to hours
  float current_timer             = fmod((elapsed_hours + begginer_timer), 24.0); // Keep in 24-hour format

  return current_timer;
}

bool VerifyDayChange(float previous_timer, float current_timer){
  bool day_changed;

  if (current_timer < previous_timer) {
    day_changed                   = true;
  } else {
    day_changed                   = false;
  }
  
  return day_changed;
}

float LightTimeCount(bool day_change, float light_timer, float current_timer, float previous_timer, float value_light, float threshold_light) {
    if (day_change) {
        light_timer               = 0;
    }

    if (value_light > threshold_light) {
        float time_difference = current_timer - previous_timer;
        light_timer += time_difference;
    }

    return light_timer;
}


// Database functions
void CheckWiFiConnection(){

  Serial.println("============= Wifi Connection");

  while(WiFi.status() != WL_CONNECTED) {
    Serial.println("Connecting Wifi...");
    delay(500);
  }

  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  Serial.println("====================================");
  Serial.println("");
}

bool VerifyPlantAdded() {
  HTTPClient http;
  int httpCode;

  http.begin("http://localhost/verify_plant_added.php"); // Replace <server-ip>
  
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String response = http.getString();
    return response.indexOf("\"plant_added\":true") >= 0;
  }

  http.end();
  return false;
}

bool VerifyNewPlant() {
  HTTPClient http;
  http.begin("http://localhost/verify_new_plant.php");

  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
    String response = http.getString();
    int newIndex = response.substring(response.indexOf(":") + 1, response.indexOf("}")).toInt();
    if (newIndex != plant_added_id) {
      plant_added_id = newIndex;
      return true;
    }
  }

  http.end();
  return false;
}

float GetBegginerTimer() {
  
  Serial.println("============= Receive the begginer time");

  HTTPClient http;
  http.begin("http://192.168.1.93/get_begginer_timer.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  postData = "";
  int httpCode = http.POST(postData);

  if (httpCode > 0) {
    payload = http.getString();

    // Parse JSON response
    JSONVar jsonObject = JSON.parse(payload);

    // Check if JSON parsing was successful and the value exists
    if (JSON.typeof(jsonObject) == "undefined") {
      Serial.println("Parsing input failed!");
    } else {
      // If data is found, get the Connection_Time value
      String connectionTime = jsonObject["Connection_Time"];
      
      // Handle cases where "Connection_Time" is "No data available"
      if (connectionTime == "No data available") {
        Serial.println("No data available for Connection_Time.");
      } else {
        // Parse and return the float value of Connection_Time
        return connectionTime.toFloat();
        Serial.println("");
      }
    }
  }

  http.end();
  Serial.println("");
  return 0;  // Return 0 if there was no valid HTTP response
}

void GetThresholders(int week) {

  Serial.println("============= Receive the threshoulders");
  Serial.print("=============   Week ");
  Serial.println(week);

  HTTPClient http;
  String postData = "week=" + String(week);

  http.begin("http://192.168.1.93/get_thresholders.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(postData);

  if (httpCode > 0) {
      payload = http.getString();

      JSONVar jsonObject = JSON.parse(payload);

      if (JSON.typeof(jsonObject) == "undefined") {
          Serial.println("Parsing failed!");
      } else if (jsonObject.hasOwnProperty("error")) {
          // Handle the error response
          Serial.println((const char*)jsonObject["error"]);
          threshold_humidity = -1; // Indicate no valid data
          threshold_light = -1;
          threshold_light_timer = -1;
      } else {
          // Extract threshold values
          threshold_humidity = String((const char*)jsonObject["Soil_Humidity_Min"]).toFloat();
          threshold_light = String((const char*)jsonObject["Light_Intensity_Min"]).toFloat();
          threshold_light_timer = String((const char*)jsonObject["Light_Exposure_Min"]).toFloat();

          Serial.print("Soil Humidity Min: ");
          Serial.println(threshold_humidity);
          Serial.print("Light Intensity Min: ");
          Serial.println(threshold_light);
          Serial.print("Light Exposure Min: ");
          Serial.println(threshold_light_timer);
      }
  } else {
      Serial.print("HTTP request failed, error code: ");
  }

  http.end(); // Close the connection
  Serial.println("====================================");
  Serial.println("");
}


void InsertData(float value_temperature, float value_humidity, float value_light, float value_water) {
  Serial.println("============= Insert sensors data in the database");

  HTTPClient http;
  int httpCode;

  // URL with formatted query parameters
  String postData = "temperature=" + String(value_temperature, 2); // Format to 2 decimal places
  postData += "&humidity=" + String(value_humidity, 2);
  postData += "&light=" + String(value_light, 2);
  postData += "&water=" + String(value_water, 2);

  http.begin("http://192.168.1.93/insert_data.php");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  httpCode = http.POST(postData); //--> Send the request
  payload = http.getString();     //--> Get the response payload

  Serial.println(payload);  //--> Print request response payload
  
  http.end(); // Close connection
  Serial.println("====================================");
  Serial.println("");
}

