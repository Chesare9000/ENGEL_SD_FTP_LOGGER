#pragma once

#ifndef LISTENER_H
#define LISTENER_H

#include <Arduino.h>
#include <memory>
#include <mutex>
#include <map>
#include <thread>
#include <atomic>
#include <tasks.h>

#include "ble.h"

namespace BLE {
    // A class that takes the map of variables and updates them when a new value is received
    class Listener {
    public:
        Listener(
            BLEngelService &bleService);

        ~Listener();

        // a function that updates the variables
        void updateVariables();

        // kills the tasks
        void stop();

    private:
        std::shared_ptr<BLEngelService*> m_bleService;
        std::mutex var_mutex;

        std::map<std::string, int> m_old_int_map;
        std::map<std::string, float> m_old_float_map;
        std::map<std::string, bool> m_old_bool_map;
        std::map<std::string, std::string> m_old_string_map;

        // a function that runs on a seperate thread and updates the variables
        void begin();

        TaskHandle_t task_handle = NULL;
    };
}
#endif