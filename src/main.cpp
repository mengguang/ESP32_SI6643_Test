#include <Arduino.h>
#include <SPI.h>
#include "SSD1306Spi.h"
#include "drv_SI446x.h"

const int led = LED_BUILTIN;

#define OLED_RST_PIN 16
#define OLED_DC_PIN 4
#define OLED_CS_PIN 17

SSD1306Spi display(OLED_RST_PIN, OLED_DC_PIN, OLED_CS_PIN);

#define SPI_SCK_PIN 23
#define SPI_MOSI_PIN 27
#define SPI_MISO_PIN 26

void drawFontFaceDemo()
{
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 0, "Hello world");
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 10, "Hello world");
  display.setFont(ArialMT_Plain_24);
  display.drawString(0, 26, "Hello world");
}

void displayReceivedData(const char *message, int rssi, int lqi)
{
  char buffer[64];
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_16);
  display.drawStringMaxWidth(0, 0, 128, message);
  display.setFont(ArialMT_Plain_16);
  display.drawStringf(0, 30, buffer, "RSSI: %d", rssi);
  display.drawStringf(0, 45, buffer, "LQI:  %d", lqi);
  display.display();
}

void dumpHex(uint8_t *data, uint32_t length)
{
  char buf[5];
  for (uint32_t i = 0; i < length; i++)
  {
    snprintf(buf, 5, "0x%02X", data[i]);
    Serial.print(buf);
    Serial.print(" ");
  }
  Serial.println();
}

bool packet_received = false;

void IRAM_ATTR isr_si4463()
{
  packet_received = true;
}

void setup()
{
  pinMode(led, OUTPUT);

  // init SPI for Pico
  // SPI = arduino::MbedSPI(CC1101_MISO_PIN, CC1101_MOSI_PIN, CC1101_SCK_PIN);
  // SPI.begin();

  // init SPI for ESP32
  SPI.begin(SPI_SCK_PIN, SPI_MISO_PIN, SPI_MOSI_PIN);
  SPI.setFrequency(1000000);

  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.clear();
  drawFontFaceDemo();
  display.display();

  Serial.begin(115200);
  Serial.println("Hello World!");

  delay(1000);
  SI446x_Init();
  // SI446x_Set_Power(0);
  uint8_t partInfo[9] = {0};
  SI446x_Get_Part_Informatoin(partInfo);
  dumpHex(partInfo, sizeof(partInfo));
  attachInterrupt(SI_IRQ_PIN, isr_si4463, FALLING);
  SI446x_Start_Rx(6, 0, PACKET_LENGTH, 0, 0, 0);
  randomSeed(millis());
}

char message[64];
uint32_t n = 0;
uint32_t lastSend = 0;
uint32_t sendDelay = 1233;

uint8_t g_SI4463Status[9] = {0};
uint8_t g_SI4463RxBuffer[64] = {0};

void sendMessage()
{
  snprintf(message, sizeof(message), "Hello %lu", n++);
  Serial.print("Sending message: ");
  Serial.println(message);
  uint32_t s = millis();
  SI446x_Send_Packet((uint8_t *)message, strlen(message) + 1, 0, 0);
  while (true)
  {
    SI446x_Interrupt_Status(g_SI4463Status);
    if (g_SI4463Status[3] & (0x01 << 5))
    {
      break;
    }
  }
  Serial.printf("send use %lu ms.\n", millis() - s);
  SI446x_Start_Rx(6, 0, PACKET_LENGTH, 0, 0, 0);
}

void loop()
{
  if (packet_received)
  {
    packet_received = false;
    SI446x_Interrupt_Status(g_SI4463Status);
    // Serial.print("Interrupt_Status: ");
    // dumpHex(g_SI4463Status, 9);
    // SI446x_Get_Modem_Status(g_SI4463Status);
    // Serial.print("Modem_Status: ");
    // dumpHex(g_SI4463Status, 9);
    // Serial.println(g_SI4463Status[4]);
    // delay(100);
    if (g_SI4463Status[3] & (0x01 << 4))
    {
      digitalWrite(led, HIGH);
      memset(g_SI4463RxBuffer, 0, sizeof(g_SI4463RxBuffer));
      uint8_t n = SI446x_Read_Packet(g_SI4463RxBuffer);

      (void)n;
      Serial.print("Received message: ");
      Serial.println((char *)g_SI4463RxBuffer);

      SI446x_Get_Modem_Status(g_SI4463Status);
      // dumpHex(g_SI4463Status, 9);
      int8_t rssi = ((int8_t)g_SI4463Status[3]) / 2 - 130;
      Serial.print("rssi: ");
      Serial.println(rssi);

      displayReceivedData((char *)g_SI4463RxBuffer, rssi, 0);

      SI446x_Start_Rx(6, 0, PACKET_LENGTH, 0, 0, 0);
      // uint8_t status = SI446x_Get_Device_Status();
      // Serial.printf("status: %d\n", status);
      delay(100);
      digitalWrite(led, LOW);

      // sendMessage();
    }
  }
}