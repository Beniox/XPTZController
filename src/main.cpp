#include <XboxSeriesXControllerESP32_asukiaaa.hpp>
// #include "env.h"

// Customise the following values to match your setup

// only define if not defined in env.h
#ifndef XBOX_ADDRESS
#define XBOX_ADDRESS "11:22:33:44:55:66" // Required to replace with your xbox address
#endif
#define RXPIN 20
#define TXPIN 21
#define TRESHOLD 2500
#define trigLTTreshold 0x200

// End of customisation

// Only one xbox controller
// XboxSeriesXControllerESP32_asukiaaa::Core
//     xboxController(XBOX_ADDRESS);

// any xbox controller
XboxSeriesXControllerESP32_asukiaaa::Core xboxController;

// ############################### Visca Area ###############################

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
      lastPacket[i] = packet[i]; // Update the last packet
    }
  }
  // sleep(5); // to avoid sending packets too fast;
  delay(20);
}

void vibrateLeft()
{
  XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase repo;
  repo.v.select.center = false;
  repo.v.select.right = false;
  repo.v.select.left = true;
  repo.v.power.left = 40;
  repo.v.timeActive = 10;
  Serial.println("run left 30\% power in half second");
  xboxController.writeHIDReport(repo);
}

void vibrateRight()
{
  XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase repo;
  repo.v.select.center = false;
  repo.v.select.right = true;
  repo.v.select.left = false;
  repo.v.power.left = 40;
  repo.v.timeActive = 10;
  Serial.println("run left 30\% power in half second");
  xboxController.writeHIDReport(repo);
}

// ############################### Connection Area ###############################

bool connect()
{
  xboxController.onLoop();
  if (!xboxController.isConnected())
  {
    Serial.println("not connected");
    if (xboxController.getCountFailedConnection() > 2)
    {
      ESP.restart();
    }
    return false;
  }
  // Ready

  if (xboxController.isWaitingForFirstNotification())
  {
    Serial.println("waiting for first notification");
    return false;
  }
  return true;
}

// ############################### Button Area ###############################

byte turnOn[6] = {0x81, 0x01, 0x04, 0x00, 0x03, 0xFF};

void handleButtons()
{
  // turn on the PTZ
  if (xboxController.xboxNotif.btnXbox)
  {
    Serial.println("turnOn");
    sendViscaPacket(turnOn, sizeof(turnOn));
    return;
  }

  // restart the esp32 by pressing the select and start button at the same time
  if (xboxController.xboxNotif.btnSelect && xboxController.xboxNotif.btnStart)
  {
    Serial.println("Restarting");
    ESP.restart();
    return;
  }
}

// ############################### Preset Area ###############################

byte recallMemory[7] = {0x81, 0x01, 0x04, 0x3F, 0x02, 0x00, 0xFF}; // 8x 01 04 3F 03 ff

void handleDPad()
{

  // recall memory 1 to 4 by pressing the dpad
  if (xboxController.xboxNotif.btnDirUp)
  {
    Serial.println("Recall memory 1");
    recallMemory[5] = 0x01;
    sendViscaPacket(recallMemory, sizeof(recallMemory));
    // vibrate();
    return;
  }
  if (xboxController.xboxNotif.btnDirRight)
  {
    Serial.println("Recall memory 2");
    recallMemory[5] = 0x02;
    sendViscaPacket(recallMemory, sizeof(recallMemory));
    return;
  }
  if (xboxController.xboxNotif.btnDirDown)
  {
    Serial.println("Recall memory 3");
    recallMemory[5] = 0x03;
    sendViscaPacket(recallMemory, sizeof(recallMemory));
    return;
  }
  if (xboxController.xboxNotif.btnDirLeft)
  {
    Serial.println("Recall memory 4");
    recallMemory[5] = 0x04;
    sendViscaPacket(recallMemory, sizeof(recallMemory));
    return;
  }
}

