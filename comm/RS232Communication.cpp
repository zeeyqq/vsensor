#include "RS232Communication.h"
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <mutex>

RS232Communication::RS232Communication(const std::string& port, int intervalMs)
    : port(port), intervalMs(intervalMs), connectionStatus(false), lastReceivedTime(""), lastReceivedTimestamp(std::chrono::system_clock::now()) {}

RS232Communication::~RS232Communication() {
    stop();
}

void RS232Communication::run() {
    std::thread sendThread(&RS232Communication::sendData, this);
    std::thread receiveThread(&RS232Communication::receiveData, this);
    std::thread monitorThread(&RS232Communication::monitorConnection, this);

    sendThread.join();
    receiveThread.join();
    monitorThread.join();
}

void RS232Communication::sendData() {
    while (isConnected()) {
        std::string data = generateNMEAData();
        std::string timestamp = getCurrentTimestamp();

        // 가상 직렬 포트에 데이터 전송
        std::ofstream serialPort(port);
        if (serialPort.is_open()) {
            serialPort << timestamp << " - " << data << std::endl;
            std::lock_guard<std::mutex> lock(dataMutex);
            receivedData.push_back(timestamp + " - " + data);

            // 송신 데이터 출력
            std::cout << "[RS232 송신] " << timestamp << " - " << data << std::endl;
            // printNMEAMessage(data);
            serialPort.close();
        } else {
            std::cerr << "RS232 포트를 열 수 없습니다: " << port << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(intervalMs));  // 송신 간격
    }
}

