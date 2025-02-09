#include "CANCommunication.h"
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <thread>
#include <fcntl.h>
#include <poll.h>

CANCommunication::CANCommunication(const std::string& interfaceName) : interfaceName(interfaceName) {
    idDataRanges = {
        {0x19FF1000, {-64.0f, 64.51f}},  // 차량 방향각
        {0x19FF1001, {-320.0f, 322.55f}}, // 차량 속도
        {0x19FF1002, {-250.0f, 250.99f}}  // 차량 자세 데이터
    };
}

CANCommunication::~CANCommunication() {
    stop();
    if (socket_fd >= 0) {
        close(socket_fd);
        std::cout << "[정보] 소켓 닫힘" << std::endl;
    }
}

void CANCommunication::initializeSocket(int& socket_fd, struct sockaddr_can& addr, struct ifreq& ifr) {
    socket_fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd < 0) {
        std::cerr << "[오류] 소켓 생성 실패" << std::endl;
        throw std::runtime_error("소켓 생성 실패");
    }

    strcpy(ifr.ifr_name, interfaceName.c_str());
    if (ioctl(socket_fd, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "[오류] 인터페이스 설정 실패: " << interfaceName << std::endl;
        close(socket_fd);
        throw std::runtime_error("인터페이스 설정 실패");
    }

    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socket_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "[오류] 소켓 바인딩 실패" << std::endl;
        close(socket_fd);
        throw std::runtime_error("소켓 바인딩 실패");
    }

    // 자신이 보낸 메시지도 수신하도록 설정.
    int recvOwn = 1;
    if (setsockopt(socket_fd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recvOwn, sizeof(recvOwn)) < 0) {
        std::cerr << "[오류] CAN_RAW_RECV_OWN_MSGS 옵션 설정 실패: " << strerror(errno) << std::endl;
    }

    // std::cout << "[디버깅] CAN 소켓 초기화 완료: socket_fd=" << socket_fd << std::endl;
    this->socket_fd = socket_fd;
}

float CANCommunication::generateRandomValue(float minValue, float maxValue) {
    std::uniform_real_distribution<float> distribution(minValue, maxValue);
    return distribution(randomEngine);
}

int CANCommunication::generateRandomCANID() {
    std::vector<int> possibleIDs = {0x19FF1000, 0x19FF1001, 0x19FF1002};
    std::uniform_int_distribution<int> distribution(0, possibleIDs.size() - 1);
    return possibleIDs[distribution(randomEngine)];
}

void CANCommunication::sendIMUData() {
    struct can_frame frame;

    while (connected && canSendEnabled) {
        std::vector<int> sensorIDs = {0x19FF1000, 0x19FF1001, 0x19FF1002};

        for (int canID : sensorIDs) {
            frame.can_id = canID;
            frame.can_dlc = 6;  // IMU 데이터는 6바이트

            float scale, offset, minValue, maxValue;

            // 센서 ID별 데이터 범위 및 변환값 설정
            if (canID == 0x19FF1000) {  // Roll/Pitch/Yaw
                scale = 0.002f; offset = -64.0f;
                minValue = -64.0f; maxValue = 64.51f;
            } else if (canID == 0x19FF1001) {  // Accel_X/Y/Z
                scale = 0.01f; offset = -320.0f;
                minValue = -320.0f; maxValue = 322.55f;
            } else if (canID == 0x19FF1002) {  // Gyro_X/Y/Z
                scale = 1.0f / 128.0f; offset = -250.0f;
                minValue = -250.0f; maxValue = 250.99f;
            }

            // 3개의 센서 값 생성
            float values[3];
            for (int i = 0; i < 3; i++) {
                values[i] = generateRandomValue(minValue, maxValue);
            }

            // 변환 후 CAN 프레임에 저장
            for (int i = 0; i < 3; i++) {
                int16_t scaledData = static_cast<int16_t>((values[i] - offset) / scale);
                frame.data[i * 2] = scaledData & 0xFF;
                frame.data[i * 2 + 1] = (scaledData >> 8) & 0xFF;
            }

            sendData(frame);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sendPeriodMs.load()));
    }
}

void CANCommunication::run() {
    struct sockaddr_can addr;
    struct ifreq ifr;

    try {
        initializeSocket(socket_fd, addr, ifr);
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return;
    }

    std::thread sender(&CANCommunication::sendIMUData, this);
    std::thread receiver(&CANCommunication::handleIncomingData, this);
    std::thread statusUpdater(&CANCommunication::updateConnectionStatus, this);

    sender.join();
    receiver.join();
    statusUpdater.join();

    close(socket_fd);
    socket_fd = -1;
}

