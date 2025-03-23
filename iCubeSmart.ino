/*
 *  Open Source iCubeSmart 3D8RGB LED Cube Firmware
 *
 *  8×8×8 Configuration, 24-bit color
 *
 */

 #include <SPI.h>

// Pin definitions
#define LED_Red PB8
#define LED_Green PB9

#define RX PA10
#define TX PA9

#define Key1 PC0  // Previous
#define Key2 PC1  // Next
#define Key3 PC2  // Speed+
#define Key4 PB3  // Speed-
#define Key5 PA14 // Run-Pause
#define Key6 PA13 // Cycle
#define Key7 PA11 // On-Off

#define SW1 PA1
#define SW2 PC3

// #define SPI_Clock PA5 // Hardware SPI clock (if not using SPI mode, used with digitalWrite)
// #define SPI_MOSI PA7  // Hardware SPI MOSI (if not using SPI mode, used with digitalWrite)
SPIClass mySPI(PA7, PA6, PA5, PA4);

#define LE PC4        // Latch pin for the shift registers
#define OE PC5        // Output enable for the shift registers

#define DEMUX_A PB0
#define DEMUX_B PB1
#define DEMUX_C PB10
#define DEMUX_D PB11
#define DEMUX_ENABLE PA8 // Demux enable

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

// We set the timer overflow to roughly 8 µs per render pulse
//(2040 for 61Hz cube)
uint32_t timerInterval = 10; // microseconds

HardwareTimer *timer = nullptr;

bool debugEnabled = false;

// Buffer for incoming serial command text
static char inputBuffer[64];
static size_t bufIndex = 0;


///////////////////////////////////////////////////////////////////////////////
// LEDCube Class
///////////////////////////////////////////////////////////////////////////////
class LEDCube
{
public:
  static const int WIDTH = 8;
  static const int DEPTH = 8;
  static const int HEIGHT = 8;
  byte pwmPhase = 0;

  LEDCube() : currentLayer(0)
  {
    clearAll();
  }

  // Set the color of an individual voxel.
  void setVoxel(byte x, byte y, byte z, byte r, byte g, byte b)
  {
    if (x < WIDTH && y < DEPTH && z < HEIGHT)
    {
      voxels[x][y][z][0] = r;
      voxels[x][y][z][1] = g;
      voxels[x][y][z][2] = b;
    }
  }

  // Clear (turn off) an individual voxel.
  void clearVoxel(byte x, byte y, byte z)
  {
    setVoxel(x, y, z, 0, 0, 0);
  }

  // Clear the entire cube.
  void clearAll()
  {
    for (int x = 0; x < WIDTH; x++)
    {
      for (int y = 0; y < DEPTH; y++)
      {
        for (int z = 0; z < HEIGHT; z++)
        {
          voxels[x][y][z][0] = 0;
          voxels[x][y][z][1] = 0;
          voxels[x][y][z][2] = 0;
        }
      }
    }
  }

  // Render one layer of the cube.
  void render()
  {
    if (pwmPhase == 0) {
      currentLayer = (currentLayer + 1) % 8;
      clearCurrentLayer();
      switchToLayer(currentLayer);
    }


    uint16_t layerData[12] = {0};

    // Pack the voxel data for the current layer into 12 16-bit values.
    // The ordering follows: chips 0-3 for red, chips 4-7 for blue, chips 8-11 for green.
    for (int x = 0; x < WIDTH; x++)
    {
      for (int y = 0; y < DEPTH; y++)
      {
        int ledIndex = y + (WIDTH - 1 - x) * DEPTH;
        int chip = ledIndex / 16;
        int bit = ledIndex % 16;

        byte r = voxels[x][y][currentLayer][0];
        byte g = voxels[x][y][currentLayer][1];
        byte b = voxels[x][y][currentLayer][2];

        if (r > pwmPhase)
          layerData[chip] |= (1 << bit);
        if (b > pwmPhase)
          layerData[chip + 4] |= (1 << bit);
        if (g > pwmPhase)
          layerData[chip + 8] |= (1 << bit);
      }
    }

    paintCurrentLayer(layerData);
    pwmPhase = (pwmPhase + 1) % 8;
  }

