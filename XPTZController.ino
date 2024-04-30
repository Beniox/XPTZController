#include <XboxSeriesXControllerESP32_asukiaaa.hpp>

// Customise the following values to match your setup

#define XBOX_ADDRESS "11:22:33:44:55:66" // Required to replace with your xbox address
#define RXPIN 20
#define TXPIN 21

// End of customisation

XboxSeriesXControllerESP32_asukiaaa::Core
    xboxController(XBOX_ADDRESS);

#define TRESHOLD 2500

// Pan/Tilt
byte panTilt[9] = {0x81, 0x01, 0x06, 0x01, 0x00, 0x00, 0x03, 0x03, 0xFF};
byte panUp[9] = {0x81, 0x01, 0x06, 0x01, 0x00, 0x00, 0x03, 0x01, 0xFF};        // 8x 01 06 01 0p 0t 03 01 ff
byte panDown[9] = {0x81, 0x01, 0x06, 0x01, 0x00, 0x00, 0x03, 0x02, 0xFF};      // 8x 01 06 01 0p 0t 03 02 ff
byte panLeft[9] = {0x81, 0x01, 0x06, 0x01, 0x00, 0x00, 0x01, 0x03, 0xFF};      // 8x 01 06 01 0p 0t 01 03 ff
byte panRight[9] = {0x81, 0x01, 0x06, 0x01, 0x00, 0x00, 0x02, 0x03, 0xFF};     // 8x 01 06 01 0p 0t 02 03 ff
byte panUpLeft[9] = {0x81, 0x01, 0x06, 0x01, 0x00, 0x00, 0x01, 0x01, 0xFF};    // 8x 01 06 01 0p 0t 01 01 ff
byte panUpRight[9] = {0x81, 0x01, 0x06, 0x01, 0x00, 0x00, 0x02, 0x01, 0xFF};   // 8x 01 06 01 0p 0t 02 01 ff
byte panDownLeft[9] = {0x81, 0x01, 0x06, 0x01, 0x00, 0x00, 0x01, 0x02, 0xFF};  // 8x 01 06 01 0p 0t 01 02 ff
byte panDownRight[9] = {0x81, 0x01, 0x06, 0x01, 0x00, 0x00, 0x02, 0x02, 0xFF}; // 8x 01 06 01 0p 0t 02 02 ff

byte panStop[9] = {0x81, 0x01, 0x06, 0x01, 0x09, 0x09, 0x03, 0x03, 0xFF}; // Camera Stop
byte panTiltPosReq[5] = {0x81, 0x09, 0x06, 0x12, 0xff};

// Zoom
// Tele: 8x 01 04 07 2p ff
// Wide: 8x 01 04 07 2p ff
byte zoomCommand[6] = {0x81, 0x01, 0x04, 0x07, 0x2F, 0xff}; // 8x 01 04 07 2p ff
byte zoomIn[6] = {0x81, 0x01, 0x04, 0x07, 0x2F, 0xff};      // 8x 01 04 07 2p ff
byte zoomOut[6] = {0x81, 0x01, 0x04, 0x07, 0x3F, 0xff};     // 8x 01 04 07 3p ff
byte zoomStop[6] = {0x81, 0x01, 0x04, 0x07, 0x00, 0xff};
byte zoomDirect[9] = {0x81, 0x01, 0x04, 0x47, 0x00, 0x00, 0x00, 0x00, 0xff}; // 8x 01 04 47 0p 0q 0r 0s ff pqrs: zoomCommand position
byte zoomPosReq[5] = {0x81, 0x09, 0x04, 0x47, 0xff};

byte turnOn[6] = {0x81, 0x01, 0x04, 0x00, 0x03, 0xFF};

byte setMemory[6] = {0x81, 0x01, 0x04, 0x3F, 0x02, 0xFF};    // 8x 01 04 3F 02 ff
byte recallMemory[6] = {0x81, 0x01, 0x04, 0x3F, 0x03, 0xFF}; // 8x 01 04 3F 03 ff

auto leftXS = 0;
auto leftYS = 0;

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

  //  print the zoom command
  // Serial.println("Zoom command: ");
  // for (int i = 0; i < sizeof(zoomCommand); i++)
  // {
  //   Serial.print(zoomCommand[i], HEX);
  //   Serial.print(" ");
  // }
  // Serial.println();

  sendViscaPacket(zoomCommand, sizeof(zoomCommand));
}

