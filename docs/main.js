// app.js

// Attach a click handler to the "Connect" button.
document.getElementById("connect-button").addEventListener("click", async () => {
    await connectSerial();
  });
  
  // Function to request and open the serial port.
  async function connectSerial() {
    if ("serial" in navigator) {
      try {
        // Request the serial port from the user.
        const port = await navigator.serial.requestPort();
        
        // Open the port with the specified baud rate.
        await port.open({ baudRate: 115200 });
        console.log("Port opened!");
  
        // Create a TextDecoderStream to convert the incoming bytes into text.
        const textDecoder = new TextDecoderStream();
        // Pipe the port's readable stream into the decoder.
        const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
        const reader = textDecoder.readable.getReader();
  
        // Continuously read data from the serial port.
        while (true) {
          const { value, done } = await reader.read();
          if (done) {
            // Allow the serial port to be closed later.
            console.log("Stream closed");
            break;
          }
          if (value) {
            processSerialData(value);
          }
        }
        
        // Close the reader when finished.
        reader.releaseLock();
      } catch (error) {
        console.error("Error opening the serial port:", error);
      }
    } else {
      console.error("Web Serial API not supported in this browser.");
    }
  }
  
  // Function to process and parse incoming serial data.
  function processSerialData(data) {
    // Split the incoming data into lines (in case multiple lines come in).
    const lines = data.split("\n");
    lines.forEach(line => {
      if (line.trim() !== "") {
        // Example line:
        // "Key1: true	Key2: false	Key3: true	Key4: false	Key5: false	Key6: true	Key7: false"
        // "SW1: true	SW2: false"
        // We'll split by whitespace (tabs or spaces) to get each key-value pair.
        const fields = line.split(/\s+/);
        fields.forEach(field => {
          // Each field should be in the form "KeyX:" or "SWY:" followed by the value.
          const parts = field.split(":");
          if (parts.length === 2) {
            const key = parts[0].trim();
            const value = parts[1].trim();
            // If there's an element with this key as its ID, update its content.
            const element = document.getElementById(key);
            if (element) {
              element.textContent = value;
            } else {
              console.log(`No element found for key: ${key}`);
            }
          }
        });
      }
    });
  }
  