  void renderPrism()
  {
    if (pwmPhase == 0) {
      currentLayer = (currentLayer + 1) % 8;
      clearCurrentLayer();
      switchToLayer(currentLayer);
    }


    uint16_t layerData[12] = {0};

    // Pack the voxel data for the current layer into 12 16-bit values.
    // The ordering follows: chips 0-3 for red, chips 4-7 for blue, chips 8-11 for green.
    for (int x = 0; x < WIDTH; x++)
    {
      for (int y = 0; y < DEPTH; y++)
      {
        int ledIndex = y + (WIDTH - 1 - x) * DEPTH;
        int chip = ledIndex / 16;
        int bit = ledIndex % 16;

        byte r = x;
        byte g = y;
        byte b = currentLayer;

        if (r > pwmPhase)
          layerData[chip] |= (1 << bit);
        if (b > pwmPhase)
          layerData[chip + 4] |= (1 << bit);
        if (g > pwmPhase)
          layerData[chip + 8] |= (1 << bit);
      }
    }

    paintCurrentLayer(layerData);
    pwmPhase = (pwmPhase + 1) % 8;
  }
private:
  // Cube voxel data: 4 dimensions: [x][y][z][color] (0: red, 1: green, 2: blue)
  uint8_t voxels[WIDTH][DEPTH][HEIGHT][3];
  int currentLayer = HEIGHT;

  // Clear the current layer by shifting out zeros.
  void clearCurrentLayer()
  {
    digitalWrite(OE, HIGH); // Disable output while shifting data
    digitalWrite(LE, LOW);  // Keep latch low until data is fully sent

    // Clear all outputs by sending 12×16 bits of 0 via SPI (fast blanking)
    for (int chip = 11; chip >= 0; --chip) {
      mySPI.transfer16(0x0000);
  }

    digitalWrite(LE, HIGH); // Latch data to outputs
    delayMicroseconds(1);  
    digitalWrite(LE, LOW);  // Reset latch
    digitalWrite(OE, LOW);  // Enable output
  }

  // Shift out the prepared 12×16-bit data for the current layer.
  void paintCurrentLayer(uint16_t data[12])
  {
    // digitalWrite(OE, HIGH); // Disable output while shifting data
    digitalWrite(LE, LOW);  // Keep latch low until data is fully sent

    // Shift out 12 x 16-bit values (MSB first)
    for (int chip = 11; chip >= 0; --chip) {
      mySPI.transfer16(data[chip]);  // Send 16 bits to the shift registers
  }

    digitalWrite(LE, HIGH); // Latch data to outputs
    // delayMicroseconds(1);  
    // digitalWrite(LE, LOW);
    digitalWrite(OE, LOW); // Enable output
  }

  // Set the demultiplexer outputs to select the proper layer.
  void switchToLayer(int z)
  {
    z = z & 0x1F; // Mask to 5 bits
    digitalWrite(DEMUX_A, z & 1);
    digitalWrite(DEMUX_B, (z >> 1) & 1);
    digitalWrite(DEMUX_C, (z >> 2) & 1);
    digitalWrite(DEMUX_D, (z >> 3) & 1);
  }
};

///////////////////////////////////////////////////////////////////////////////
// Global LEDCube instance
///////////////////////////////////////////////////////////////////////////////
LEDCube cube;

///////////////////////////////////////////////////////////////////////////////
// Timer Interrupt Callback
///////////////////////////////////////////////////////////////////////////////
void timerCallback()
{
  cube.renderPrism();
}

