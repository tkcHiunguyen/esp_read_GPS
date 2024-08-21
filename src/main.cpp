#include <SoftwareSerial.h>

// Define the RX and TX pins for the SoftwareSerial
const int RXPin = 4; // GPIO 4 (D2 on NodeMCU)
const int TXPin = 5; // GPIO 5 (D1 on NodeMCU)
const int GPSBaud = 38400;

// Create a software serial port
SoftwareSerial gpsSerial(RXPin, TXPin);

// Define the RX and TX pins for the hardware Serial2
const int UART2_RX = 13; // GPIO 13 (D7)
const int UART2_TX = 12; // GPIO 12 (D6)
const int UART2Baud = 38400;
bool length_valid = false;
SoftwareSerial lapSerial(UART2_RX, UART2_TX);

const int BUFFER_SIZE = 256;
uint8_t sendData[255];
uint8_t headerData[5];
uint8_t realData[255];
enum State {
      WAIT_HEADER1,
      WAIT_HEADER2,
      WAIT_CLASS,
      WAIT_ID,
      WAIT_LENGTH,
      WAIT_PAYLOAD
};
State state = WAIT_HEADER1;
uint8_t payloadLength = 0;
uint8_t payloadCounter = 0;
void combineArrays(uint8_t array1[], int size1, uint8_t array2[], int size2, uint8_t result[]) {
      for (int i = 0; i < size1; ++i) {
            result[i] = array1[i];
      }
      for (int i = 0; i < size2; ++i) {
            result[size1 + i] = array2[i];
      }
}
void setup() {
      Serial.begin(115200);
      gpsSerial.begin(GPSBaud);
      lapSerial.begin(UART2Baud);
}
void loop() {
      while (gpsSerial.available()) {
            uint8_t byte = gpsSerial.read();
            // Serial.println(byte);
            switch (state) {
            case WAIT_HEADER1:
                  if (byte == 0xB5) {
                        headerData[0] = byte;
                        state = WAIT_HEADER2;
                  } else {
                        memset(sendData, 0, BUFFER_SIZE); // Clear the sendData array
                  }
                  break;

            case WAIT_HEADER2:
                  if (byte == 0x62) {
                        headerData[1] = byte;
                        state = WAIT_CLASS;
                  } else {
                        state = WAIT_HEADER1;
                  }
                  break;

            case WAIT_CLASS:
                  if (byte == 0x01) { // UBX-NAV class
                        headerData[2] = byte;
                        state = WAIT_ID;
                  } else if (byte == 0x0A) {
                        headerData[2] = byte;
                        state = WAIT_ID;
                  } else {
                        state = WAIT_HEADER1;
                  }
                  break;

            case WAIT_ID:
                  if (byte == 0x07) { // NAV-PVT
                        payloadLength = 95;
                        headerData[3] = byte;
                        state = WAIT_LENGTH;
                  } else if (byte == 0x04) { // NAV-DOP
                        payloadLength = 21;
                        headerData[3] = byte;
                        state = WAIT_LENGTH;
                  } else if (byte == 0x20) { // NAV-TIME
                        payloadLength = 19;
                        headerData[3] = byte;
                        state = WAIT_LENGTH;
                  } else if (byte == 0x0B) { // MON2
                        payloadLength = 31;
                        headerData[3] = byte;
                        state = WAIT_LENGTH;
                  } else if (byte == 0x09) { // MON1
                        payloadLength = 63;
                        headerData[3] = byte;
                        state = WAIT_LENGTH;
                  } else {
                        state = WAIT_HEADER1;
                  }
                  break;
            case WAIT_LENGTH:
                  if ((headerData[3] == 0x07 && byte == 0x5C) || // NAV-PVT
                      (headerData[3] == 0x04 && byte == 0x12) || // NAV-DOP
                      (headerData[3] == 0x20 && byte == 0x10) || // NAV-TIME
                      (headerData[3] == 0x0B && byte == 0x1C) || // NAV-MON1
                      (headerData[3] == 0x09 && byte == 0x3C)) { // NAV-MON2
                        length_valid = true;
                        headerData[4] = byte;
                        state = WAIT_PAYLOAD;
                  } else {
                        state = WAIT_ID;
                  }
                  break;
            case WAIT_PAYLOAD:
                  if (payloadCounter < payloadLength) {
                        sendData[payloadCounter] = byte;
                        payloadCounter++;
                  }

                  if (payloadCounter == payloadLength) {

                        Serial.print("length: ");
                        Serial.println(payloadCounter + 5);
                        combineArrays(headerData, 5, sendData, payloadCounter, realData);
                        for (int i = 0; i < payloadCounter + 5; i++) {
                              // Serial.print(realData[i], HEX);
                              // Serial.print(" ");
                              // lapSerial.write(realData[i]);
                        }
                        // Serial.println("");
                        memset(sendData, 0, BUFFER_SIZE);
                        payloadCounter = 0;
                        payloadLength = 0;
                        state = WAIT_HEADER1;
                  }

                  break;
            }
      }
}