bool zooming = false;
bool moving = false;
void loop()
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
  }
  else
  {
    // Input

    // left joycon
    auto leftX = xboxController.xboxNotif.joyLHori;
    auto leftY = xboxController.xboxNotif.joyLVert;

    // The joysick values are between 0 and 65535

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
      // ESP.deepSleep(0);
      return;
    }

    // recall memory 1 to 4 by pressing the dpad
    if (xboxController.xboxNotif.dpadUp)
    {
      Serial.println("Recall memory 1");
      recallMemory[4] = 0x01;
      sendViscaPacket(recallMemory, sizeof(recallMemory));
      return;
    }
    if (xboxController.xboxNotif.dpadRight)
    {
      Serial.println("Recall memory 2");
      recallMemory[4] = 0x02;
      sendViscaPacket(recallMemory, sizeof(recallMemory));
      return;
    }
    if (xboxController.xboxNotif.dpadDown)
    {
      Serial.println("Recall memory 3");
      recallMemory[4] = 0x03;
      sendViscaPacket(recallMemory, sizeof(recallMemory));
      return;
    }
    if (xboxController.xboxNotif.dpadLeft)
    {
      Serial.println("Recall memory 4");
      recallMemory[4] = 0x04;
      sendViscaPacket(recallMemory, sizeof(recallMemory));
      return;
    }

    // set the values to zero if they are below the threshold
    if (abs(leftX - 32541) < TRESHOLD)
    {
      leftX = 32541;
    }
    if (abs(leftY - 32541) < TRESHOLD)
    {
      leftY = 32541;
    }

    if (leftX > 32541)
    {
      // move the camera to the right

      // normalize the values between 0x01 and 0x14
      leftXS = map(leftX, 32541, 65535, 1, 18);

      if (leftY > 32541)
      {
        // move the camera to the right and up
        leftYS = map(leftY, 32541, 65535, 1, 18);
        panUpRight[4] = leftXS;
        panUpRight[5] = leftYS;
        sendViscaPacket(panUpRight, sizeof(panUpRight));
      }
      else if (leftY < 32541)
      {
        // move the camera to the right and down
        leftYS = map(leftY, 0, 32541, 18, 1);
        panDownRight[4] = leftXS;
        panDownRight[5] = leftYS;
        sendViscaPacket(panDownRight, sizeof(panDownRight));
      }
      else
      {
        // move the camera to the right
        panRight[4] = leftXS;
        sendViscaPacket(panRight, sizeof(panRight));
      }
      moving = true;
    }
    else if (leftX < 32541)
    {
      // move the camera to the left

      // normalize the values between 0x01 and 0x14
      leftXS = map(leftX, 0, 32541, 18, 1);

      if (leftY > 32541)
      {
        // move the camera to the left and up
        leftYS = map(leftY, 32541, 65535, 1, 18);
        panUpLeft[4] = leftXS;
        panUpLeft[5] = leftYS;
        sendViscaPacket(panUpLeft, sizeof(panUpLeft));
      }
      else if (leftY < 32541)
      {
        // move the camera to the left and down
        leftYS = map(leftY, 0, 32541, 18, 1);
        panDownLeft[4] = leftXS;
        panDownLeft[5] = leftYS;
        sendViscaPacket(panDownLeft, sizeof(panDownLeft));
      }
      else
      {
        // move the camera to the left
        panLeft[4] = leftXS;
        sendViscaPacket(panLeft, sizeof(panLeft));
      }
      moving = true;
    }
    else
    {
      if (leftY > 32541)
      {
        // move the camera up
        leftYS = map(leftY, 32541, 65535, 1, 18);
        panUp[5] = leftYS;
        sendViscaPacket(panUp, sizeof(panUp));
        moving = true;
      }
      else if (leftY < 32541)
      {
        // move the camera down
        leftYS = map(leftY, 0, 32541, 1, 18);
        panDown[5] = leftYS;
        sendViscaPacket(panDown, sizeof(panDown));
        moving = true;
      }
      else
      {
        if (moving)
        {
          // stop moving
          sendViscaPacket(panStop, sizeof(panStop));
          Serial.println("Stop moving");
          moving = false;
        }
      }
    }

    // Zoom via the right joystick
    auto rightY = xboxController.xboxNotif.joyRVert;

    if (abs(rightY - 32541) < TRESHOLD)
    {
      rightY = 32541;
    }

    if (rightY > 32541)
    {
      // zoom out
      int zoomSpeed = map(rightY, 32541, 65535, 0, 15);
      sendZoomPacket(0x20, zoomSpeed);
      zooming = true;
    }
    else if (rightY < 32541)
    {
      // zoom in
      int zoomSpeed = map(rightY, 32541, 0, 0, 15);
      sendZoomPacket(0x30, zoomSpeed);
      zooming = true;
    }
    else
    {
      if (zooming)
      {
        // stop zooming
        sendViscaPacket(zoomStop, sizeof(zoomStop));
        Serial.println("Stop zooming");
        zooming = false;
      }
    }
  }
  delay(50);
}
