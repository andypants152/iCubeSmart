// main.js
document.addEventListener("DOMContentLoaded", () => {
// Attach a click handler to the "Connect" button after the site has loaded
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
  
// Global buffer to accumulate incoming data
let serialBuffer = "";

// Function to process serial data from the Web Serial API.
function processSerialData(data) {
  // Append the new data to our buffer.
  serialBuffer += data;
  
  // Split the buffer by newline characters.
  const lines = serialBuffer.split("\n");
  
  // The last element may be an incomplete line, so save it back to the buffer.
  serialBuffer = lines.pop();
  
  // Process each complete line.
  lines.forEach(line => {
    line = line.trim();
    if (line !== "") {
      // Split the line into fields based on whitespace.
      const fields = line.split("\t");
      fields.forEach(field => {
        // Each field should be in the form "KeyX:" or "SWY:" followed by the value.
        const parts = field.split(":");
        if (parts.length === 2) {
          const key = parts[0].trim();
          let value = parts[1].trim();
          
          // Convert "0" and "1" to boolean text if desired.
          if (value === "0" || value === "1") {
            value = (value === "1") ? "true" : "false";
          }
          
          // Update the corresponding DOM element if it exists.
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
});  