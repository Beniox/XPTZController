#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
#include "env.h"

// Customise the following values to match your setup

// only define if not defined in env.h
#ifndef XBOX_ADDRESS
#define XBOX_ADDRESS "11:22:33:44:55:66" // Required to replace with your xbox address
#endif
#define RXPIN 20
#define TXPIN 21

// End of customisation

XboxSeriesXControllerESP32_asukiaaa::Core
    xboxController(XBOX_ADDRESS);

#define TRESHOLD 2500

byte zoomCommand[6] = {0x81, 0x01, 0x04, 0x07, 0x2F, 0xff}; // 8x 01 04 07 2p ff
byte turnOn[6] = {0x81, 0x01, 0x04, 0x00, 0x03, 0xFF};
byte setMemory[6] = {0x81, 0x01, 0x04, 0x3F, 0x02, 0xFF};    // 8x 01 04 3F 02 ff
byte recallMemory[6] = {0x81, 0x01, 0x04, 0x3F, 0x03, 0xFF}; // 8x 01 04 3F 03 ff

void setup()
{
  Serial.begin(9600);

  Serial.println("Starting NimBLE Client");
  xboxController.begin();
  pinMode(2, OUTPUT);

  Serial1.begin(9600, SERIAL_8N1, RXPIN, TXPIN);
}

byte lastPacket[9] = {0}; // Array to store the last sent packet
void sendViscaPacket(byte *packet, int byteSize)
{
  // Compare the current packet with the last packet
  bool isSamePacket = true;
  for (int i = 0; i < byteSize; i++)
  {
    if (packet[i] != lastPacket[i])
    {
      isSamePacket = false;
      break;
    }
  }

  // If the packets are different, send the current packet
  if (!isSamePacket)
  {
    Serial.println("packet: ");
    for (int i = 0; i < byteSize; i++)
    {
      Serial1.write(packet[i]);
      Serial.print(packet[i], HEX);
      Serial.print(" ");
      lastPacket[i] = packet[i]; // Update the last packet
    }
    Serial.println();
  }
}

void sendZoomPacket(byte zoomDir, int zoomSpeed)
{
  uint8_t zoomDirSpeed = (uint8_t)zoomDir + zoomSpeed;
  zoomCommand[4] = zoomDirSpeed;

  sendViscaPacket(zoomCommand, sizeof(zoomCommand));
}

bool moving = false;
void loop()
{
  connect();

  // turn on the PTZ
  if (xboxController.xboxNotif.btnXbox)
  {
    Serial.println("turnOn");
    sendViscaPacket(turnOn, sizeof(turnOn));
    return;
  }

  // restart the esp32 by pressing the x
  if (xboxController.xboxNotif.btnSelect && xboxController.xboxNotif.btnStart)
  {
    Serial.println("Restarting");
    // ESP.restart();
    return;
  }

  handleMove();
  handleZoom();
  handleDPad();

  delay(50);
}

void connect()
{
  xboxController.onLoop();
  if (!xboxController.isConnected())
  {
    Serial.println("not connected");
    if (xboxController.getCountFailedConnection() > 2)
    {
      ESP.restart();
    }
    return;
  }
  // Ready

  if (xboxController.isWaitingForFirstNotification())
  {
    Serial.println("waiting for first notification");
    return;
  }
}

void handleDPad()
{

  // recall memory 1 to 4 by pressing the dpad
  if (xboxController.xboxNotif.btnDirUp)
  {
    Serial.println("Recall memory 1");
    recallMemory[4] = 0x01;
    sendViscaPacket(recallMemory, sizeof(recallMemory));
    // vibrate();
    return;
  }
  if (xboxController.xboxNotif.btnDirRight)
  {
    Serial.println("Recall memory 2");
    recallMemory[4] = 0x02;
    sendViscaPacket(recallMemory, sizeof(recallMemory));
    return;
  }
  if (xboxController.xboxNotif.btnDirDown)
  {
    Serial.println("Recall memory 3");
    recallMemory[4] = 0x03;
    sendViscaPacket(recallMemory, sizeof(recallMemory));
    return;
  }
  if (xboxController.xboxNotif.btnDirLeft)
  {
    Serial.println("Recall memory 4");
    recallMemory[4] = 0x04;
    sendViscaPacket(recallMemory, sizeof(recallMemory));
    return;
  }
}

bool zooming = false;
void handleZoom()
{
  // Zoom via the right joystick
  auto rightY = xboxController.xboxNotif.joyRVert;

  if (abs(rightY - 32541) < TRESHOLD)
  {
    rightY = 32541;
  }

  uint8_t zoomSpeed = (uint8_t)map(abs(rightY - 32541), 0, 32541, 0, 15);
  byte zoomDir = 0x00;

  if (rightY > 32541)
  {
    // zoom out
    zoomDir = 0x20;
    zooming = true;
    sendZoomPacket(zoomDir, zoomSpeed);
  }
  else if (rightY < 32541)
  {
    // zoom in
    zoomDir = 0x30;
    zooming = true;
    sendZoomPacket(zoomDir, zoomSpeed);
  }
  else if (zooming)
  {
    // stop zooming
    zooming = false;
    sendZoomPacket(zoomDir, 0);
    return;
  }
}

byte move[9] = {0x81, 0x01, 0x06, 0x01, 0x00, 0x00, 0x03, 0x03, 0xFF};
void handleMove()
{
  // left joycon
  auto leftX = xboxController.xboxNotif.joyLHori;
  auto leftY = xboxController.xboxNotif.joyLVert;

  // The joysick values are between 0 and 65535
  // set the values to zero if they are below the threshold
  if (abs(leftX - 32541) < TRESHOLD)
  {
    leftX = 32541;
  }
  if (abs(leftY - 32541) < TRESHOLD)
  {
    leftY = 32541;
  }

  move[4] = (uint8_t)map(abs(leftX - 32541), 0, 32541, 0, 24);
  move[5] = (uint8_t)map(abs(leftY - 32541), 0, 32541, 0, 20);

  if (leftX < 32541)
  {
    move[6] = 0x01;
  }
  else if (leftX > 32541)
  {
    move[6] = 0x02;
  }
  else
  {
    move[6] = 0x03;
  }

  if (leftY < 32541)
  {
    move[7] = 0x01;
  }
  else if (leftY > 32541)
  {
    move[7] = 0x02;
  }
  else
  {
    move[7] = 0x03;
  }

  sendViscaPacket(move, sizeof(move));
}