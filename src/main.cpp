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

const uint8_t channel = 6;

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
  Serial.println("\n\nHello World!");

  delay(1000);
  SI446x_Init();
  // SI446x_Set_Power(0);
  uint8_t partInfo[9] = {0};
  SI446x_Get_Part_Informatoin(partInfo);
  dumpHex(partInfo, sizeof(partInfo));
  attachInterrupt(SI_IRQ_PIN, isr_si4463, FALLING);
  SI446x_Start_Rx(channel, 0, PACKET_LENGTH, 0, 0, 0);
  randomSeed(millis());
}

char message[64];
int n = 0;
uint32_t lastSend = 0;
uint32_t sendDelay = 500;
uint32_t lastRecv = 0;
uint8_t statusBuffer[32] = {0};
uint32_t now = millis();

void sendMessage()
{
  n = n + n / 5 + 1;
  n = n < 0 ? 1 : n;
  memset(message, 0, sizeof(message));
  snprintf(message, sizeof(message), "Hi %d", n);
  Serial.print("Sending message: ");
  Serial.println(message);
  uint32_t s = millis();
#if PACKET_LENGTH == 0
  SI446x_Send_Packet((uint8_t *)message, strlen(message) + 1, channel, 0);
#else
  SI446x_Send_Packet((uint8_t *)message, PACKET_LENGTH, channel, 0);
#endif
  // while (true)
  // {
  //   delay(5);
  //   SI446x_Interrupt_Status(statusBuffer);
  //   if (statusBuffer[3] & (0x01 << 5))
  //   {
  //     break;
  //   }
  // }
  delay(50);
  Serial.print("Sending message use time(ms): ");
  Serial.println(millis() - s);
}

void loop()
{

  now = millis();
  if (now > lastSend + sendDelay + random(500))
  {
    lastSend = now;
    sendMessage();
    SI446x_Start_Rx(channel, 0, PACKET_LENGTH, 0, 0, 0);
  }

  now = millis();
  if (now > lastRecv + 10000)
  {
    // Serial.println("Reset Chip!");
    // SI446x_Init();
    // SI446x_Set_Packet_Variable_Length(MAX_PACKET_LENGTH);
    // SI446x_Start_Rx(channel, 0, PACKET_LENGTH, 0, 0, 0);
    // n = 0;
    // lastRecv = now;
  }

  SI446x_Get_Chip_Status(statusBuffer);
  // Serial.print("Chip_Status: ");
  // dumpHex(statusBuffer, 4);
  if (statusBuffer[2] & (0x01 << 3))
  {
    Serial.println("CMD_ERROR!");
    SI446x_Start_Rx(channel, 0, PACKET_LENGTH, 0, 0, 0);
  }

  SI446x_Interrupt_Status(statusBuffer);
  if (statusBuffer[3] & (0x01 << 4))
  {
    digitalWrite(led, HIGH);
    lastRecv = millis();
    memset(message, 0, sizeof(message));
    uint8_t n = SI446x_Read_Packet((uint8_t *)message);
    if (n == 0)
    {
      Serial.print("Error: ");
      Serial.println((const char *)message);
    }
    else
    {
      // Serial.print("Received packet length: ");
      // Serial.println(n);
      Serial.print("Received message: ");
      Serial.println((char *)message);
    }

    SI446x_Get_Modem_Status(statusBuffer);
    // dumpHex(statusBuffer, 9);
    int8_t rssi = ((int8_t)statusBuffer[3]) / 2 - 130;
    Serial.print("rssi: ");
    Serial.println(rssi);

    displayReceivedData((char *)message, rssi, 0);

    // SI446x_Start_Rx(channel, 0, PACKET_LENGTH, 0, 0, 0);
    // sendMessage();

    // delay(50);
    SI446x_Start_Rx(channel, 0, PACKET_LENGTH, 0, 0, 0);

    delay(100);
    digitalWrite(led, LOW);
  }
}