void RS232Communication::receiveData() {
    while (isConnected()) {
        // 가상 직렬 포트에서 데이터 수신
        std::ifstream serialPort(port);
        if (serialPort.is_open()) {
            std::string receivedMessage;
            while (std::getline(serialPort, receivedMessage)) {
                std::string timestamp = getCurrentTimestamp();
                std::cout << "[RS232 수신] " << timestamp << " - " << receivedMessage << std::endl;
                printNMEAMessage(receivedMessage);

                // 수신된 메시지가 있으면, 마지막 수신 시간 갱신
                lastReceivedTime = timestamp;
                lastReceivedTimestamp = std::chrono::system_clock::now();
            }
            serialPort.close();
        } else {
            std::cerr << "RS232 포트를 열 수 없습니다: " << port << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 수신 대기 시간
    }
}

void RS232Communication::printNMEAMessage(const std::string& message) {
    if (message.find("$GPGGA") == 0) {
        std::cout << " - [GPGGA 포맷] \n";
        std::cout << "   - UTC 시간: " << message.substr(7, 6) << "\n";
        std::cout << "   - 위도: " << message.substr(14, 9) << " " << message[24] << "\n";
        std::cout << "   - 경도: " << message.substr(25, 10) << " " << message[35] << "\n";
        std::cout << "   - 고정 품질: " << message[46] << "\n";
        std::cout << "   - 사용 중 위성 수: " << message[48] << "\n";
        std::cout << "   - HDOP: " << message.substr(50, 3) << "\n";
        std::cout << "   - 고도: " << message.substr(54, 5) << " 미터\n";
        std::cout << "   - 지구 표면 간격: " << message.substr(60, 4) << " 미터\n";
        std::cout << "   - DGPS 데이터 지연: " << message.substr(65, 3) << " 초\n";
        std::cout << "   - DGPS 기준 ID: " << message.substr(69, 4) << "\n";
    }
    else if (message.find("$GPHDT") == 0) {
        std::cout << " - [GPHDT 포맷] \n";
        std::cout << "   - 헤딩: " << message.substr(7, 4) << " 도\n";
        std::cout << "   - 방향: " << message[12] << " (True Heading)\n";
    }
    else if (message.find("$GPVTG") == 0) {
        std::cout << " - [GPVTG 포맷] \n";
        std::cout << "   - 진북 기준 트랙 각도: " << message.substr(7, 4) << " 도\n";
        std::cout << "   - 속도 (노트): " << message.substr(12, 4) << " 노트\n";
        std::cout << "   - 속도 (킬로미터/시간): " << message.substr(18, 4) << " km/h\n";
    }
    else {
        std::cout << " - [알 수 없는 포맷]\n";
    }
}

std::string RS232Communication::generateNMEAData() {
    std::random_device rd;
    std::uniform_int_distribution<> dist(0, 2);  // 0~2 중 랜덤 값 선택

    switch (dist(rd)) {
    case 0:
        return generateGPGGA();
    case 1:
        return generateGPHDT();
    case 2:
        return generateGPVTG();
    default:
        return "";
    }
}

std::string RS232Communication::generateGPGGA() {
    // 시간 생성 (00:00:00 ~ 23:59:59)
    int hours = generateRandomInt(0, 23);
    int minutes = generateRandomInt(0, 59);
    int seconds = generateRandomInt(0, 59);
    int milliseconds = generateRandomInt(0, 99);  // 소수점 이하 2자리

    // 위도 생성 (00.0000 ~ 90.0000)
    double latitude = generateRandomDouble(0.0, 90.0);
    char lat_dir = (generateRandomInt(0, 1) == 0) ? 'N' : 'S';  // 북반구 또는 남반구

    // 경도 생성 (000.0000 ~ 180.0000)
    double longitude = generateRandomDouble(0.0, 180.0);
    char lon_dir = (generateRandomInt(0, 1) == 0) ? 'E' : 'W';  // 동경 또는 서경

    // 고정 품질, 위성 수, HDOP, 고도, 지구 표면 간격, DGPS 지연 및 DGPS ID 값은 랜덤으로 생성
    std::string fix = std::to_string(generateRandomInt(0, 2));  // 고정 품질 0, 1, 2
    std::string nsat = std::to_string(generateRandomInt(0, 12));  // 사용 중 위성 수
    std::string hdop = std::to_string(generateRandomDouble(0.0, 99.9));  // HDOP
    std::string altitude = std::to_string(generateRandomDouble(-1000.0, 10000.0));  // 고도 (미터)
    std::string sep = std::to_string(generateRandomDouble(-9999.9, 9999.9));  // 지구 표면 간격
    std::string dgps_age = std::to_string(generateRandomDouble(0.0, 999.9));  // DGPS 데이터 지연 (초)
    std::string dgps_id = std::to_string(generateRandomInt(0, 1023));  // DGPS 기준 ID

    // 문자열로 포맷팅
    std::ostringstream oss;
    oss << "$GPGGA,"
        << std::setw(2) << std::setfill('0') << hours
        << std::setw(2) << std::setfill('0') << minutes
        << std::setw(2) << std::setfill('0') << seconds
        << "." << std::setw(2) << std::setfill('0') << milliseconds << ","  // 시간 포맷
        << std::fixed << std::setprecision(4) << latitude << "," << lat_dir << ","
        << std::fixed << std::setprecision(4) << longitude << "," << lon_dir << ","
        << fix << ","
        << nsat << ","
        << hdop << ","
        << altitude << ",M,"
        << sep << ",M,,"
        << "*" << dgps_id;

    return oss.str();
}

std::string RS232Communication::generateGPHDT() {
    std::random_device rd;
    std::uniform_real_distribution<> dist(0.0, 360.0);
    return "$GPHDT," + formatDouble(dist(rd)) + ",T*42";  // 예시 헤딩 값
}

std::string RS232Communication::generateGPVTG() {
    std::random_device rd;
    std::uniform_real_distribution<> dist(0.0, 360.0);
    std::uniform_real_distribution<> speedDist(0.0, 999.9);  // 속도 (노트)
    return "$GPVTG," + formatDouble(dist(rd)) + ",T," +
           formatDouble(dist(rd)) + ",M," +
           formatDouble(speedDist(rd)) + ",N," +
           formatDouble(speedDist(rd) * 1.852) + ",K*6A";  // 속도 (킬로미터/시간)
}

std::string RS232Communication::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void RS232Communication::monitorConnection() {
    while (isConnected()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));  // 1초마다 상태 체크

        // 현재 시간과 마지막 수신 시간을 비교하여 연결 상태 확인
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = now - lastReceivedTimestamp;

        if (elapsed_seconds.count() < 2) {
            // 양호: 최근 2초 이내에 메시지 수신됨
            std::cout << "[RS232 연결 상태] 양호" << std::endl;
        } else if (elapsed_seconds.count() < 5) {
            // 미흡: 2초 이상 5초 이내에 메시지 수신됨
            std::cout << "[RS232 연결 상태] 미흡" << std::endl;
        } else {
            // 끊김: 5초 이상 메시지 수신 안 됨
            std::cout << "[RS232 연결 상태] 끊김" << std::endl;
        }
    }
}

std::string RS232Communication::formatDouble(double val) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(4) << val;
    return ss.str();
}

int RS232Communication::generateRandomInt(int min, int max) {
    std::random_device rd;
    std::uniform_int_distribution<> dist(min, max);
    return dist(rd);
}

double RS232Communication::generateRandomDouble(double min, double max) {
    std::random_device rd;
    std::uniform_real_distribution<> dist(min, max);
    return dist(rd);
}
