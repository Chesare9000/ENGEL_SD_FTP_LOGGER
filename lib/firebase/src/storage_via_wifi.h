#pragma once


void uploadLogsFromSD();

void test_uploadLogsFromSD();

void run_storage_via_wifi();

bool firebase_file_init(char* ssid , char* password);

//Terminating firebase upon request
void firebase_file_deinit();

void processData2(AsyncResult &aResult);