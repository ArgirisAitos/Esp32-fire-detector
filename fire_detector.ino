 #include <WiFi.h>
  #include <PubSubClient.h>
  #include <Adafruit_Sensor.h>
  #include <DHT.h>
  #include <WiFiManager.h> 

  
  #define MQ2_PIN 32       
  #define FLAME_SENSOR_PIN 34 
  #define DHT_PIN 4          
  #define DHT_TYPE DHT22 
  #define BUZZER_PIN 25      
  #define LED_GREEN 26        
  #define LED_YELLOW 27       
  #define LED_RED 14          

  DHT dht(DHT_PIN, DHT_TYPE);
    
  const char* mqtt_server = ".............";  
  const int mqtt_port=1883;
  const char* topic_temp = "sensors/temperature";
  const char* topic_hum = "sensors/humidity";
  const char* topic_smoke = "sensors/smoke";
  const char* topic_flame = "sensors/flame";
    
  WiFiClient espClient;
  PubSubClient client(espClient);
  
  float lastTemperature = 0;

  void setup() {
    Serial.begin(115200);
    dht.begin();
    pinMode(MQ2_PIN, INPUT);
    pinMode(FLAME_SENSOR_PIN, INPUT);
    pinMode(BUZZER_PIN, OUTPUT);
    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_YELLOW, OUTPUT);
    pinMode(LED_RED, OUTPUT);

      // Intial state
    digitalWrite(LED_GREEN, HIGH);
    digitalWrite(LED_YELLOW, LOW);
    digitalWrite(LED_RED, LOW);
    digitalWrite(BUZZER_PIN, LOW);

    setup_wifi();
    client.setServer(mqtt_server,mqtt_port);
    
    }
    
    void setup_wifi() {
      WiFiManager wifiManager;
      if (!wifiManager.autoConnect("FireDetector-Setup", "12345678")) {
         Serial.println("Connection failed, restarting ESP...");
         delay(3000);
         ESP.restart();
     }
    
    Serial.println("WiFi connected!");
    Serial.print("Local IP: ");
    Serial.println(WiFi.localIP());

    }

    void reconnect(){
      while (!client.connected()){
            Serial.print("ΜQTT Connection attempt");
            if(client.connect("ESP32Client")){
             Serial.println("Connected");
             } else{
            Serial.print("failed, returnCode= ");
            Serial.print(client.state());
            Serial.print("test in 3 second");
            delay(3000);
            }
         } 
      }


     void sendMQTT(const char* topic, float value){
          char buffer[10];
        dtostrf(value, 1, 2, buffer);
        client.publish(topic, buffer);
      }
      void loop() {
        
        if (!client.connected()) {
           reconnect();
          }
      client.loop();


      int smokeValue = analogRead(MQ2_PIN); // Read MQ-2
      int ppm = map(smokeValue, 0, 4095, 0, 5000); //Approximate smoke concentration (PPM)  

      int flameValue = analogRead(FLAME_SENSOR_PIN); // Read flame sensor
      float temperature = dht.readTemperature(); // Read Temperature
      float humidity = dht.readHumidity(); // Read Humidity

      if (isnan(temperature) || isnan(humidity)) {
          Serial.println("Error reading from DHT22 sensor");
          return;
        }
      
    sendMQTT(topic_temp, temperature);
    sendMQTT(topic_hum, humidity);
    sendMQTT(topic_smoke, ppm);
    sendMQTT(topic_flame, flameValue);

        
      // Print value at serial monitor
      Serial.print("Temperature: "); Serial.print(temperature);
      Serial.print("°C | Humidity: "); Serial.print(humidity);
      Serial.print("% | Smoke: "); Serial.print(smokeValue);
      Serial.print(" | Flame: "); Serial.println(flameValue);
      Serial.print("Estimated Smoke PPM: ");Serial.println(ppm);

    // Evaluate current sensor readings and trigger appropriate local alerts (LEDs, buzzer)

            //   Confirmed fire 
         if ((smokeValue > 900 && temperature > 40) ||    // smoke and Tempereature 
            (flameValue > 1000 && smokeValue > 900)   ||   // flame and Smoke 
            (temperature > 40 && humidity < 30)) {        //Temperature and low humidity
            digitalWrite(LED_GREEN, LOW);
            digitalWrite(LED_YELLOW, LOW);
            digitalWrite(LED_RED, HIGH);
            tone(BUZZER_PIN, 3000);
            Serial.println(" Alarm: Confirmed fire"); 
          }

       //  Warning: signs of fire
        
        else if ((smokeValue >= 500) || (temperature > 38 && (temperature - lastTemperature > 0.2)) || (flameValue > 850) || (humidity < 35)) { 
          digitalWrite(LED_GREEN, LOW);
          digitalWrite(LED_YELLOW, HIGH);
          digitalWrite(LED_RED, LOW);

          for (int i = 0; i < 3; i++) {  // warning sound
            tone(BUZZER_PIN, 2000);
            delay(300);
            noTone(BUZZER_PIN);
            delay(300);
          }

          Serial.println(" Warning: Fire hazard");
      }
      // Normal State
    else {
          digitalWrite(LED_GREEN, HIGH);
          digitalWrite(LED_YELLOW, LOW);
          digitalWrite(LED_RED, LOW);
          digitalWrite(BUZZER_PIN, LOW);
          noTone(BUZZER_PIN);
          Serial.println(" Normal State: There is no fire.");
      }

      lastTemperature = temperature; // Save last temperature
      delay(400); 
    }