#include "CANCommunication.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    CANCommunication canComm("vcan0");

    std::cout << "[정보] CAN 통신 시작" << std::endl;
    canComm.start();

    std::this_thread::sleep_for(std::chrono::seconds(10));

    std::cout << "[정보] CAN 통신 중지" << std::endl;
    canComm.stop();

    return 0;
}
