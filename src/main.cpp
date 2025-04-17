#include <Arduino.h>

void setup() {
  Serial.begin(115200);
  while (!Serial);  // Wait for serial to connect (optional on some boards)

  delay(3000);  // Let USB settle
  Serial.println("Starting crash test...");
  Serial.flush();  // Make sure message is out

  delay(100);  // Let the flush complete (important!)
  *((volatile int*)nullptr) = 42;  // Intentional crash
}

void loop() {}
