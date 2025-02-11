// main.js
import * as THREE from 'three';
let port = null;
let reader = null;

// Function to request and open the serial port.
async function connectSerial() {
  if (!("serial" in navigator)) {
    console.error("Web Serial API not supported in this browser.");
    return false;
  }

  try {
    // Request the serial port from the user.
    port = await navigator.serial.requestPort();

    // Open the port with the specified baud rate.
    await port.open({ baudRate: 115200 });
    console.log("Port opened!");

    // Create a TextDecoderStream to convert incoming bytes into text.
    const textDecoder = new TextDecoderStream();
    // Pipe the port's readable stream into the decoder.
    const readableStreamClosed = port.readable.pipeTo(textDecoder.writable);
    reader = textDecoder.readable.getReader();

    // Return true immediately after opening the port
    startReading();
    return true;
  } catch (error) {
    console.error("Error opening the serial port:", error);
    return false;
  }
}

// Function to continuously read data from the serial port
async function startReading() {
  try {
    while (port && port.readable) {
      const { value, done } = await reader.read();
      if (done) {
        console.log("Stream closed");
        break;
      }
      if (value) {
        processSerialData(value);
      }
    }
  } catch (error) {
    console.error("Error reading from serial port:", error);
  } finally {
    // Clean up on disconnect
    if (reader) {
      reader.releaseLock();
      reader = null;
    }
    if (port) {
      await port.close();
      port = null;
    }
    // Update button state when disconnected
    document.getElementById("connect-btn").textContent = "Connect";
  }
}

// Function to properly disconnect the serial port
async function disconnectSerial() {
  if (port) {
    try {
      if (reader) {
        // Cancel the reader to allow the reading loop to exit
        await reader.cancel();
      }
      // Optionally, you could also close the port here,
      // but the finally block in startReading() will handle it.
      console.log("Port closed.");
      return true;
    } catch (error) {
      console.error("Error closing the serial port:", error);
      return false;
    }
  }
  return false;
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

// Function to toggle connection state
async function toggleConnection() {
  const button = document.getElementById("connect-btn");

  if (!port) {
    // Try to connect
    const success = await connectSerial();
    if (success) {
      button.textContent = "Disconnect";
    } else {
      console.log("Failed to connect.");
    }
  } else {
    // Try to disconnect
    const success = await disconnectSerial();
    if (success) {
      button.textContent = "Connect";
    } else {
      console.log("Failed to disconnect.");
    }
  }
}

function sendData() {
  console.log("Sending data...");
  // Implement data sending logic
}

function readCubeData() {
  console.log("Reading cube data...");
  // Implement data reading logic
}

//once everything is loaded
document.addEventListener("DOMContentLoaded", () => {

  const connectButton = document.getElementById("connect-btn");
  const sendButton = document.getElementById("send-btn");
  const readCubeButton = document.getElementById("read-cube-btn");

  if (connectButton) {
    connectButton.addEventListener("click", toggleConnection);
  }
  if (sendButton) {
    sendButton.addEventListener("click", sendData);
  }
  if (readCubeButton) {
    readCubeButton.addEventListener("click", readCubeData);
  }

  const container = document.getElementById('threejs-container');
  const scene = new THREE.Scene();
  const camera = new THREE.PerspectiveCamera(75, container.clientWidth / container.clientHeight, 0.1, 1000);
  const renderer = new THREE.WebGLRenderer();
  renderer.setSize(container.clientWidth, container.clientHeight);
  container.appendChild(renderer.domElement);

  const geometry = new THREE.BoxGeometry(1, 1, 1);
  const material = new THREE.MeshBasicMaterial({ color: 0x00ff00 });
  const cube = new THREE.Mesh(geometry, material);
  scene.add(cube);

  camera.position.z = 5;
  function animate() {
    cube.rotation.x += 0.01;
    cube.rotation.y += 0.01;
    renderer.render(scene, camera);
  }
  renderer.setAnimationLoop(animate);
});  