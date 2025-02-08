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
int key1Pressed = 0;
int key2Pressed = 0;
int key3Pressed = 0;
int key4Pressed = 0;
int key5Pressed = 0;
int key6Pressed = 0;
int key7Pressed = 0;

int switch1 = 0;
int switch2 = 0;

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

void clearCurrentLayer()
{
  digitalWrite(OE, HIGH);  // Disable output while shifting data
  digitalWrite(LE, LOW);   // Keep latch low until data is fully sent

  // Send 192 bits (12 MBI5024s × 16 bits each)
  for (int chip = 0; chip < 12; chip++) {
    uint16_t data = 0x0000; // All LEDs off (MBI5024 is active-low)
    
    for (int i = 15; i >= 0; i--) { // Shift 16 bits per chip
      digitalWrite(SPI_Clock, LOW);
      digitalWrite(SPI_MOSI, (data & (1 << i)) ? HIGH : LOW);
      digitalWrite(SPI_Clock, HIGH);
    }
  }

  digitalWrite(LE, HIGH);  // Latch data to outputs
  digitalWrite(LE, LOW);   // Reset latch
  digitalWrite(OE, LOW);   // Enable output
}

void paintCurrentLayer(uint16_t data[12]) {
  digitalWrite(OE, HIGH);  // Disable output while shifting data
  digitalWrite(LE, LOW);   // Keep latch low until data is fully sent

  // Shift out 12 x 16-bit values (MSB first)
  for (int chip = 11; chip >= 0; chip--) { // Start from last chip to first
    for (int i = 15; i >= 0; i--) { // MSB first
      digitalWrite(SPI_Clock, LOW);
      digitalWrite(SPI_MOSI, (data[chip] & (1 << i)) ? HIGH : LOW);
      digitalWrite(SPI_Clock, HIGH);
    }
  }

  digitalWrite(LE, HIGH);  // Latch data to outputs
  delayMicroseconds(1);    // Short delay for stability
  digitalWrite(LE, LOW);
  digitalWrite(OE, LOW);   // Enable output
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
  static uint8_t lastKeyState = 0;
  uint8_t currentKeyState = 0;
  key1Pressed = digitalRead(Key1) == LOW;
  key2Pressed = digitalRead(Key2) == LOW;
  key3Pressed = digitalRead(Key3) == LOW;
  key4Pressed = digitalRead(Key4) == LOW;
  key5Pressed = digitalRead(Key5) == LOW;
  key6Pressed = digitalRead(Key6) == LOW;
  key7Pressed = digitalRead(Key7) == LOW;

  // Combine key states into a single byte for easy comparison
  currentKeyState = key1Pressed | key2Pressed << 1 | key3Pressed << 2 |
                    key4Pressed << 3 | key5Pressed << 4 | key6Pressed << 5 |
                    key7Pressed << 6;

  switch1 = digitalRead(SW1) == HIGH;
  switch2 = digitalRead(SW2) == HIGH;

  uint8_t color = 255;
  // Detect key press (any key pressed that wasn’t pressed before)
  if (currentKeyState && currentKeyState != lastKeyState) {
    color = (color + 1) % 256; // Cycle through colors 0 to 255
  }

  lastKeyState = currentKeyState;
uint16_t testPattern[12] = {0x0002, 0xFFFF, 0xFFFF, 0xFFFF, 0x0000, 0x0000,
                            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000};

paintCurrentLayer(testPattern);

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