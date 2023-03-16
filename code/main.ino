// importing libraries
#include <BLEPeripheral.h>
#include <SPI.h>

// Defining PIN button and LED
#define LED_PIN 10
#define BUTTON_PIN 12

// variabel for state management and message
int stateButton = 0;
int stateKeys = 0;

// variabel for counting time
unsigned long long startTime = 0;

// blePeripheral class
BLEPeripheral blePeripheral = BLEPeripheral();

// create service
BLEService communication = BLEService("19b10010-e8f2-537e-4f6c-d104768a1213");

// create characteristic

// requestMessage send request open door lock to central
BLECharacteristic requestMessage =
    BLECharacteristic("19b10010-e8f2-537e-4f6c-d104768a1214",
                      BLERead | BLENotify, "                    ");

// receiveMessage take encrypted message from central
BLECharacteristic receiveMessage =
    BLECharacteristic("19b10010-e8f2-537e-4f6c-d104768a1215",
                      BLERead | BLEWrite | BLENotify, "                    ");

// responseMessage send response for encryptedMessage to central
BLECharacteristic responseMessage =
    BLECharacteristic("19b10010-e8f2-537e-4f6c-d104768a1217",
                      BLERead | BLENotify, "                    ");

// debugging BLECharacteristic as serial print
// stateKeyMessage notification keys state (1 if request open door'lock and 0
// otherwise)
BLEBoolCharacteristic stateKeyMessage = BLEBoolCharacteristic(
    "19b10010-e8f2-537e-4f6c-d104768a1216", BLERead | BLENotify);

// lengthResponse give length of char*
BLEIntCharacteristic lengthResponse = BLEIntCharacteristic(
    "19b10010-e8f2-537e-4f6c-d104768a1218", BLERead | BLENotify);

void setup() {
  // set LED pin to output mode, button pin to input mode
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  // set advertised local name and service UUID
  // blePeripheral.setLocalName("NRF51822");
  // blePeripheral.setDeviceName("NRF51822");
  blePeripheral.setAppearance(1000);

  // set advertised UUID as indetifier keys
  blePeripheral.setAdvertisedServiceUuid(
      "19b10010-e8f2-537e-4f6c-d104768a1210");

  // adding service and characteristic
  blePeripheral.addAttribute(communication);
  blePeripheral.addAttribute(requestMessage);
  blePeripheral.addAttribute(receiveMessage);
  blePeripheral.addAttribute(stateKeyMessage);
  blePeripheral.addAttribute(responseMessage);
  blePeripheral.addAttribute(lengthResponse);

  // begin initialization
  blePeripheral.begin();
}

// function definition
char *enkripsi(char *message);
char *dekripsi(char *message);

void loop() {
  // status connection
  BLECentral central = blePeripheral.central();

  // connected to central
  if (central) {
    // set LED ON for user notification connection ack
    digitalWrite(LED_PIN, HIGH);

    // reset after connected for new processing
    char *saveMessage = "                    ";
    stateKeys = 0;
    requestMessage.setValue("                    ");
    stateKeyMessage.setValue(0);
    responseMessage.setValue("                    ");
    receiveMessage.setValue("                    ");
    lengthResponse.setValue(0);

    while (central.connected()) {
      // check if button state changing, user request open door
      if (stateButton != digitalRead(BUTTON_PIN)) {
        // take button value
        stateButton = digitalRead(BUTTON_PIN);

        // run if statebutton going HIGH
        if (stateButton == 1) {
          if (stateKeys == 0) {
            stateKeys = 1;
            requestMessage.setValue("tolong buka pintu dong hehehe");
            stateKeyMessage.setValue(1);
            startTime = millis();
          }
        }
      }

      // processing if button is pushed
      if (stateKeys == 1) {
        // check for 3 second after button pushed
        if ((millis() - startTime) > 3000) {
          // reset all value
          saveMessage = "                    ";
          stateKeys = 0;
          requestMessage.setValue("                    ");
          stateKeyMessage.setValue(0);
          responseMessage.setValue("                    ");
          receiveMessage.setValue("                    ");
          lengthResponse.setValue(0);
        }

        // convert const unsigned char * to char* message
        unsigned char *_unsignedMessage =
            const_cast<unsigned char *>(receiveMessage.value());
        char *_saveMessage = reinterpret_cast<char *>(_unsignedMessage);
        _saveMessage[receiveMessage.valueLength()] = '\0';

        // processing after get newMessage from central
        if (strcmp(_saveMessage, saveMessage) != 0) {

          // copy new saveMessage from _saveMessage
          saveMessage = (char *)malloc(receiveMessage.valueLength() + 1);
          strcpy(saveMessage, _saveMessage);

          // give response from central
          char *encryptedMessage = enkripsi(saveMessage);
          responseMessage.setValue(encryptedMessage);
          lengthResponse.setValue(strlen(encryptedMessage));
        }
      }
    }

    // set LED OFF for user notification connection terminated
    digitalWrite(LED_PIN, LOW);
  }
}

char *enkripsi(char *message) {
  // function menambahkan nilai 1 ASCII ke setiap char

  char *encryptedMessage;
  // take length string
  int lenghtChar = strlen(message);

  // copy string first
  // used for saveMessage didnt change
  encryptedMessage = (char *)malloc(lenghtChar + 1);
  strcpy(encryptedMessage, message);

  // processing +1 in ascii
  for (int i = 0; i < lenghtChar; i++) {
    encryptedMessage[i] = message[i] + 1;
  }
  encryptedMessage[lenghtChar] = '\0';

  return encryptedMessage;
}

char *dekripsi(char *message) {
  char *decryptedMessage;
  // function here
}
