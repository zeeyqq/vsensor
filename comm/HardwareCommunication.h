#ifndef HARDWARECOMMUNICATION_H
#define HARDWARECOMMUNICATION_H

#include <thread>
#include <atomic>
#include <iostream>
#include <mutex>
#include <chrono>

class HardwareCommunication {
public:
    virtual ~HardwareCommunication() { stop(); }

    void start() {
        if (!connected) {
            connected = true;
            communicationThread = std::thread(&HardwareCommunication::run, this);
        }
    }

    void stop() {
        if (connected) {
            connected = false;
            if (communicationThread.joinable()) {
                communicationThread.join();
            }
        }
    }

    bool isConnected() const { return connected; }

protected:
    std::atomic<bool> connected{false};
    std::thread communicationThread;

    virtual void run() = 0;
};

#endif // HARDWARECOMMUNICATION_H
