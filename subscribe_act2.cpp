#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include <thread>
#include <chrono>
#include <iostream>
#include "RTC.h"
#include "MQTTClient.h"

#define ADDRESS     "tcp://192.168.7.1:1883"
#define CLIENTID    "rpi2"
#define AUTHMETHOD  "eoghan"
#define AUTHTOKEN   "ligia1993"
#define TOPIC       "ee513/Sensor"
#define PAYLOAD     "Hello World!"
#define QOS         1
#define TIMEOUT     10000L

using namespace std::chrono;
using namespace std::this_thread;

// This actuator application will flash the LED hooked up to GPIO pin 67

volatile MQTTClient_deliveryToken deliveredtoken;

void resetRTC(){
    RTC rtc;
    rtc.setDate(0,0,0,0,0,0,0); 
}

float determineRoll(std::string json){
    std::string pitchDelim {"\"Roll\":"};
    std::string endDelim {"}"};
    auto first_pos = json.find(pitchDelim);
    auto end_pos = first_pos + pitchDelim.length();
    auto last_pos = json.find_first_of(endDelim, end_pos);
    return std::stof(json.substr(end_pos, last_pos - end_pos));

}

void delivered(void *context, MQTTClient_deliveryToken dt) {
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    int i;
    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    payloadptr = (char*) message->payload;
    std::string json {payloadptr, (size_t)message->payloadlen};
    std::cout << json << std::endl;
    if(std::abs(determineRoll(json)) > 10){
	std::cout << "Take it handy" << std::endl;
        resetRTC();
    }

    RTC rtc;
    rtc.displayTime(); 

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause) {
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[]) {
    MQTTClient client;
    MQTTClient_connectOptions opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    opts.keepAliveInterval = 20;
    opts.cleansession = 1;
    opts.username = AUTHMETHOD;
    opts.password = AUTHTOKEN;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client, &opts)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    do {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}

