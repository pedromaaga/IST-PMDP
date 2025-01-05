// Libraries
#include <Ethernet.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

// Ports
const int pin_sensor_humidity 		  = A0;
const int pin_sensor_temperature	  = A1;
const int pin_sensor_water    		  = A2;
const int pin_sensor_light    		  = A3;
const int pin_pump            		  = 9;
const int pin_LED             		  = 8;

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

// MySQL Connection
byte mac_addr[]                   = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };

IPAddress server_addr(192,168,1,254);       // IP of the MySQL *server* here
char user[]                       = "root"; // MySQL user login username
char password[]                   = "2605"; // MySQL user login password

EthernetClient client;
MySQL_Connection conn((Client *)&client);

bool connected;
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

  // Connect to the database
  connected                       = false;
  while (!connected) {
    Ethernet.begin(mac_addr);
    Serial.println("Connecting...");
    if (conn.connect(server_addr, 3306, user, password)) {
      connected                   = true;
      delay(1000);
    }
    else
      connected                   = false;
      Serial.println("Connection failed.");
  }
}


void loop() {
  if (conn.connected()){
    // Verify there is a plant and the program can run
    plant_added                   = VerifyPlantAdded(conn, VERIFY_PLANT_ADD);
    if (plant_added){
      // Verify if it is a new plant
      new_plant                   = VerifyNewPlant(conn, VERIFY_NEW_PLANT_ADD, plant_added_id);

      if (new_plant){
        // Reset Variables
        current_timer             = 0;
        light_timer               = 0;
        qnt_days                  = 0;
      }
    }

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
      InsertData(value_temperature, value_humidity, value_light, value_water, INSERT_DATA);

      delay(6000);
    }
  }else {
    conn.close();
    connected = false;
    while (!connected) {
    Serial.println("Connecting...");
      if (conn.connect(server_addr, 3306, user, password)) {
        connected                 = true;
        delay(1000);
      }
      else {
        connected                 = false;
        Serial.println("Connection failed.");
      }
    }
  }
}


// Functions

// Sensors functions
float GetHumidity() {
  int raw_humidity;

  raw_humidity                    = analogRead(pin_sensor_humidity);
  float humidity_percent          = (raw_humidity / 798.0) * 100.0;

  return humidity_percent;
}

float GetLight() {
  int raw_light;

  raw_light                       = analogRead(pin_sensor_light);
  float light_percent             = ((raw_light - 6.0) / (679.0 - 6.0)) * 100.0;

  return light_percent;
}

float GetWaterLevel() {
  int raw_water;

  raw_water                       = analogRead(pin_sensor_water);
  float water_percent             = ((raw_water - 20.0) / (358.0 - 20.0)) * 100.0;

  return water_percent;
}

float GetTemperature() {
  int raw_temperature;

  raw_temperature                 = analogRead(pin_sensor_temperature);
  float temperature               = ((raw_temperature - 20.0) / (358.0 - 20.0))*((125.0 - (-40.0))) + (-40.0);

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
bool VerifyPlantAdded(MySQL_Connection conn, char VERIFY_PLANT_ADD){

  bool plant_added = false;
  row_values *row = NULL;
  int end_time;

  MySQL_Cursor *cursor = new MySQL_Cursor(&conn);
  cursor->execute(VERIFY_PLANT_ADD);

  // We dont use these columns, but is required do this
  column_names *columns = cursor->get_columns();
  do {
    row = cursor->get_next_row();
    if (row != NULL) {
      end_time = atol(row->values[0]);
    }
  } while (row != NULL);

  if (end_time == -1){
    plant_added   = true;
  };

  delete cursor;

  delay(1000);

  return plant_added;
}

bool VerifyNewPlant(MySQL_Connection conn, char VERIFY_NEW_PLANT_ADD, int plant_added_id){

  row_values *row = NULL;
  int new_index;
  bool new_plant   = false;

  MySQL_Cursor *cursor = new MySQL_Cursor(&conn);
  cursor->execute(VERIFY_NEW_PLANT_ADD);

  // We dont use these columns, but is required do this
  column_names *columns = cursor->get_columns();
  do {
    row = cursor->get_next_row();
    if (row != NULL) {
      new_index = atol(row->values[0]);
    }
  } while (row != NULL);

  if (new_index != plant_added_id){
    plant_added_id   = new_index;
    new_plant = true;
  };

  delete cursor;

  delay(1000);

  return new_plant;
}

void GetThresholders(int week){

  char query[128];
  row_values *row = NULL;

  sprintf(query, GET_THRESHOLDERS, week);
  MySQL_Cursor *cursor = new MySQL_Cursor(&conn);
  cursor->execute(query);

  // We dont use these columns, but is required do this
  column_names *columns = cursor->get_columns();
  do {
    row = cursor->get_next_row();
    if (row != NULL) {
      threshold_humidity = atol(row->values[0]);
      threshold_light = atol(row->values[1]);
      threshold_light_timer = atol(row->values[0]);
    }
  } while (row != NULL);

  delete cursor;

  delay(1000);
}

void InsertData(float value_temperature, float value_humidity, float value_light, float value_water, char INSERT_DATA){

  char query[128];

  sprintf(query, INSERT_DATA, value_temperature, value_humidity, value_light, value_water);
  MySQL_Cursor *cursor = new MySQL_Cursor(&conn);
  cursor->execute(query);

  delete cursor;

  delay(1000);
}