///////////////////////////////////////////////////////////////////////////////
// Setup
///////////////////////////////////////////////////////////////////////////////
void setup()
{
  // Disable JTAG to free PA14 for GPIO.
  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_DISABLE();

  // Initialize onboard LEDs.
  pinMode(LED_Red, OUTPUT);
  pinMode(LED_Green, OUTPUT);

  // Initialize buttons.
  pinMode(Key1, INPUT_PULLUP);
  pinMode(Key2, INPUT_PULLUP);
  pinMode(Key3, INPUT_PULLUP);
  pinMode(Key4, INPUT_PULLUP);
  pinMode(Key5, INPUT_PULLUP);
  pinMode(Key6, INPUT_PULLUP);
  pinMode(Key7, INPUT_PULLUP);

  // Initialize switches.
  pinMode(SW1, INPUT_PULLUP);
  pinMode(SW2, INPUT_PULLUP);

  //
  // pinMode(SPI_Clock, OUTPUT);
  // pinMode(SPI_MOSI, OUTPUT);
  mySPI.begin();
  mySPI.beginTransaction(SPISettings(25000000, MSBFIRST, SPI_MODE0));
  pinMode(LE, OUTPUT);
  pinMode(OE, OUTPUT);

  // Initialize demultiplexer pins.
  pinMode(DEMUX_A, OUTPUT);
  pinMode(DEMUX_B, OUTPUT);
  pinMode(DEMUX_C, OUTPUT);
  pinMode(DEMUX_D, OUTPUT);
  pinMode(DEMUX_ENABLE, OUTPUT);

  // Open Serial1 at 115200 bps.
  //Serial1.begin(115200);
  cube.clearAll();


  // Set up the hardware timer.
  timer = new HardwareTimer(TIM2);
  timer->setPrescaleFactor(1);
  timer->setOverflow(timerInterval, MICROSEC_FORMAT);
  timer->attachInterrupt(timerCallback);
  timer->resume();
  // Example test pattern: update a few voxels.
  // cube.setVoxel(0, 0, 0, 0, 0, 0); // "Black"
  // cube.setVoxel(7, 0, 0, 7, 0, 0); // "Red"
  // cube.setVoxel(6, 0, 0, 6, 0, 0);

  // cube.setVoxel(0, 7, 0, 0, 7, 0); // "Green"
  // cube.setVoxel(0, 0, 7, 0, 0, 7); // "Blue"
  // cube.setVoxel(7, 0, 7, 7, 0, 7); // Magenta (Red + Blue)
  // cube.setVoxel(7, 7, 0, 7, 7, 0); // Yellow (red + green)
  // cube.setVoxel(0, 7, 7, 0, 7, 7); // Cyan (green + blue)
  // cube.setVoxel(7, 7, 7, 7, 7, 7); // White (all channels)

}

///////////////////////////////////////////////////////////////////////////////
// Main Loop
///////////////////////////////////////////////////////////////////////////////
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


  for(int x = 7; x >= 0; x--){
    for(int y = 7; y >= 0; y--){
     for(int z = 7; z >= 0; z--){
      cube.setVoxel(x, y, z, x, y, z);
      delayMicroseconds(100);
    }
   }
  }


  // NEW: Debug prints if enabled
  if (debugEnabled)
  {
    // Print button/switch status, for example
    Serial1.print("Key1: ");
    Serial1.print(key1Pressed);
    Serial1.print("  Key2: ");
    Serial1.print(key2Pressed);
    Serial1.print("  Key3: ");
    Serial1.print(key3Pressed);
    Serial1.print("  Key4: ");
    Serial1.print(key4Pressed);
    Serial1.print("  Key5: ");
    Serial1.print(key5Pressed);
    Serial1.print("  Key6: ");
    Serial1.print(key6Pressed);
    Serial1.print("  Key7: ");
    Serial1.println(key7Pressed);

    Serial1.print("SW1: ");
    Serial1.print(switch1);
    Serial1.print("  SW2: ");
    Serial1.println(switch2);

    // Adjust delay or remove if it interferes with timing
    delay(100);
  }
}