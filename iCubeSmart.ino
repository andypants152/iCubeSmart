/*
 *
 *    Open Source iCubeSmart 3D8RGB LED Cube Firmware
 *
 *      8X8X8 Configuration
 *
 */

// Onboard LEDs on the yellow board. Blue is a power indicator only:
#define LED_Red PB8
#define LED_Green PB9

// TX/RX Ports for Serial UART:
#define RX PA10
#define TX PA9

// Onboard Buttons:
#define Key1 PC0  // Previous
#define Key2 PC1  // Next
#define Key3 PC2  // Speed+
#define Key4 PB3  // Speed-
#define Key5 PA14 // Run-Pause
#define Key6 PA13 // Cycle
#define Key7 PA11 // On-Off

// Onboard Switchs
#define SW1 PA1
#define SW2 PC3

// Shift Register (74HC245/MBI5024):
#define SPI_Clock PA5 // SPI Clock - Clock input terminal for data shift
#define SPI_MOSI PA7  // SPI MOSI - Serial-data input to the shift register
#define LE PC4        // LE - Data strobe input terminal (latch pin)
#define OE PC5        // OE - Output enable terminal

// Demultiplexer (74HC154):
#define DEMUX_A PB0
#define DEMUX_B PB1
#define DEMUX_C PB10
#define DEMUX_D PB11
#define DEMUX_ENABLE PA8 // Enable Pin

// Instance of Serial used for UART:
HardwareSerial Serial1(RX, TX);

// Retains the state of different buttons being pressed:
bool key1Pressed = false;
bool key2Pressed = false;
bool key3Pressed = false;
bool key4Pressed = false;
bool key5Pressed = false;
bool key6Pressed = false;
bool key7Pressed = false;

bool switch1 = false;
bool switch2 = false;

// Setup the hardware abstraction layer for the cube
struct Voxel
{
  byte x;
  byte y;
  byte z;
  bool r;
  bool g;
  bool b;
};

#define CUBE_WIDTH 8
#define CUBE_DEPTH 8
#define CUBE_HEIGHT 8

uint8_t cube[CUBE_WIDTH][CUBE_DEPTH][CUBE_HEIGHT][3];

void initializeCube()
{
  for (int x = 0; x < CUBE_WIDTH; x++)
  {
    for (int y = 0; y < CUBE_DEPTH; y++)
    {
      for (int z = 0; z < CUBE_HEIGHT; z++)
      {
        cube[x][y][z][0] = 0; // Red
        cube[x][y][z][1] = 0; // Green
        cube[x][y][z][2] = 0; // Blue
      }
    }
  }
}

void setVoxel(byte x, byte y, byte z, bool r, bool g, bool b)
{
  if (x < CUBE_WIDTH && y < CUBE_DEPTH && z < CUBE_HEIGHT)
  {
    cube[x][y][z][0] = r;
    cube[x][y][z][1] = g;
    cube[x][y][z][2] = b;
  }
}

void clearVoxel(byte x, byte y, byte z)
{
  setVoxel(x, y, z, 0, 0, 0);
}

void renderCube()
{
  for (int z = 0; z < CUBE_HEIGHT; z++)
  {
    switchToLayerZ(z); // Switch to the current layer

    uint16_t layerData[12] = {0}; // Initialize layer data

    // Pack the voxel data into the layerData array
    for (int x = 0; x < CUBE_WIDTH; x++)
    {
      for (int y = 0; y < CUBE_DEPTH; y++)
      {
        int ledIndex = y + (7 - x) * 8;
        int chip = ledIndex / 16; // which register (0-3)
        int bit = ledIndex % 16;  // which bit in that register

        bool r = cube[x][y][z][0];
        bool g = cube[x][y][z][1];
        bool b = cube[x][y][z][2];

        // Pack the RGB values into the layerData array
        if (r)
        {
          layerData[chip] |= (1 << bit); // Red channel occupies chips 0-3
        }
        if (b)
        {
          layerData[chip + 4] |= (1 << bit); // Green channel occupies chips 4-7
        }
        if (g)
        {
          layerData[chip + 8] |= (1 << bit); // Blue channel occupies chips 8-11
        }
      }
    }

    paintCurrentLayer(layerData); // Send the data to the shift registers
    delay(1000);
  }
}

