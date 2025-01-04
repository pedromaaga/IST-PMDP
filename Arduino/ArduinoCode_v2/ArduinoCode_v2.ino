// Libraries
#include <Ethernet.h>
#include <MySQL_Connection.h>

// Ports
const int pin_sensor_humidity 		= A0;
const int pin_sensor_temperature	= A1;
const int pin_sensor_water    		= A2;
const int pin_sensor_light    		= A3;
const int pin_pump            		= 9;
const int pin_LED             		= 8;

// Sensor variables
float value_humidity;
float value_temperature;
float value_water;
float value_light;

// Actuate variables
bool active_pump 					        = 0;
bool active_light 					      = 0;

// Thresholders;
float threshold_humidity			    = 50;
float threshold_light 				    = 50;
float threshold_light_timer			  = 8;	    // [h]

// Time variables
float begginer_timer;						            // [h]
float current_timer 				      = 0;	    // [h]
float previous_timer;						            // [h]
float light_timer					        = 0;	    // [h]
bool day_change;

// MySQL Connection
byte mac_addr[]                   = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress server_addr(192,168,1,254);     // IP of the MySQL *server* here
char user[]                       = "root"; // MySQL user login username
char password[]                   = "2605"; // MySQL user login password

EthernetClient client;
MySQL_Connection conn((Client *)&client);

int connected                     = 0;
int plant_added                   = 0;

// Queries
char INSERT_DATA[]                = "INSERT INTO db_arduino.tb_sensors (Air_Temperature, Soil_Humidity, Light_Intensity, Water_Level) VALUES (%.2f,%.2f,%.2f,%.2f)";
char VERIFY_PLANT_ADD[]           = "SELECT ID, End_Time FROM db_arduino.tb_config WHERE db_arduino.tb_config.ID = (SELECT MAX(ID) FROM db_arduino.tb_config)";
char GET_BEGGINER_TIME[]          = "SELECT Connection_Time FROM db_arduino.tb_config WHERE db_arduino.tb_config.ID = (SELECT MAX(ID) FROM db_arduino.tb_config)";
char GET_THRESHOLDERS[]           = "SELECT * FROM db_arduino.v_thresholders";
char query[128];


void setup() {
  pinMode(pin_pump, OUTPUT);
  pinMode(pin_LED, OUTPUT);
  
  Serial.begin(9600);

  // Connect to the database
  while (!Serial);
  while (!connected) {
    Ethernet.begin(mac_addr);
    Serial.println("Connecting...");
    if (conn.connect(server_addr, 3306, user, password)) {
      connected = 1;
      delay(1000);
    }
    else
      connected = 0;
      Serial.println("Connection failed.");
  }
}


void loop() {

  // Verify if a new plant was added and the program can run
  plant_added         = VerifyPlantAdded();
  if (plant_added){
    // Sensors values
    value_humidity 	  = GetHumidity();
    value_light		    = GetLight();
    value_water		    = GetWaterLevel();
    value_temperature	= GetTemperature();

    // Time variables
    current_timer 	  = CurrentTimeCount(begginer_timer);
    day_change 		    = VerifyDayChange(previous_timer, current_timer);
    
    // Update light timer
    light_timer       = LightTimeCount(day_change, light_timer, current_timer, previous_timer, value_light, threshold_light);
    
    // Update previous timer
    previous_timer    = current_timer;
    
    // Mensages
    Serial.println("-- Current timer --");
    Serial.println(current_timer);
    Serial.println("-- New values --");
    Serial.print("Humidity Value:\t\t");
    Serial.println(value_humidity);
    Serial.print("Light Value:\t\t");
    Serial.println(value_light);
    Serial.print("Water level Value:\t");
    Serial.println(value_water);
    Serial.print("Temperature Value:\t");
    Serial.println(value_temperature);
    
    // Actuators
    active_pump       = ActPump(value_humidity, threshold_humidity);
    if (active_pump)
    {
      digitalWrite(pin_pump, HIGH);
    }else{
      digitalWrite(pin_pump, LOW);
    }
    
    active_light     = ActLight(value_light, threshold_light, light_timer, threshold_light_timer);
    
    if (active_light)
    {
      digitalWrite(pin_LED, HIGH);
    }else{
      digitalWrite(pin_LED, LOW);
    }
    
    delay(1000);
  }
}


// Functions

// Sensors functions
float GetHumidity() {
  int raw_humidity;
  raw_humidity = analogRead(pin_sensor_humidity);
  float humidity_percent = (raw_humidity / 798.0) * 100.0;
  return humidity_percent;
}

float GetLight() {
  int raw_light;
  raw_light = analogRead(pin_sensor_light);
  float light_percent = ((raw_light - 6.0) / (679.0 - 6.0)) * 100.0;
  return light_percent;
}

float GetWaterLevel() {
  int raw_water;
  raw_water = analogRead(pin_sensor_water);
  float water_percent = ((raw_water - 20.0) / (358.0 - 20.0)) * 100.0;
  return water_percent;
}

float GetTemperature() {
  int raw_temperature;
  raw_temperature = analogRead(pin_sensor_temperature);
  float temperature = ((raw_temperature - 20.0) / (358.0 - 20.0))*((125.0 - (-40.0))) + (-40.0);
  return temperature;
}


// Actuate functions
bool ActPump(float humidity, float threshold){
  
  bool active = 0;
  
  if (humidity < threshold){
  	active = 1;
  }
  
  return active;
}

bool ActLight(float light, float threshold, float light_timer, float threshold_timer){
  
  bool active = 0;
  
  if ((light < threshold) && (light_timer < threshold_timer)){
  	active = 1;
  }
  return active;
}


// Time functions
float CurrentTimeCount(float begginer_timer) {
  
  float elapsed_hours = millis() / (1000.0 * 60.0 * 60.0); // Convert millis to hours
  float current_timer = fmod((elapsed_hours + begginer_timer), 24.0); // Keep in 24-hour format
  return current_timer;
  
}

bool VerifyDayChange(float previous_timer, float current_timer){
  
  bool day_changed;
  if (current_timer < previous_timer) {
    day_changed = true;
  } else {
    day_changed = false;
  }
  
  return day_changed;
}

float LightTimeCount(bool day_change, float light_timer, float current_timer, float previous_timer, float value_light, float threshold_light) {
    if (day_change) {
        light_timer = 0;
    }

    if (value_light > threshold_light) {
        float time_difference = current_timer - previous_timer;
        light_timer += time_difference;
    }

    return light_timer;
}

// Database functions
bool VerifyPlantAdded(){
  return 0;
}
