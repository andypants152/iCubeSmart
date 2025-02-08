/*
 * Initial Firmware Test
 *
 *    Key's 1-7 now working!
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

void setup()
{
  // Disable JTAG to use PA14 as GPIO
  __HAL_RCC_AFIO_CLK_ENABLE();   // Enable AFIO clock
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

  // Initialize IR receiver
  pinMode(IR_RECEIVER_PIN, INPUT);
    


  // Open serial port and listen @ 115200 bps:
  Serial1.begin(115200);
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

  Serial1.print("Key1: " + String(key1Pressed) + "\t");
  Serial1.print("Key2: " + String(key2Pressed) + "\t");
  Serial1.print("Key3: " + String(key3Pressed) + "\t");
  Serial1.print("Key4: " + String(key4Pressed) + "\t");
  Serial1.print("Key5: " + String(key5Pressed) + "\t");
  Serial1.print("Key6: " + String(key6Pressed) + "\t");
  Serial1.println("Key7: " + String(key7Pressed) + "\t");

}