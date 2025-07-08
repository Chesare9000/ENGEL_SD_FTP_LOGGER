#include "listener.h"
#include "freertos/FreeRTOS.h"

// creates and starts a listener that will notifiy the BLE service when a value changed
BLE::Listener::Listener(
    BLE::BLEngelService &bleService) :
    m_bleService(std::make_shared<BLE::BLEngelService*>(&bleService))
{
    for (const auto& pair : *bleService.m_int_map) {
        m_old_int_map[pair.first] = *(pair.second);
    }

    for (const auto& pair : *bleService.m_float_map) {
        m_old_float_map[pair.first] = *(pair.second);
    }

    for (const auto& pair : *bleService.m_bool_map) {
        m_old_bool_map[pair.first] = *(pair.second);
    }

    for (const auto& pair : *bleService.m_string_map) {
        m_old_string_map[pair.first] = *(pair.second);
    }

    // this will start and infinite task loop that will listen to changes
    // once anythign changes it will notify the BLE service
    begin();
}

// make sure there thread is properly stopped
BLE::Listener::~Listener() {
    // kill task
    stop();
}

// starts a new thread that will update the variables
void BLE::Listener::begin()
{

    auto thread_logic = [](void* param) { 
        BLE::Listener* listener = static_cast<BLE::Listener*>(param);
        while(true)
        {
            listener->updateVariables();
            vTaskDelay(10 / portTICK_PERIOD_MS); // Add a delay to prevent watchdog timeout
        }
    };

    xTaskCreatePinnedToCore(
        thread_logic,
        "BLE Listener",
        4096, // Stack size, might need more, as we are sending data
        this,
        1,
        &task_handle,
        0);
}


// updates the variables
void BLE::Listener::updateVariables()
{
    std::lock_guard<std::mutex> lock(var_mutex);
        bool sendUpdate = false;

        // loop through all the keys
        for(auto && key : *(*m_bleService)->m_int_map)
        {
            // check if the value has changed
            if(m_old_int_map[key.first] != *key.second)
            {
                // update the value
                (*m_bleService)->changedKeys.push_back(key.first);
                m_old_int_map[key.first] = *key.second;
                sendUpdate = true;
            }
        }

        for(auto && key : *(*m_bleService)->m_float_map){

            if(m_old_float_map[key.first] != *key.second)
            {
                // update the value
                (*m_bleService)->changedKeys.push_back(key.first);
                m_old_float_map[key.first] = *key.second;
                sendUpdate = true;
            }
        }

        for(auto && key : *(*m_bleService)->m_bool_map){        

            if(m_old_bool_map[key.first] != *key.second)
            {
                Serial.println("Change detected for " + String(key.first.c_str()) + " : " + String(*key.second ? "true" : "false"));
                Serial.println("Old value: " + String(m_old_bool_map[key.first] ? "true" : "false"));
                // update the value
                (*m_bleService)->changedKeys.push_back(key.first);
                m_old_bool_map[key.first] = *key.second;
                sendUpdate = true;
                Serial.println("New value: " + String(m_old_bool_map[key.first] ? "true" : "false"));
            }
        }

        for (auto && key : *(*m_bleService)->m_string_map){

            if(m_old_string_map[key.first] != *key.second)
            {
                // update the value
                (*m_bleService)->changedKeys.push_back(key.first);
                m_old_string_map[key.first] = *key.second;
                sendUpdate = true;
            }
        }


        if (sendUpdate)
        {
            /*Serial.println("Sending Update for");
            for(auto && key : (*m_bleService)->changedKeys)
            {
                if((*m_bleService)->m_int_map->find(key) != (*m_bleService)->m_int_map->end())
                    Serial.println(String(key.c_str()) + " : " + String(*(*m_bleService)->m_int_map->at(key)));
                else if((*m_bleService)->m_float_map->find(key) != (*m_bleService)->m_float_map->end())
                    Serial.println(String(key.c_str()) + " : " + String((*(*m_bleService)->m_float_map->at(key))));
                else if((*m_bleService)->m_bool_map->find(key) != (*m_bleService)->m_bool_map->end())
                    Serial.println(String(key.c_str()) + " : " + String((*(*m_bleService)->m_bool_map->at(key) ? "true" : "false")));
                else if((*m_bleService)->m_string_map->find(key) != (*m_bleService)->m_string_map->end())
                    Serial.println(String(key.c_str()) + " : " + String((*(*m_bleService)->m_string_map->at(key)).c_str()));
                
            }*/

            (*m_bleService)->sendUpdate(true);
            (*m_bleService)->changedKeys.clear();
            // sleep for 50ms
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }

        
    
}

void BLE::Listener::stop()
{
    vTaskDelete(task_handle);
}