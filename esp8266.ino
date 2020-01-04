#include <Wire.h>
#include <ESP8266WiFi.h>
//#include <ESP8266WebServer.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>

const char* ssid = "Chipotle Free Wifi";
const char* password = "1550stanford";
//WiFiServer server(80);
AsyncWebServer server(80);
//ESP8266WebServer server(80);

File image;
bool receiving = false;
bool writing = true;
uint16_t jpglen = 0;
int count = 0;
long timePassed = 0;

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://stackpath.bootstrapcdn.com/bootstrap/4.3.1/css/bootstrap.min.css" 
  integrity="sha384-ggOyR0iXCbMQv3Xipma34MD+dH/1fQ784/j6cY/iJTQUOhcWr7x9JvoRxT2MZw1T" crossorigin="anonymous">
  <script defer src="https://code.highcharts.com/highcharts.js"></script>
  <script defer src="https://drive.google.com/uc?export=view&id=1m8hVCjgy964OCCOHLZP44X5shRFrNxat"></script>
</head>
<body style="text-align: center;">
  <h2>ESP Web Server</h2>
  <br />
  <br />
  <div style="padding: 5px;">
  Arduino
  <form style="display: inline;" method="get" action="/arduinoOn"><button class="btn btn-success">ON</button></form>
  <form style="display: inline;" method="get" action="/arduinoOff"><button class="btn btn-danger">OFF</button></form>
  </div>
  <br />
  <div style="padding: 5px;">
  Buzzer
  <form style="display: inline;" method="get" action="/buzzerOn"><button class="btn btn-success">ON</button></form>
  <form style="display: inline;" method="get" action="/buzzerOff"><button class="btn btn-danger">OFF</button></form>
  </div>
  <br />

  <div id="chart"></div>
</body>  
</html>)rawliteral";

void sendToArduino(String data);

void setupWiFi() {
  delay(500);
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/arduinoOn", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
    sendToArduino("a1");
  });

  server.on("/arduinoOff", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
    sendToArduino("a0");
    receiving = false;
    image.close();
    writing = false;
  });

  server.on("/buzzerOn", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
    sendToArduino("b1");
  });

  server.on("/buzzerOff", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
    sendToArduino("b0");
  });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
    sendToArduino("c");
    receiving = false;
    image.close();
    writing = false;
  });

  server.on("/image", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/image.jpg", "image/jpeg");
  });

  server.on("/data", HTTP_GET, [](AsyncWebServerRequest * request) {
    String data = "No Motion";
    if (receiving) {
      data = "Motion";
    }
    request->send_P(200, "text/plain", data.c_str());
  });

  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());

}

void setup() {
  Serial.begin(9600);
  SPIFFS.format();
  SPIFFS.begin();

  Wire.begin(2, 14);

  setupWiFi();

  Serial.println("Setup Done");
}

void sendToArduino(String data) {
  Wire.beginTransmission(0x08);
  Wire.write(data.c_str());
  Wire.endTransmission();
}

//recieves up to 30 characters at a time
String receiveFromArduino() {
  Wire.requestFrom(0x08, 32);
  char data[32];
  data[0] = '\0';
  for (int i = 0; Wire.available(); ++i) {
    data[i] = Wire.read();
    if (data[i] == '!') {
      data[i] = '\0';
      break;
    }
  }
  return data;
}

void writeToFile(uint8_t* data, unsigned int len) {
  image.write(data, len);
}

void recieveByte() {
  uint8_t len = min((uint16_t) 32, jpglen);
  Wire.requestFrom(0x08, (int) len);
  uint8_t data;
  for (int i = 0; i < 32 && Wire.available(); ++i) {
    data = Wire.read();
    writeToFile(&data, 1);
    //Serial.println(data);
  }
  jpglen -= len;
  Serial.print("Remaining length: "); Serial.println(jpglen);
}

void loop() {
  delay(80);
  if (!receiving) {
    String message = receiveFromArduino();
    //Serial.print("Message: "); Serial.println(message);
    jpglen = atoi(message.c_str());
    if (jpglen > 0) {
      Serial.print("Requested :"); Serial.print(jpglen); Serial.println(" of data");
      receiving = true;
      image = SPIFFS.open("/image.jpg", "w");
      if (!image) {
        Serial.println("Error opening file for writing");
        return;
      }
    }
  } else {
    if (jpglen > 0) {
      recieveByte();
      if (jpglen <= 0) {
        //Serial.print("Size: "); Serial.println(image.size());
        image.close();
        Serial.println("File Closed");
      }
    }
    else {
      Serial.println("Image Writing Done");
      writing = false;
      receiving = false;
    }
  }

  if (!writing && count == 0) {


    ++count;
  }
  /*
    //delay(100);
    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client) {
      return;
    }

    // Wait until the client sends some data
    Serial.println("new client");
    while (!client.available()) {
      delay(1);
    }
    if (!writing)
      client.println("<!DOCTYPE html><img src='image.jpg'><html>");
    delay(1000);
  */

  /*
    delay(100);
    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client) {
    return;
    }
    // Read the first line of the request
    String req = client.readStringUntil('\r');
    Serial.println(req);
    client.flush();

    // Prepare the response. Start with the common header:
    String s = "HTTP/1.1 200 OK\r\n";
    s += "Content-Type: text/html\r\n\r\n";
    s += "<!DOCTYPE HTML>\r\n<html>\r\n";
    s += "The message is:";
    s += String(x);
    s += "<br>"; // Go to the next line.
    s += "</html>\n";
    // Send the response to the client
    client.print(s);
    delay(1);
    Serial.println("Client disonnected");
    // The client will actually be disconnected
    // when the function returns and 'client' object is destroyed
  */
}
