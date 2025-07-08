#pragma once

//#include <WebSocketsServer.h>  

void wifi_demo_init(); 

//the parameters of this callback function are always the same: 
//num: id of the client who send the event, 
//type: type of message, payload: actual data sent and length: length of payload


//move to cpp to workaround a bug on dep , but here is where is suppose to be , TODO UNCOMMENT and fix 
//void webSocketEvent(byte num, WStype_t type, uint8_t * payload, size_t length);

// Simple function to send information to the web clients
void sendJson(String l_type, String l_value);


//WIFI_DEMO TASK ----------

void create_task_wifi_demo();//once created it will automatically run

void task_wifi_demo_i2c_declare();

void task_wifi_demo_i2c_release();

void wifi_demo(void * parameters);

void wifi_demo_disconnect();
void wifi_demo_off();
