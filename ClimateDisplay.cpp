#include "OledFont8x8.h"
#include "OledFont8x16.h"
#include "OledI2C.h"
#include "Config.h"

#include <signal.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <mosquitto.h>
#include <unistd.h>
#include <cstring>
#include <algorithm>
#include <bit>
#include <bitset>

#define MQTT_HUMIDITY "homie/pi0-3/climate/humidity"
#define MQTT_TEMPERATURE "homie/pi0-3/climate/temperature"

static std::bitset<360>nans;
static int nanIndex = 0;

static int run = 1;
void handle_signal(int s) {
    run = 0;
}

void connect_callback(struct mosquitto *mosq, void *obj, int result) {
    printf("connect callback, rc = %d\n", result);
}

void clearRectangle(SSD1306::OledI2C* oled, int y) {
    SSD1306::OledBitmap<128, 16> rectangle /* { 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0x80, 0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,    1,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }*/ ;

    oled->setFrom(rectangle, SSD1306::OledPoint(0, y));
}

void drawMessage(SSD1306::OledI2C* oled, const char* messageTemplate, const char* value, const int line) {
    clearRectangle(oled, line);
    char buffer[16];
    sprintf(buffer, messageTemplate, value);
    auto length = strlen(buffer);
    int offset = (128 - (8 * length)) / 2; 
    constexpr SSD1306::PixelStyle STYLE(SSD1306::PixelStyle::Set);
    SSD1306::OledPoint location(offset, line);
    location = drawString8x16(location, buffer, STYLE, *oled); 
}

void drawTime(SSD1306::OledI2C* oled, int line) {
    time_t now;
    time(&now);
    struct tm* tm = localtime(&now);
    char dateTime[16];
    strftime(dateTime, sizeof(dateTime), "%d-%b %H:%M:%S", tm);
    drawMessage(oled, "%s", dateTime, line);
}

void drawNaNs(SSD1306::OledI2C* oled, int line) {
    int nanCount = 0;
    for (int i=0; i< nans.size(); i++) {
        nanCount += nans.test(i);
    }
    char buffer[4];
    sprintf(buffer, "%d", nanCount);
    drawMessage(oled, "NaNs/h: %s", buffer, line);
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    bool match = 0;
    printf("received message(%s) : %s\n",message->topic, message->payload);
    const auto oled = static_cast<SSD1306::OledI2C*>(obj);
    const char* payload = static_cast<const char*>(message->payload);
    char buffer[16];
    if (strcmp(message->topic,MQTT_HUMIDITY)==0) {
        clearRectangle(oled, 48);
        drawTime(oled, 0);
        drawMessage(oled, "Hum. %s %%", payload, 16);
        nans.set(nanIndex, strcmp(payload,"nan")==0);
        nanIndex = (nanIndex + 1) % nans.size();
        drawNaNs(oled, 48);
    } else if (strcmp(message->topic, MQTT_TEMPERATURE)==0) {
        drawMessage(oled, "Temp. %s " "\xF8" "C", payload, 32);
   }
   oled->displayUpdate();
}


int main(void)
{
    uint8_t reconnect = true;
    struct mosquitto *mosq;
    int rc = 0;

    signal(SIGINT, handle_signal);
    signal(SIGINT, handle_signal);

    nans.reset();
    
    SSD1306::OledI2C oled{"/dev/i2c-1", 0x3C};

    char hostName[_SC_HOST_NAME_MAX]; 
    gethostname(hostName, _SC_HOST_NAME_MAX);
    std::transform(hostName, hostName + strlen(hostName), hostName, ::tolower);

    Config config;
    config.begin("/home/pi/.config/dht.conf", hostName);
    mosquitto_lib_init();

    mosq = mosquitto_new(NULL, true, &oled);
    if(mosq){
        mosquitto_connect_callback_set(mosq, connect_callback);
        mosquitto_message_callback_set(mosq, message_callback);
	mosquitto_tls_set(mosq, config.getEntry("caCert").c_str(), NULL, NULL, NULL, NULL);
	mosquitto_username_pw_set(mosq, config.getEntry("user").c_str(), config.getEntry("password").c_str());
	int port = 1883;
        config.setIfExists("port", &port);
        int keepalive = 60;
        config.setIfExists("keepAliveSeconds", &keepalive);

        mosquitto_connect(mosq, config.getEntry("broker", "localhost").c_str(), port, keepalive);
        mosquitto_subscribe(mosq, NULL, MQTT_HUMIDITY, 0);
        mosquitto_subscribe(mosq, NULL, MQTT_TEMPERATURE, 0);
        drawTime(&oled, 0);
        drawMessage(&oled, "%s", "Starting..", 48);
        bool isDirty = true;
        oled.displayUpdate();
        
        while(run){
            rc = mosquitto_loop(mosq, -1, 1);
            if(run && rc){
                drawMessage(&oled, "Err: %s", mosquitto_strerror(rc), 48);
                oled.displayUpdate();
                printf("connection error: %s\n", mosquitto_strerror(rc));
                sleep(5);
                mosquitto_reconnect(mosq);
            }
        }
        mosquitto_destroy(mosq);
    }
    mosquitto_lib_cleanup();
    oled.clear();
    oled.displayUpdate();
    return rc;
}
