// importing libraries
#include <AESLib.h>
#include <BLEPeripheral.h>
#include <SPI.h>

// Defining PIN button and LED
#define LED_PIN 3
#define BUTTON_PIN 16

// blePeripheral class
BLEPeripheral blePeripheral = BLEPeripheral();

// create service and characteristic for auth
BLEService communication = BLEService("19b10010-e8f2-537e-4f6c-d104768a1213");
BLECharacteristic receiveMessage = BLECharacteristic(
                                     "19b10010-e8f2-537e-4f6c-d104768a1215",
                                     BLERead | BLEWrite | BLENotify | BLEBroadcast, "                    ");
BLECharacteristic responseMessage =
  BLECharacteristic("19b10010-e8f2-537e-4f6c-d104768a1217",
                    BLERead | BLENotify, "                    ");

// create service and characteristic for broadcast connectable
BLEService broadService = BLEService("feaa");
BLECharacteristic broadcastMessage =
  BLECharacteristic("feab", BLERead | BLEBroadcast, 9);

// Defining AES Class from library
AESLib aesLib;

// AES Encryption Key, secret key for encrypt and decrpyt
byte aes_key[] = {0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
                  0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x32
                 };

// AES Initialization Vector, company's personal key for encrypt and decrpyt
byte aes_iv[N_BLOCK] = {0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
                        0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31
                       };

// variabel for state management and message
int stateButton = 0;              // 0->unpushed, viseversa
int stateKeys = 0;                // 0->unconnectable, viseversa
unsigned long long startTime = 0; // Timeout counter

const unsigned char device_template[4] = {0x53, 0x41, 0x43, 0x5F};
const unsigned char device_id[4] = {0x30, 0x30, 0x30, 0x31};

unsigned char serviceData[12];

void setup() {
  // set LED pin to output mode, button pin to input mode
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  // set advertised local name and service UUID
  blePeripheral.setLocalName((char *)device_id);
  blePeripheral.setDeviceName((char *)device_id);

  // adding service and characteristic auth
  blePeripheral.addAttribute(communication);
  blePeripheral.addAttribute(receiveMessage);
  blePeripheral.addAttribute(responseMessage);

  // adding service and characteristic broadcastMessage connectable
  blePeripheral.addAttribute(broadService);
  blePeripheral.addAttribute(broadcastMessage);

  for (int i = 0; i < 8; i++) {
    if (i < 4) {
      serviceData[i] = device_template[i];
    } else {
      serviceData[i] = device_id[i - 4];
    }
  }
  serviceData[8] = 0x30; // Unconnectable
  serviceData[9] = 0x00; // Reserved for future use, must be: 0x00
  serviceData[10] = 0x00;
  broadcastMessage.setValue(serviceData, 11);
  blePeripheral.setAdvertisedServiceUuid("feaa");

  // initialization BLE
  blePeripheral.begin();
  broadcastMessage.broadcast();

  // initialization AESLib
  aesLib.gen_iv(aes_iv);
  aesLib.set_paddingmode((paddingMode)0);
}

// General initialization vector
byte enc_iv[N_BLOCK] = {0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
                        0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31
                       };
byte enc_iv_to[N_BLOCK] = {0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
                           0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31
                          };
byte enc_iv_from[N_BLOCK] = {0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31,
                             0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31, 0x31
                            };

void loop() {
  //  // reset all variabel after booting
  //  stateKeys = 0;
  //  receiveMessage.setValue("                    ");
  //  responseMessage.setValue("                    ");

  //  while (1) {
  BLECentral central = blePeripheral.central();

  // check if button state changing, user make NRF51822 connectable
  if (stateButton != digitalRead(BUTTON_PIN)) {
    // take button value
    stateButton = digitalRead(BUTTON_PIN);
    // run if statebutton going HIGH
    if (stateButton == 1) {
      if (stateKeys == 0) {
        // give statement to NRF its connectable
        stateKeys = 1;

        // give notification to PSOC connectable
        for (int i = 0; i < 8; i++) {
          if (i < 4) {
            serviceData[i] = device_template[i];
          } else {
            serviceData[i] = device_id[i - 4];
          }
        }
        serviceData[8] = 0x31; // Connectable
        serviceData[9] = 0x00; // Reserved for future use, must be: 0x00
        serviceData[10] = 0x00;
        broadcastMessage.setValue(serviceData, 11);

        // take time start connectable
        startTime = millis();
      }
    }
  }

  // stateKeys -> 0 unconnectable, stateKeys -> connectable
  if (stateKeys == 1) {

    // connected to central
    if (central) {
      // set LED ON for user notification connection ack
      digitalWrite(LED_PIN, HIGH);

      // reset to init value after connected
      char resultMessage[16] = {0};
      receiveMessage.setValue("                    ");
      responseMessage.setValue("                    ");

      unsigned long long startTime_auth = millis(); // Timeout counter

      // auth
      while (central.connected()) {

        // processing after get newMessage from central
        if (receiveMessage.written()) {

          // take chippertext from central
          unsigned char *_chippertext =
            const_cast<unsigned char *>(receiveMessage.value());
          char *chippertext = reinterpret_cast<char *>(_chippertext);
          chippertext[receiveMessage.valueLength()] = '\0';

          // decryption message
          memcpy(enc_iv, enc_iv_from, sizeof(enc_iv_from));
          aesLib.decrypt((unsigned char *)chippertext, strlen(chippertext),
                         (byte *)resultMessage, aes_key, sizeof(aes_key),
                         enc_iv);
          responseMessage.setValue((char *)resultMessage);
        }

        if ((millis() - startTime_auth) > 5000) {
          central.disconnect();
        }
      }

      // event central disconnect
      digitalWrite(LED_PIN, LOW); // user notification connection terminatte
      stateKeys = 0;              // state keys for unconnectable

      // send notification to PSOC unconnectable
      for (int i = 0; i < 8; i++) {
        if (i < 4) {
          serviceData[i] = device_template[i];
        } else {
          serviceData[i] = device_id[i - 4];
        }
      }
      serviceData[8] = 0x30; // Unconnectable
      serviceData[9] = 0x00; // Reserved for future use, must be: 0x00
      serviceData[10] = 0x00;
      broadcastMessage.setValue(serviceData, 11);
    }

    // check timeout time
    if ((millis() - startTime) > 3000) {
      // make NRF unconnectable
      stateKeys = 0;

      // send notification to PSOC unconnectable
      for (int i = 0; i < 8; i++) {
        if (i < 4) {
          serviceData[i] = device_template[i];
        } else {
          serviceData[i] = device_id[i - 4];
        }
      }
      serviceData[8] = 0x30; // Unconnectable
      serviceData[9] = 0x00; // Reserved for future use, must be: 0x00
      serviceData[10] = 0x00;
      broadcastMessage.setValue(serviceData, 11);
    }

  }
  else {
    //     peripheral always disconnect until user push button
    central.disconnect();
  }
  //  }
}
