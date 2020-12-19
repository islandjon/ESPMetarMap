/*
  ESP Metar Map

  Designed for use with an ESP8266.

  Defaults are for a Wemos D1 Mini dev board with WS2812b LEDs using D7 for data line.

*/
  #include <ESP8266WiFi.h>
  #include <FastLED.h>
  #include <vector>
  #include <ESP8266WiFi.h>          
  #include <DNSServer.h>    
  #include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
  #include <WiFiManager.h>
  using namespace std;
  #define FASTLED_ESP8266_RAW_PIN_ORDER
  
  #define NUM_AIRPORTS 58 // Number of Airports/LEDs
  #define WIND_THRESHOLD 25 // Maximum windspeed for GREEN, otherwise the LED turns YELLOW
  #define LOOP_INTERVAL 5000 // ms - interval between brightness updates and lightning strikes
  #define DO_LIGHTNING false // Lightning uses more power, but is cool.
  #define DO_WINDS true // color LEDs for high winds
  #define REQUEST_INTERVAL 100000 // How often we update. In practice LOOP_INTERVAL is added. In ms (15 min is 900000)
    
  // Define the array of leds
  CRGB leds[NUM_AIRPORTS];
  #define DATA_PIN    D7 // Set to the pin used for led data
  #define LED_TYPE    WS2812B // Set to your LED type
  #define COLOR_ORDER GRB // Color order of the LEDs used
  #define BRIGHTNESS 30 // 20-30 recommended. Adjust as desired.


  // Airport code listing. Be sure to set NUM_AIRPORTS above to match the number used below.
  std::vector<unsigned short int> lightningLeds;
  std::vector<String> airports({
   "KGPT", // 1
   "KDED", // 2
   "KLEE", // 3
   "KSFB", // 4
   "KORL", // 5
   "KISM", // 6
   "KGIF", // 7
   "KBOW", // 8
   "KEVB", // 9
   "KNPA", // 10
   "KXMR", // 11
   "KCOF", // 12
   "KMLB", // 13
   "KLNA", // 14
   "KBCT", // 15
   "KPMP", // 16
   "KHWO", // 17
   "KOPF", // 18
   "KTMB", // 19
   "KHST", // 20
   "KECP", // 21
   "KPAM", // 22
   "KAAF", // 23
   "KBKV", // 24
   "KZPH", // 25
   "KLAL", // 26
   "KCLW", // 27
   "KPIE", // 28
   "KMCF", // 29
   "KSPG", // 30
   "KSRQ", // 31
   "KTLH", // 32
   "KFPY", // 33
   "KCTY", // 34
   "K24J", // 35
   "KLCQ", // 36
   "KVQQ", // 37
   "KSGJ", // 38
   "KDAB", // 39
   "KMCO", // 40
   "KLEE", // 41
   "K42J", // 42
   "KGNV", // 43
   "KOCF", // 44
   "KINF", // 45
   "KBKV", // 46
   "KTPA", // 47
   "KVNC", // 48
   "KPGD", // 49
   "KRSW", // 50
   "KAPF", // 51
   "KIMM", // 52
   "KSEF", // 53
   "KOBE", // 54
   "KSUA", // 55
   "KPBI", // 56
   "KFLL", // 57
   "KMIA", // 58
   });
  
  #define DEBUG false
  
  #define READ_TIMEOUT 15 // Cancel query if no data received (seconds)
  #define WIFI_TIMEOUT 60 // in seconds
  #define RETRY_TIMEOUT 15000 // in ms
  
  #define SERVER "www.aviationweather.gov"
  #define BASE_URI "/adds/dataserver_current/httpparam?dataSource=metars&requestType=retrieve&format=xml&hoursBeforeNow=3&mostRecentForEachStation=true&stationString="
  
  boolean ledStatus = true; // used so leds only indicate connection status on first boot, or after failure
  int loops = -1;
  
  int status = WL_IDLE_STATUS;
  
  void setup() {
    //Initialize serial and wait for port to open:
    Serial.begin(115200);
    //pinMode(D1, OUTPUT); //Declare Pin mode
    //while (!Serial) {
    //    ; // wait for serial port to connect. Needed for native USB
    //}
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    
    //set custom ip for portal
    //wifiManager.setAPStaticIPConfig(IPAddress(10,0,1,1), IPAddress(10,0,1,1), IPAddress(255,255,255,0));

    //fetches ssid and pass from eeprom and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    wifiManager.autoConnect("MetarMap");
    //or use this for auto generated name ESP + ChipID
    //wifiManager.autoConnect();

    
    //if you get here you have connected to the WiFi
    Serial.println("connected...");
  
    pinMode(LED_BUILTIN, OUTPUT); // give us control of the onboard LED
    digitalWrite(LED_BUILTIN, LOW);
  
    // Initialize LEDs
    FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_AIRPORTS).setCorrection(TypicalLEDStrip);
    FastLED.setBrightness(BRIGHTNESS);
  }

  void loop() {
    digitalWrite(LED_BUILTIN, LOW); // on if we're awake
  
    int c;
    loops++;
    Serial.print("Loop: ");
    Serial.println(loops);
    unsigned int loopThreshold = 1;
    if (DO_LIGHTNING) loopThreshold = REQUEST_INTERVAL / LOOP_INTERVAL;
  
    // Connect to WiFi. We always want a wifi connection for the ESP8266
  
    // Do some lightning
    if (DO_LIGHTNING && lightningLeds.size() > 0) {
      std::vector<CRGB> lightning(lightningLeds.size());
      for (unsigned short int i = 0; i < lightningLeds.size(); ++i) {
        unsigned short int currentLed = lightningLeds[i];
        lightning[i] = leds[currentLed]; // temporarily store original color
        leds[currentLed] = CRGB::White; // set to white briefly
        Serial.print("Lightning on LED: ");
        Serial.println(currentLed);
      }
      delay(25);
      FastLED.show();
      delay(25);
      for (unsigned short int i = 0; i < lightningLeds.size(); ++i) {
        unsigned short int currentLed = lightningLeds[i];
        leds[currentLed] = lightning[i]; // restore original color
      }
      FastLED.show();
    }
  
    if (loops >= loopThreshold || loops == 0) {
      loops = 0;
      if (DEBUG) {
        fill_gradient_RGB(leds, NUM_AIRPORTS, CRGB::Red, CRGB::Blue); // Just let us know we're running
        FastLED.show();
      }
  
      Serial.println("Getting METARs ...");
      if (getMetars()) {
        Serial.println("Refreshing LEDs.");
        FastLED.show();
        if ((DO_LIGHTNING && lightningLeds.size() > 0)) {
          Serial.println("There is lightning, so no long sleep.");
          digitalWrite(LED_BUILTIN, HIGH);
          delay(LOOP_INTERVAL); // pause during the interval
        } else {
          Serial.print("No lightning; Going into sleep for: ");
          Serial.println(REQUEST_INTERVAL);
          digitalWrite(LED_BUILTIN, HIGH);
          delay(REQUEST_INTERVAL);
        }
      } else {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(RETRY_TIMEOUT); // try again if unsuccessful
      }
    } else {
      digitalWrite(LED_BUILTIN, HIGH);
      delay(LOOP_INTERVAL); // pause during the interval
    }
  }
  
  bool getMetars(){
    lightningLeds.clear(); // clear out existing lightning LEDs since they're global
    fill_solid(leds, NUM_AIRPORTS, CRGB::Black); // Set everything to black just in case there is no report
    uint32_t t;
    char c;
    boolean readingAirport = false;
    boolean readingCondition = false;
    boolean readingWind = false;
    boolean readingGusts = false;
    boolean readingWxstring = false;
  
    std::vector<unsigned short int> led;
    String currentAirport = "";
    String currentCondition = "";
    String currentLine = "";
    String currentWind = "";
    String currentGusts = "";
    String currentWxstring = "";
    String airportString = "";
    bool firstAirport = true;
    for (int i = 0; i < NUM_AIRPORTS; i++) {
      if (airports[i] != "NULL" && airports[i] != "VFR" && airports[i] != "MVFR" && airports[i] != "WVFR" && airports[i] != "IFR" && airports[i] != "LIFR") {
        if (firstAirport) {
          firstAirport = false;
          airportString = airports[i];
        } else airportString = airportString + "," + airports[i];
      }
    }
  
    BearSSL::WiFiClientSecure client;
    client.setInsecure();
    Serial.println("\nStarting connection to server...");
    // if you get a connection, report back via serial:
    if (!client.connect(SERVER, 443)) {
      Serial.println("Connection failed!");
      client.stop();
      return false;
    } else {
      Serial.println("Connected ...");
      Serial.print("GET ");
      Serial.print(BASE_URI);
      Serial.print(airportString);
      Serial.println(" HTTP/1.1");
      Serial.print("Host: ");
      Serial.println(SERVER);
      Serial.println("Connection: close");
      Serial.println();
      // Make a HTTP request, and print it to console:
      client.print("GET ");
      client.print(BASE_URI);
      client.print(airportString);
      client.println(" HTTP/1.1");
      client.print("Host: ");
      client.println(SERVER);
      client.println("Connection: close");
      client.println();
      client.flush();
      t = millis(); // start time
      FastLED.clear();
  
      Serial.print("Getting data");
  
      while (!client.connected()) {
        if ((millis() - t) >= (READ_TIMEOUT * 1000)) {
          Serial.println("---Timeout---");
          client.stop();
          return false;
        }
        Serial.print(".");
        delay(1000);
      }
  
      Serial.println();
  
      while (client.connected()) {
        if ((c = client.read()) >= 0) {
          yield(); // Otherwise the WiFi stack can crash
          currentLine += c;
          if (c == '\n') currentLine = "";
          if (currentLine.endsWith("<station_id>")) { // start paying attention
            if (!led.empty()) { // we assume we are recording results at each change in airport
              for (vector<unsigned short int>::iterator it = led.begin(); it != led.end(); ++it) {
                doColor(currentAirport, *it, currentWind.toInt(), currentGusts.toInt(), currentCondition, currentWxstring);
              }
              led.clear();
            }
            currentAirport = ""; // Reset everything when the airport changes
            readingAirport = true;
            currentCondition = "";
            currentWind = "";
            currentGusts = "";
            currentWxstring = "";
          } else if (readingAirport) {
            if (!currentLine.endsWith("<")) {
              currentAirport += c;
            } else {
              readingAirport = false;
              for (unsigned short int i = 0; i < NUM_AIRPORTS; i++) {
                if (airports[i] == currentAirport) {
                  led.push_back(i);
                }
              }
            }
          } else if (currentLine.endsWith("<wind_speed_kt>")) {
            readingWind = true;
          } else if (readingWind) {
            if (!currentLine.endsWith("<")) {
              currentWind += c;
            } else {
              readingWind = false;
            }
          } else if (currentLine.endsWith("<wind_gust_kt>")) {
            readingGusts = true;
          } else if (readingGusts) {
            if (!currentLine.endsWith("<")) {
              currentGusts += c;
            } else {
              readingGusts = false;
            }
          } else if (currentLine.endsWith("<flight_category>")) {
            readingCondition = true;
          } else if (readingCondition) {
            if (!currentLine.endsWith("<")) {
              currentCondition += c;
            } else {
              readingCondition = false;
            }
          } else if (currentLine.endsWith("<wx_string>")) {
            readingWxstring = true;
          } else if (readingWxstring) {
            if (!currentLine.endsWith("<")) {
              currentWxstring += c;
            } else {
              readingWxstring = false;
            }
          }
          t = millis(); // Reset timeout clock
        } else if ((millis() - t) >= (READ_TIMEOUT * 1000)) {
          Serial.println("---Timeout---");
          fill_solid(leds, NUM_AIRPORTS, CRGB::Cyan); // indicate status with LEDs
          FastLED.show();
          ledStatus = true;
          client.stop();
          return false;
        }
      }
    }
    // need to doColor this for the last airport
    for (vector<unsigned short int>::iterator it = led.begin(); it != led.end(); ++it) {
      doColor(currentAirport, *it, currentWind.toInt(), currentGusts.toInt(), currentCondition, currentWxstring);
    }
    led.clear();
  
    // Do the key LEDs now if they exist
    for (int i = 0; i < (NUM_AIRPORTS); i++) {
      // Use this opportunity to set colors for LEDs in our key then build the request string
      if (airports[i] == "VFR") leds[i] = CRGB::Green;
      else if (airports[i] == "WVFR") leds[i] = CRGB::Yellow;
      else if (airports[i] == "MVFR") leds[i] = CRGB::Blue;
      else if (airports[i] == "IFR") leds[i] = CRGB::Red;
      else if (airports[i] == "LIFR") leds[i] = CRGB::Magenta;
    }
  
    client.stop();
    return true;
  }
  
  void doColor(String identifier, unsigned short int led, int wind, int gusts, String condition, String wxstring) {
    CRGB color;
    Serial.print(identifier);
    Serial.print(": ");
    Serial.print(condition);
    Serial.print(" ");
    Serial.print(wind);
    Serial.print("G");
    Serial.print(gusts);
    Serial.print("kts LED ");
    Serial.print(led);
    Serial.print(" WX: ");
    Serial.println(wxstring);
    if (wxstring.indexOf("TS") != -1) {
      Serial.println("... found lightning!");
      lightningLeds.push_back(led);
    }
    if (condition == "LIFR" || identifier == "LIFR") color = CRGB::Magenta;
    else if (condition == "IFR") color = CRGB::Red;
    else if (condition == "MVFR") color = CRGB::Blue;
    else if (condition == "VFR") {
      if ((wind > WIND_THRESHOLD || gusts > WIND_THRESHOLD) && DO_WINDS) {
        color = CRGB::Yellow;
      } else {
        color = CRGB::Green;
      }
    } else color = CRGB::Black;
  
    leds[led] = color;
  }