byte setMemory[7] = {0x81, 0x01, 0x04, 0x3F, 0x01, 0x00, 0xFF}; // 8x 01 04 3F 02 ff
bool released = false;
void handleABXY()
{

  if (xboxController.xboxNotif.btnA)
  {
    // preset 100
    setMemory[5] = 0x64;
  }
  else if (xboxController.xboxNotif.btnB)
  {
    // preset 101
    setMemory[5] = 0x65;
  }
  else if (xboxController.xboxNotif.btnX)
  {
    // preset 102
    setMemory[5] = 0x66;
  }
  else if (xboxController.xboxNotif.btnY)
  {
    // preset 103
    setMemory[5] = 0x67;
  }
  else
  {
    released = true;
    return;
  }

  bool rBack = xboxController.xboxNotif.btnRB;
  if (rBack)
  {
    // Save the current position to a custom preset
    Serial.println("preset set");
    setMemory[4] = 0x01;

    if (released)
    {
      XboxSeriesXHIDReportBuilder_asukiaaa::ReportBase repo;
      repo.v.select.center = true;
      repo.v.select.right = false;
      repo.v.select.left = false;
      repo.v.power.left = 40;
      repo.v.timeActive = 10;
      Serial.println("run left 30\% power in half second");
      xboxController.writeHIDReport(repo);
      released = false;
    }
  }
  else
  {
    // change to recall
    Serial.println("preset recall");
    setMemory[4] = 0x02;
  }

  sendViscaPacket(setMemory, sizeof(setMemory));
}

byte fastMemory[7] = {0x81, 0x01, 0x04, 0x3F, 0x02, 0x70, 0xFF};
bool saved = false;
// when the left trigger button is presed, save the current position to a fast preset, when released recall the fast preset
void fastPreset()
{
  if (xboxController.xboxNotif.trigLT > trigLTTreshold)
  {
    if (!saved)
    {
      // Save the current position to a custom preset
      Serial.println("fast preset set");
      vibrateLeft();
      fastMemory[4] = 0x01;
      sendViscaPacket(fastMemory, sizeof(fastMemory));
      saved = true;
    }
  }
  else
  {
    if (saved)
    {
      // recall the fast preset
      Serial.println("fast preset recall");
      fastMemory[4] = 0x02;
      sendViscaPacket(fastMemory, sizeof(fastMemory));
      saved = false;
    }
  }
}

// ############################### Zoom Area ###############################

byte zoomCommand[6] = {0x81, 0x01, 0x04, 0x07, 0x2F, 0xff}; // 8x 01 04 07 2p ff
uint8_t lastZoomSpeed = 0x00;                               // Store the last speed to avoid sending the same packet
void handleZoom()
{
  // Zoom via the right joystick
  auto rightY = xboxController.xboxNotif.joyRVert;

  // Threshold
  if (abs(rightY - 32541) < TRESHOLD)
  {
    rightY = 32541;
  }

  uint8_t zoomSpeed = (uint8_t)map(abs(rightY - 32541), 0, 32541, 0, 15);

  if (zoomSpeed == lastZoomSpeed)
  {
    // we are assuming that the joystick is not moving
    return;
  }
  lastZoomSpeed = zoomSpeed;

  byte zoomDir = 0x00;

  if (rightY > 32541)
  {
    zoomDir = 0x20; // zoom out
  }
  else if (rightY < 32541)
  {
    zoomDir = 0x30; // zoom in
  }

  uint8_t zoomDirSpeed = (uint8_t)zoomDir + zoomSpeed;
  zoomCommand[4] = zoomDirSpeed;

  sendViscaPacket(zoomCommand, sizeof(zoomCommand));
}

// ############################### Move Area ###############################

byte move[9] = {0x81, 0x01, 0x06, 0x01, 0x00, 0x00, 0x03, 0x03, 0xFF};
uint8_t lastSpeedX = 0x00; // Store the last speed to avoid sending the same packet
uint8_t lastSpeedY = 0x00; // Store the last speed to avoid sending the same packet
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

  // map the joystick values to the speed values
  uint8_t speedX = (uint8_t)map(abs(leftX - 32541), 0, 32541, 0, 24);
  uint8_t speedY = (uint8_t)map(abs(leftY - 32541), 0, 32541, 0, 20);

  // We compare the speed instead of the joystick values, because the value could change slightly without that the speed changed
  if (speedX == lastSpeedX && speedY == lastSpeedY)
  {
    // we are assuming that the joystick is not moving
    return;
  }

  // the joystick is in a different position

  lastSpeedX = speedX;
  lastSpeedY = speedY;

  move[4] = speedX;
  move[5] = speedY;

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

void setup()
{
  Serial.begin(9600);

  Serial.println("Starting NimBLE Client");
  xboxController.begin();
  pinMode(2, OUTPUT);

  Serial1.begin(9600, SERIAL_8N1, RXPIN, TXPIN);
}

void loop()
{
  // if (!connect())
  // {
  //   return;
  // }

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

  handleButtons();
  handleMove();
  handleZoom();
  handleDPad();
  handleABXY();
  fastPreset();

  delay(5);
}