void CANCommunication::handleIncomingData() {
    struct can_frame frame;
    struct pollfd pfd;

    pfd.fd = socket_fd;
    pfd.events = POLLIN;  // 읽기 가능 이벤트 감지

    // std::cout << "[디버깅] 수신 스레드 시작됨" << std::endl;

    while (connected) {
        if (socket_fd < 0) {
            std::cerr << "[오류] 수신 소켓이 유효하지 않음" << std::endl;
            break;
        }

        // std::cout << "[디버깅] CAN 데이터 수신 대기 중..." << std::endl;

        // poll()을 사용하여 데이터가 도착했는지 확인 (타임아웃 500ms)
        int ret = poll(&pfd, 1, 500);
        if (ret < 0) {
            std::cerr << "[오류] poll() 실패: " << strerror(errno)
                << " (errno=" << errno << ")" << std::endl;
            break;
        } else if (ret == 0) {
            // std::cout << "[디버깅] poll() 타임아웃: 데이터 없음" << std::endl;
            continue;  // 데이터가 없으면 다시 대기
        }

        // 데이터가 있을 때만 read() 실행
        ssize_t nbytes = read(socket_fd, &frame, sizeof(struct can_frame));

        if (nbytes > 0 && nbytes == sizeof(struct can_frame)) {
            // std::cout << "[디버깅] CAN 데이터 수신됨: ID=0x"
            //           << std::hex << frame.can_id << std::dec
            //           << ", 길이=" << (int)frame.can_dlc << std::endl;

            processReceivedData(frame);
        } else if (nbytes == 0) {
            std::cerr << "[경고] read() 반환 값이 0 (EOF?)" << std::endl;
        } else {
            std::cerr << "[오류] read() 실패: " << strerror(errno)
                << " (errno=" << errno << ")" << std::endl;
        }
    }

    // std::cout << "[디버깅] 수신 스레드 종료됨" << std::endl;
}

void CANCommunication::processReceivedData(const can_frame& frame) {
    std::lock_guard<std::mutex> lock(dataMutex);

    auto it = idDataRanges.find(frame.can_id);
    if (it == idDataRanges.end()) {
        std::cerr << "[경고] 알 수 없는 CAN ID: 0x" << std::hex << frame.can_id << std::endl;
        return;
    }

    float scale, offset;
    std::string dataType;

    if (frame.can_id == 0x19FF1000) {  // Roll/Pitch/Yaw
        scale = 0.002f; offset = -64.0f;
        dataType = "[자세] Roll/Pitch/Yaw";
    } else if (frame.can_id == 0x19FF1001) {  // Accel_X/Y/Z
        scale = 0.01f; offset = -320.0f;
        dataType = "[가속도] Accel_X/Y/Z";
    } else if (frame.can_id == 0x19FF1002) {  // Gyro_X/Y/Z
        scale = 1.0f / 128.0f; offset = -250.0f;
        dataType = "[각속도] Gyro_X/Y/Z";
    } else {
        return;
    }

    float values[3];
    for (int i = 0; i < 3; i++) {
        int16_t rawData = frame.data[i * 2] | (frame.data[i * 2 + 1] << 8);
        values[i] = (rawData * scale) + offset;
    }

    // 수신 시간 기록 (통신 상태 업데이트용)
    lastReceiveTime = std::chrono::steady_clock::now();

    // 데이터 유형과 함께 값 출력
    std::cout << "[CAN 수신] " << dataType << " | "
              << "값1=" << values[0] << ", "
              << "값2=" << values[1] << ", "
              << "값3=" << values[2] << std::endl;

    QString data = QString("[CAN 수신] %1 | 값1=%2, 값2=%3, 값3=%4")
        .arg(QString::fromStdString(dataType))
        .arg(values[0])
        .arg(values[1])
        .arg(values[2]);

    emit dataReceived(data);
}

void CANCommunication::sendData(const can_frame& frame) {
    if (socket_fd < 0) {
        std::cerr << "[오류] 소켓이 초기화되지 않음" << std::endl;
        return;
    }

    ssize_t nbytes = write(socket_fd, &frame, sizeof(struct can_frame));
    if (nbytes != sizeof(struct can_frame)) {
        std::cerr << "[오류] 데이터 전송 실패" << std::endl;
    } else {
        std::cout << "[CAN 송신] CAN ID: 0x" << std::hex << frame.can_id << " 데이터 길이: " << std::dec << (int)frame.can_dlc << " 데이터: ";
        for (int i = 0; i < frame.can_dlc; ++i) {
            std::cout << "0x" << std::hex << (int)frame.data[i] << " ";
        }
        std::cout << std::endl;
    }
}

void CANCommunication::updateConnectionStatus() {
    while (connected) {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastReceiveTime).count();

        if (elapsed < 500) {
            connectionStatus = 2;  // 양호
        } else if (elapsed < 2000) {
            connectionStatus = 1;  // 미흡
        } else {
            connectionStatus = 0;  // 끊김
        }

        // 상태 출력
        std::string statusText = (connectionStatus == 2) ? "양호" : (connectionStatus == 1) ? "미흡" : "끊김";
        std::cout << "[CAN 통신 상태] " << statusText << " (마지막 수신: " << elapsed << "ms 전)" << std::endl;

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

void CANCommunication::enableCANSend(bool enable) {
    canSendEnabled.store(enable);
}

void CANCommunication::setSendPeriod(int periodMs) {
    sendPeriodMs.store(periodMs);
}
