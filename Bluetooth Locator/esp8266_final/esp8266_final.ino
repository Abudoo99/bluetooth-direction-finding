#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>

#define UUDF_DEBUG

SoftwareSerial mySerial(D7, D8); // RX, TX

const char* ssid = "SETUP-87ED";
const char* password = "banana4480drink";
const char* websocket_server_host = "192.168.0.11";
const uint16_t websocket_server_port = 8080;  // WebSocket server port

char uudf_data, rcv_buffer[150], payload[100];

char device_id[] = "locator_2";
int8_t err = 0;
bool stringComplete = false;

/* Structure containing UUDF parameters */
typedef struct UUDF_evt_params
{
  unsigned char instance_id[13];
  int rssi;
  int azimuth_angle;
  int elevation_angle;
  int channel;
  char anchor_id[13];
  char user_defined_str[10];
  int timestamp_ms;
  int per_evt_counter; 
} uudf_evt_params;

uudf_evt_params tag_params;

WebSocketsClient webSocket;

int8_t get_tag_params(char* buffer);
void webSocketEvent(WStype_t type, uint8_t * payload, size_t length);

void setup() 
{
  // Initializing Serial ports for UART
  Serial.begin(115200);
  mySerial.begin(115200);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      // Serial.println("Connecting to WiFi...");
  }

  Serial.println("Connected to WiFi");

  webSocket.begin(websocket_server_host, websocket_server_port, "/");  // WebSocket path
  webSocket.onEvent(webSocketEvent);
}

void loop() 
{
  webSocket.loop();

  if (mySerial.available()) 
  {
    //Read data from UART 
    uudf_data = mySerial.read();

    // Append received character to buffer
    strncat(rcv_buffer, &uudf_data, 1);

    // Checking if the current UUDF event has been completely received
    if(uudf_data == '\n') 
    {
      stringComplete = true;
    }
  }

  if(stringComplete)
  {
    if(strncmp(rcv_buffer, "+UUDF:", 6) == 0) 
    {
#ifdef UUDF_DEBUG
      Serial.printf("Received buffer: %s\n", rcv_buffer);
#endif

      // Get UUDF event parameters
      err = get_tag_params(rcv_buffer);
      if(err) 
      {
        Serial.print("Failed to get tag parameters\n");
      }

      sprintf(payload, "%s, %s, %d, %d, %d, %d", device_id, tag_params.instance_id, tag_params.rssi, 
              tag_params.azimuth_angle, tag_params.elevation_angle, tag_params.channel);
      
      webSocket.sendTXT(payload, sizeof(payload)); 
    }

    // Clear buffer once event is read
    memset(rcv_buffer, '\0', sizeof(rcv_buffer));
    stringComplete = false;
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WSc] Disconnected!\n");
            break;
        case WStype_CONNECTED:
            Serial.printf("[WSc] Connected to url: %s\n", payload);
            break;
        case WStype_TEXT:
            Serial.printf("[WSc] get text: %s\n", payload);
            // Do something with the received message
            break;
        case WStype_BIN:
            Serial.printf("[WSc] get binary length: %u\n", length);
            // Do something with the received binary data
            break;
    }
}

// Function to get UUDF event parameters of the tag
int8_t get_tag_params(char* buffer)
{
  int dummy;

  sscanf(buffer, "+UUDF:%13[^,],%d,%d,%d,%d,%d,\"%13[^\"]\",\"\",%d,%d",
           tag_params.instance_id,
           &tag_params.rssi, &tag_params.azimuth_angle, &tag_params.elevation_angle, &dummy,
           &tag_params.channel, tag_params.anchor_id, &tag_params.timestamp_ms, &tag_params.per_evt_counter);

#ifdef UUDF_DEBUG
    printf("Instance ID: ");
    for (int i = 0; i < 6; i++) {
        printf("%02X", tag_params.instance_id[i]);
    }
    printf("\nRSSI: %d\n", tag_params.rssi);
    printf("Azimuth Angle: %d\n", tag_params.azimuth_angle);
    printf("Elevation Angle: %d\n", tag_params.elevation_angle);
    printf("Channel: %d\n", tag_params.channel);
    printf("Anchor ID: %s\n", tag_params.anchor_id);
    // printf("User Defined String: %s\n", tag_params.user_defined_str);
    printf("Timestamp (ms): %d\n", tag_params.timestamp_ms);
    printf("Event Counter: %d\n", tag_params.per_evt_counter);
#endif
  
  return 0;
}
