#include <Arduino.h>              // define I/O functions
#include <stdint.h>
#include <stdio.h>                // define I/O functions
#include <stdlib.h>
#include "string.h"
#include "SPIFFS.h"
#include "FS.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "usb/usb_host.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "usb/usb_host.h"
#include "esp_libusb.h"

#define DAEMON_TASK_PRIORITY 2
#define CLASS_TASK_PRIORITY 3

static const char* TAG_MAIN = "MAIN";
static const char* TAG_DAEMON = "DAEMON";

extern void class_driver_task(void* arg);



static void host_lib_daemon_task(void* arg)
{
  SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg; // Получить состояние семафора задач

  ESP_LOGI(TAG_DAEMON, "Installing USB Host Library");
  usb_host_config_t host_config = {
    .skip_phy_setup = false,
    .intr_flags = ESP_INTR_FLAG_LEVEL1,
  };
  ESP_ERROR_CHECK(usb_host_install(&host_config));

  // Сигнал задаче драйвера класса о том, что хост-библиотека установлена
  xSemaphoreGive(signaling_sem);    // Инициализация хоста выполнена. Разблокируем семафор
  vTaskDelay(10); // Short delay to let client task spin up
  
  bool has_clients = true;
  bool has_devices = true;
  while (has_clients || has_devices)  // Периодически проверяем наличие клиента или устройства
  {
    /*
       @brief Обработка событий USB-хост-библиотеки
       - Эта функция управляет всей обработкой USB-хост-библиотеки и ее следует вызывать неоднократно в цикле.
       — Проверьте event_flags_ret, чтобы узнать, установлены ли флаги, указывающие на определенные события USB-хост-библиотеки.
       - Эта функция никогда не должна вызываться несколькими потоками одновременно.
       @note Эта функция может блокировать
       @param[in] timeout_ticks Тайм-аут в тиках для ожидания возникновения события
       @param[out] event_flags_ret Флаги событий, указывающие, какое событие USB-хост-библиотеки произошло.
       @return esp_err_t
    */

        uint32_t event_flags;
        ESP_ERROR_CHECK(usb_host_lib_handle_events(portMAX_DELAY, &event_flags));
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS)
        {
          has_clients = false;
          ESP_LOGI(TAG_DAEMON, "has_clients = false");
          vTaskDelay(100); // Add a short delay to let the tasks run
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE)
        {
           has_devices = false;
           ESP_LOGI(TAG_DAEMON, "!!! has_devices = false");
           vTaskDelay(100); // Add a short delay to let the tasks run
        }
  }

  // Клиентов нет, завершаем контроль USB хоста
  ESP_LOGI(TAG_DAEMON, "*** No more clients and devices");

  // Uninstall the USB Host Library
  ESP_ERROR_CHECK(usb_host_uninstall());
  // Wait to be deleted
  xSemaphoreGive(signaling_sem);   // Разблокируем семафор
  vTaskSuspend(NULL);              // Приостановить выполнение задачи.
}

//=============================================================================================



// The setup() function runs once each time the micro-controller starts
void setup()
{
 
   esp_deep_sleep_disable_rom_logging();// Отключить ведение журнала из кода ПЗУ после глубокого сна.
  // поднимаем первый UART
  Serial.begin(115200);
  while (!Serial && millis() < 1000);

  esp_log_level_set("*", ESP_LOG_VERBOSE);  // Выводим отладочные сообщения

  String ver_soft = __FILE__;
  int val_srt = ver_soft.lastIndexOf('\\');
  ver_soft.remove(0, val_srt + 1);
  val_srt = ver_soft.lastIndexOf('.');
  ver_soft.remove(val_srt);
  Serial.println("\n*** Version "+ver_soft+"\n");

   //SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

   // TaskHandle_t daemon_task_hdl;
   // TaskHandle_t class_driver_task_hdl;
   // // Create daemon task
   // xTaskCreatePinnedToCore(host_lib_daemon_task,
   //     "daemon",
   //     4096,
   //     (void*)signaling_sem,
   //     DAEMON_TASK_PRIORITY,
   //     &daemon_task_hdl,
   //     0);
   // // Create the class driver task
   // xTaskCreatePinnedToCore(class_driver_task,
   //     "class",
   //     4096,
   //     (void*)signaling_sem,
   //     CLASS_TASK_PRIORITY,
   //     &class_driver_task_hdl,
   //     0);

   // vTaskDelay(10); // Add a short delay to let the tasks run

   // // Wait for the tasks to complete
   // for (int i = 0; i < 2; i++)
   // {
   //     xSemaphoreTake(signaling_sem, portMAX_DELAY);
   // }

   // // Delete the tasks
   // vTaskDelete(class_driver_task_hdl);
   // vTaskDelete(daemon_task_hdl);
  
  ESP_LOGI(TAG_MAIN, "*** End setup *** \n");
  vTaskDelay(1000); // Add a short delay to let the tasks run

}

// Add the main program code into the continuous loop() function
void loop()
{
    SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();

    TaskHandle_t daemon_task_hdl;
    TaskHandle_t class_driver_task_hdl;
    // Create daemon task
    xTaskCreatePinnedToCore(host_lib_daemon_task,
        "daemon",
        4096,
        (void*)signaling_sem,
        DAEMON_TASK_PRIORITY,
        &daemon_task_hdl,
        0);
    // Create the class driver task
    xTaskCreatePinnedToCore(class_driver_task,
        "class",
        4096,
        (void*)signaling_sem,
        CLASS_TASK_PRIORITY,
        &class_driver_task_hdl,
        0);

    vTaskDelay(10); // Add a short delay to let the tasks run

    // Wait for the tasks to complete
    for (int i = 0; i < 2; i++)
    {
        xSemaphoreTake(signaling_sem, portMAX_DELAY);
    }

    // Delete the tasks
    vTaskDelete(class_driver_task_hdl);
    vTaskDelete(daemon_task_hdl);


}