/*
 *  Open Source iCubeSmart 3D8RGB LED Cube Firmware
 *
 *  8×8×8 Configuration, 3-bit color, refactored for OOP.
 * 
 */

//#define TOP_SPEED_MODE


// Pin definitions (unchanged)
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

#define SPI_Clock PA5 // Hardware SPI clock (if not using SPI mode, used with digitalWrite)
#define SPI_MOSI PA7  // Hardware SPI MOSI (if not using SPI mode, used with digitalWrite)
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

#ifdef TOP_SPEED_MODE
// In TOP_SPEED mode with SPI, we aim to shift out 192 bits as quickly as possible.
// We set the timer overflow to roughly 8 µs per render for dimmest and 2000 for brightest 
//(i cant remember the math reason for the upper limit).
//1 microsecond per layer lower limit. 
const uint32_t TIMER_INTERVAL = 2000; // microseconds
#else
// For normal operation, use a lower frame rate.
const uint32_t FRAME_RATE = 500;                      // Frames per second (each layer update)
const uint32_t TIMER_INTERVAL = 1000000 / FRAME_RATE; // in microseconds
#endif

HardwareTimer *timer = nullptr;

///////////////////////////////////////////////////////////////////////////////
// LEDCube Class
///////////////////////////////////////////////////////////////////////////////
class LEDCube
{
public:
  static const int WIDTH = 8;
  static const int DEPTH = 8;
  static const int HEIGHT = 8;

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
    clearCurrentLayer();
    switchToLayer(currentLayer);

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

        if (r)
          layerData[chip] |= (1 << bit);
        if (b)
          layerData[chip + 4] |= (1 << bit);
        if (g)
          layerData[chip + 8] |= (1 << bit);
      }
    }

    paintCurrentLayer(layerData);
    currentLayer = (currentLayer + 1) % HEIGHT;
  }

private:
  // Cube voxel data: 4 dimensions: [x][y][z][color] (0: red, 1: green, 2: blue)
  uint8_t voxels[WIDTH][DEPTH][HEIGHT][3];
  int currentLayer;

  // Clear the current layer by shifting out zeros.
  void clearCurrentLayer()
  {
    digitalWrite(OE, HIGH); // Disable output while shifting data
    digitalWrite(LE, LOW);  // Keep latch low until data is fully sent

    // Shift out 12 chips × 16 bits each = 192 bits.
    for (int chip = 11; chip >= 0; chip--)
    {
      uint16_t data = 0x0000; // All LEDs off (active-low)
      for (int i = 15; i >= 0; i--)
      {
        digitalWrite(SPI_Clock, LOW);
        digitalWrite(SPI_MOSI, (data & (1 << i)) ? HIGH : LOW);
        digitalWrite(SPI_Clock, HIGH);
      }
    }

    digitalWrite(LE, HIGH); // Latch data to outputs
    digitalWrite(LE, LOW);  // Reset latch
    digitalWrite(OE, LOW);  // Enable output
  }

  // Shift out the prepared 12×16-bit data for the current layer.
  void paintCurrentLayer(uint16_t data[12])
  {
    digitalWrite(OE, HIGH); // Disable output while shifting data
    digitalWrite(LE, LOW);  // Keep latch low until data is fully sent

    // Shift out 12 x 16-bit values (MSB first)
    for (int chip = 11; chip >= 0; chip--)
    {
      for (int i = 15; i >= 0; i--)
      {
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
  cube.render();
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
  pinMode(SPI_Clock, OUTPUT);
  pinMode(SPI_MOSI, OUTPUT);
  pinMode(LE, OUTPUT);
  pinMode(OE, OUTPUT);

  // Initialize demultiplexer pins.
  pinMode(DEMUX_A, OUTPUT);
  pinMode(DEMUX_B, OUTPUT);
  pinMode(DEMUX_C, OUTPUT);
  pinMode(DEMUX_D, OUTPUT);
  pinMode(DEMUX_ENABLE, OUTPUT);

  // Open Serial1 at 115200 bps.
  Serial1.begin(115200);
  cube.clearAll();

  // Set up the hardware timer.
  timer = new HardwareTimer(TIM2);
  timer->setPrescaleFactor(1);
  timer->setOverflow(TIMER_INTERVAL, MICROSEC_FORMAT);
  timer->attachInterrupt(timerCallback);
  timer->resume();
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

  // Example test pattern: update a few voxels.
  cube.setVoxel(0, 0, 0, 0, 0, 0); // "Black"
  cube.setVoxel(7, 0, 0, 1, 0, 0); // "Red"
  cube.setVoxel(0, 7, 0, 0, 1, 0); // "Green"
  cube.setVoxel(0, 0, 7, 0, 0, 1); // "Blue"
  cube.setVoxel(7, 0, 7, 1, 0, 1); // Magenta (Red + Blue)
  cube.setVoxel(7, 7, 0, 1, 1, 0); // Yellow (red + green)
  cube.setVoxel(0, 7, 7, 0, 1, 1); // Cyan (green + blue)
  cube.setVoxel(7, 7, 7, 1, 1, 1); // White (all channels)

  // Output button/switch status or other debug info here.
  Serial1.print("Key1: " + String(key1Pressed) + "\t");
  Serial1.print("Key2: " + String(key2Pressed) + "\t");
  Serial1.print("Key3: " + String(key3Pressed) + "\t");
  Serial1.print("Key4: " + String(key4Pressed) + "\t");
  Serial1.print("Key5: " + String(key5Pressed) + "\t");
  Serial1.print("Key6: " + String(key6Pressed) + "\t");
  Serial1.println("Key7: " + String(key7Pressed) + "\t");
  Serial1.print("SW1: " + String(switch1) + "\t");
  Serial1.println("SW2: " + String(switch2) + "\t");
  delay(100);
}