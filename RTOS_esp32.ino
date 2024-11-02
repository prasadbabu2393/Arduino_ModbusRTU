
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// #if CONFIG_FREERTOS_UNICORE
//   Static Const BaseType-t app_cpu = 0;
// #else
//   Static Const BaseType_t app_cpu =1;
// #endif 

// Task handles
TaskHandle_t Task1Handle;
TaskHandle_t Task2Handle;

const int buttonPin = 12;  // Define your button pin

void Task1(void *parameter)
{
  while (true)
  {
    Serial.println("hello");
    vTaskDelay(1000 / portTICK_PERIOD_MS);  // Correct spelling of vTaskDelay
  }
}

void Task2(void *parameter)
{
  uint32_t c = 0;
  while (true)
  {
    uint8_t buttonState = digitalRead(buttonPin);
    Serial.println("but");
    if (buttonState == LOW)  // Assuming LOW when pressed
    {
      c++;
      Serial.print("Button Press Count: ");
      Serial.println(c);
      // Simple debounce
      vTaskDelay(200 / portTICK_PERIOD_MS);  // Wait 200ms to avoid bouncing
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Check button every 10ms
  }
}

void setup()
{
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(buttonPin, INPUT_PULLUP);  // Setup button pin

  xTaskCreatePinnedToCore(
    Task1,  // Task function
    "Task1", // Task name
    1000,  // Stack size (in words)
    NULL,  // Task input parameters
    2,    // Priority
    &Task1Handle, // Task handle
    0  // Core to run on (core 0)
  );

  xTaskCreatePinnedToCore(
    Task2, // Task function
    "Task2", // Task name
    1000,  // Stack size (in words)
    NULL,  // Task input parameters
    1, // Priority
    &Task2Handle, // Task handle
    0  // Core to run on (core 0)
  );
}

void loop() {
  // put your main code here, to run repeatedly:
}
