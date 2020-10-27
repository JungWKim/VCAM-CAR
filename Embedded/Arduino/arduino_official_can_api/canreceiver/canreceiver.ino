// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

#include <CAN.h>

char rxBuffer;

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("CAN Receiver");

  // start the CAN bus at 500 kbps
  if (!CAN.begin(500E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
}

void loop() {
  // try to parse packet
    int packetSize = CAN.parsePacket();
//
//  if (packetSize) {
//    // received a packet
//    Serial.print("Received ");
//    Serial.print("packet with id 0x");
//    Serial.print(CAN.packetId(), HEX);
//
//    if (CAN.packetRtr()) {
//      Serial.print(" and requested length ");
//      Serial.println(CAN.packetDlc());
//    } else {
//      Serial.print(" and length ");
//      Serial.println(packetSize);

      // only print packet data for non-RTR packets
      if(CAN.available()) {
        rxBuffer = CAN.read();
        Serial.print((signed int)rxBuffer);
      }
      Serial.println();
      delay(1000);
//    }
//
//    Serial.println();
//  }
}