void setup()
{
  // Disable JTAG to use PA14 as GPIO
  __HAL_RCC_AFIO_CLK_ENABLE();    // Enable AFIO clock
  __HAL_AFIO_REMAP_SWJ_DISABLE(); // Disable JTAG

  // Onboard LEDs:
  pinMode(LED_Red, OUTPUT);
  pinMode(LED_Green, OUTPUT);

  // Buttons:
  pinMode(Key1, INPUT_PULLUP);
  pinMode(Key2, INPUT_PULLUP);
  pinMode(Key3, INPUT_PULLUP);
  pinMode(Key4, INPUT_PULLUP);
  pinMode(Key5, INPUT_PULLUP);
  pinMode(Key6, INPUT_PULLUP);
  pinMode(Key7, INPUT_PULLUP);

  // Switches
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);

  // Shift Register:
  pinMode(SPI_Clock, OUTPUT);
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(LE, OUTPUT);
  pinMode(OE, OUTPUT);

  // Initialize demultiplexer which handles the layer that is being grounded
  pinMode(DEMUX_A, OUTPUT);
  pinMode(DEMUX_B, OUTPUT);
  pinMode(DEMUX_C, OUTPUT);
  pinMode(DEMUX_D, OUTPUT);
  pinMode(DEMUX_ENABLE, OUTPUT);

  // Open serial port and listen @ 115200 bps:
  Serial1.begin(115200);

  initializeCube();
}

void clearCurrentLayer()
{
  digitalWrite(OE, HIGH); // Disable output while shifting data
  digitalWrite(LE, LOW);  // Keep latch low until data is fully sent

  // Send 192 bits (12 MBI5024s × 16 bits each)
  for (int chip = 11; chip >= 0; chip--)
  {
    uint16_t data = 0x0000; // All LEDs off (MBI5024 is active-low)

    for (int i = 15; i >= 0; i--)
    { // Shift 16 bits per chip
      digitalWrite(SPI_Clock, LOW);
      digitalWrite(SPI_MOSI, (data & (1 << i)) ? HIGH : LOW);
      digitalWrite(SPI_Clock, HIGH);
    }
  }

  digitalWrite(LE, HIGH); // Latch data to outputs
  digitalWrite(LE, LOW);  // Reset latch
  digitalWrite(OE, LOW);  // Enable output
}

void paintCurrentLayer(uint16_t data[12])
{
  digitalWrite(OE, HIGH); // Disable output while shifting data
  digitalWrite(LE, LOW);  // Keep latch low until data is fully sent

  // Shift out 12 x 16-bit values (MSB first)
  for (int chip = 11; chip >= 0; chip--)
  { // Start from last chip to first
    for (int i = 15; i >= 0; i--)
    { // MSB first
      digitalWrite(SPI_Clock, LOW);
      digitalWrite(SPI_MOSI, (data[chip] & (1 << i)) ? HIGH : LOW);
      digitalWrite(SPI_Clock, HIGH);
    }
  }

  digitalWrite(LE, HIGH); // Latch data to outputs
  delayMicroseconds(1);   // Short delay for stability
  digitalWrite(LE, LOW);
  digitalWrite(OE, LOW); // Enable output
}

void switchToLayerZ(int z)
{
  z = z & 0x1F;
  digitalWrite(DEMUX_A, z & 1);
  digitalWrite(DEMUX_B, (z >> 1) & 1);
  digitalWrite(DEMUX_C, (z >> 2) & 1);
  digitalWrite(DEMUX_D, (z >> 3) & 1);
}

void loop()
{
  key1Pressed = digitalRead(Key1) == LOW;
  key2Pressed = digitalRead(Key2) == LOW;
  key3Pressed = digitalRead(Key3) == LOW;
  key4Pressed = digitalRead(Key4) == LOW;
  key5Pressed = digitalRead(Key5) == LOW;
  key6Pressed = digitalRead(Key6) == LOW;
  key7Pressed = digitalRead(Key7) == LOW;
  switch1 = digitalRead(SW1) == HIGH;
  switch2 = digitalRead(SW2) == HIGH;

  // uint16_t testPattern[12] = {0x0003, 0x0000, 0x0000, 0xFFFF,
  //                             0x0001, 0x0001, 0x0000, 0x0000,
  //                             0x0001, 0x0000, 0x0001, 0x0000};

  // paintCurrentLayer(testPattern);

  // // Example: Set some voxels
  setVoxel(0, 0, 5, 1, 0, 0); // Should be LED 56 (Red)
  setVoxel(7, 0, 6, 0, 1, 0); // Should be LED 0 (Green)
  setVoxel(0, 7, 0, 0, 0, 1); // Should be LED 63 (Blue)
  setVoxel(7, 7, 7, 1, 1, 1); // Should be LED 7 (White)

  // // Render the cube
  renderCube();
  delay(1000);

  Serial1.print("Key1: " + String(key1Pressed) + "\t");
  Serial1.print("Key2: " + String(key2Pressed) + "\t");
  Serial1.print("Key3: " + String(key3Pressed) + "\t");
  Serial1.print("Key4: " + String(key4Pressed) + "\t");
  Serial1.print("Key5: " + String(key5Pressed) + "\t");
  Serial1.print("Key6: " + String(key6Pressed) + "\t");
  Serial1.println("Key7: " + String(key7Pressed) + "\t");
  Serial1.print("SW1: " + String(switch1) + "\t");
  Serial1.println("SW2: " + String(switch2) + "\t");
}