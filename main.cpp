#include "CANCommunication.h"
#include "RS232Communication.h"
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    // CAN 통신 시작
    CANCommunication canComm("vcan0");
    canComm.start();

    // RS232 통신 시작
    RS232Communication rs232Comm("/dev/pts/5", 1000);
    rs232Comm.start();  // RS232 시작

    // exec for 10 secs..
    std::this_thread::sleep_for(std::chrono::seconds(10));

    canComm.stop();
    rs232Comm.stop();

    std::cout << "END" << std::endl;

    return 0;
}
