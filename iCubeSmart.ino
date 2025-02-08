/*
 * Initial Firmware Test
 *
 *    Key's 1-7 and SW1/2 now working!
 *
 */

// Onboard LEDs:
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

// Shift Register (MBI5024):
#define SPI_Clock     PA5     // SPI Clock - Clock input terminal for data shift
#define SPI_MOSI      PA7     // SPI MOSI - Serial-data input to the shift register
#define LE            PC4     // LE - Data strobe input terminal (latch pin)
#define OE            PC5     // OE - Output enable terminal

// Demultiplexer (74HC154):
#define DEMUX_A       PB0
#define DEMUX_B       PB1
#define DEMUX_C       PB10
#define DEMUX_D       PB11
#define DEMUX_ENABLE  PA8        // Enable Pin

// Instance of Serial used for UART:
HardwareSerial Serial1(RX, TX);

// Retains the state of different buttons being pressed:
int key1Pressed = 0;
int key2Pressed = 0;
int key3Pressed = 0;
int key4Pressed = 0;
int key5Pressed = 0;
int key6Pressed = 0;
int key7Pressed = 0;

int switch1 = 0;
int switch2 = 0;

const int colorDepth = 2;

// Height of cube (Z-Axis):
const int height = 8;

// Number of rows front-to-back (Y-Axis):
const int yAxisRows = 8;

// Number of primary colors: Red, Green, Blue:
const int primaryColors = 3;

// The Y-Axis is responsible for lighting up different primary colors (in this case, 24 states):
const int yAxisStates = primaryColors * yAxisRows;

/* Contains the max value each Serial read should store (consisting of 2x color layers +
 *  1 byte prepended corresponding to color depth to update):
 */
const byte numBytes = yAxisStates * height + 1;
byte receivedBytes[numBytes]; // Array that holds incoming data to paint the cube.
boolean newData = false; // Controls whether incoming data has finished being read.
byte header[] = {0x00, 0xFF, 0x00, 0x00}; // The header used to delineate incoming data.

int targetColorDepth = 0; // Contains the destination color layer that is to be rendered.

// The Cube, represented by a multidimensional array of bytes (each byte lighting up an X-Axis row of LEDs).
/*
 * If a byte is set to 0, the entire row is off.
 * If a byte is 1, the leftmost LED is on.
 * If a byte is 255, the entire row is on.
 */
byte cube[colorDepth][height][yAxisStates] =
{{
// G  G  B  B  R  R  G  G  B  B  R  R  G  G  B  B  R  R  G  G  B  B  R  R
{0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255}, // Red
{0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255}, // Red+Yellow
{255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255}, // Yellow
{255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0}, // Green
{255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0}, // Cyan
{0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0}, // Blue
{0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255}, // Magenta
{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}, // White
},{
{0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255}, // Red
{255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255}, // Yellow+Red
{255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255}, // Yellow
{255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0}, // Green
{255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0}, // Cyan
{0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 255, 255, 0, 0}, // Blue
{0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255}, // Magenta
{255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255}, // White
}};

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
}

void clearCurrentLayer() {
  digitalWrite(LE, LOW);
  for (int i = 0; i < yAxisStates; i++) 
    shiftOut(SPI_MOSI, SPI_Clock, LSBFIRST, 0);
  digitalWrite(LE, HIGH);
}

void switchToLayerZ(int z) {
  z = z & 0x1F;
  digitalWrite(DEMUX_A, z & 1);
  digitalWrite(DEMUX_B, (z >> 1) & 1);
  digitalWrite(DEMUX_C, (z >> 2) & 1);
  digitalWrite(DEMUX_D, (z >> 3) & 1);
}

void readIncomingBytes() {
  static boolean recvInProgress = false;  // Controls whether the cube is currently receiving data.
  static byte ndx = 0;                    // Index of current incoming byte.
  int triggerStart = 0;                   // Keeps track of whether the entire header was read from Serial.
  byte rb;                                // Current byte being read from Serial.

  while (Serial1.available() > 0 && newData == false) {
      rb = Serial1.read();
      if (recvInProgress == true) {
          if (ndx < numBytes - 1) {
              receivedBytes[ndx] = rb;
              ndx++;
              if (ndx >= numBytes) ndx = numBytes - 1;
          } else if (ndx == numBytes - 1) {
              receivedBytes[ndx] = rb;
              recvInProgress = false;
              ndx = 0;
              newData = true;
          }
      } else if (rb == header[triggerStart] && triggerStart == sizeof(header)) {
        recvInProgress = true;
        triggerStart = 0;
      } else if (rb == header[triggerStart] && triggerStart < sizeof(header)) {
        triggerStart++;
        if (triggerStart == sizeof(header)) {
          recvInProgress = true;
          triggerStart = 0;
        }
      } else {
        triggerStart = 0;
      }
  }
}

// Paints the entire cube whatever value is given (0 - 255):
void paintCube(int value) {
  for (int c = 0; c < colorDepth; c++) {
    for (int z = 0; z < height; z++) {
      for (int y = 0; y < yAxisStates; y++) {
        cube[c][z][y] = value;
      }
    }
  }
}

// Function to randomize the cube values
void randomizeCube() {
    for (int color = 0; color < colorDepth; ++color) {
        for (int heightIdx = 0; heightIdx < height; ++heightIdx) {
            for (int yAxis = 0; yAxis < yAxisStates; ++yAxis) {
                cube[color][heightIdx][yAxis] = rand() % 256; // Random value between 0 and 255
            }
        }
    }
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

  renderCube();
  readAndUpdateCube();

  // Serial1.print("Key1: " + String(key1Pressed) + "\t");
  // Serial1.print("Key2: " + String(key2Pressed) + "\t");
  // Serial1.print("Key3: " + String(key3Pressed) + "\t");
  // Serial1.print("Key4: " + String(key4Pressed) + "\t");
  // Serial1.print("Key5: " + String(key5Pressed) + "\t");
  // Serial1.print("Key6: " + String(key6Pressed) + "\t");
  // Serial1.println("Key7: " + String(key7Pressed) + "\t");
  // Serial1.print("SW1: " + String(switch1) + "\t");
  // Serial1.println("SW2: " + String(switch2) + "\t");

}

void renderCube() {
  // Loop through color depth and Z-axis layers
  for (int c = 0; c < colorDepth; c++) {
    for (int z = 0; z < height; z++) {
      clearCurrentLayer();
      switchToLayerZ(z);
      digitalWrite(LE, LOW);
      
      // Render each Y-axis layer
      for (int y = 0; y < yAxisStates; y++) {
        // Render each X-axis byte-layer
        shiftOut(SPI_MOSI, SPI_Clock, LSBFIRST, cube[c][z][y]);
      }
      digitalWrite(LE, HIGH);
    }
  }
}

void readAndUpdateCube() {
  readIncomingBytes();
  if (newData) {
    targetColorDepth = receivedBytes[0];
    updateCubeWithNewData();
    newData = false;
  }
}

void updateCubeWithNewData() {
  // Update the cube with the received bytes
  for (int z = 0; z < height; z++) {
    for (int y = 0; y < yAxisStates; y++) {
      cube[targetColorDepth][z][y] = receivedBytes[z * 24 + y + 1];
    }
  }
}