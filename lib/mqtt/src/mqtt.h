#pragma once

typedef enum
{
    sensors,
    status,
    emergency,
    alarm_refresh,
    actions_refresh
}
out_topic_list;

typedef enum
{
    leds,
    park_alarm,
    all
}
in_topic_list;

typedef enum
{
    silent,
    print_topic,
    print_all
}
topic_print_info;



/*

void mqtt_init();
void mqtt_connect();

bool mqtt_still_connected();

void mqtt_client_loop();

void mqtt_publish(out_topic_list topic_to_publish,topic_print_info selection);

void mqtt_subscribe(in_topic_list topic_to_subscribe);

void subscriber_callback(char* topic , byte* payload , unsigned int length );


void create_task_mqtt();
void task_mqtt_i2c_declare();
void task_mqtt_i2c_release();
void task_mqtt(void * parameters);

*/