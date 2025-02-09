#include "RS232Communication.h"
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <regex>

RS232Communication::RS232Communication(const std::string& sendPort, const std::string& receivePort)
    : sendPort(sendPort), receivePort(receivePort){}

RS232Communication::~RS232Communication() {
    stop();
}


void RS232Communication::run() {
    std::thread sender(&RS232Communication::sendData, this);
    std::thread receiver(&RS232Communication::receiveData, this);
    std::thread statusUpdater(&RS232Communication::monitorConnection, this);

    sender.join();
    receiver.join();
    statusUpdater.join();
}

void RS232Communication::sendData() {
    while (connected) {
        std::string data = generateNMEAData();
        std::string timestamp = getCurrentTimestamp();

        // 가상 직렬 포트에 데이터 전송
        std::ofstream serialPort(sendPort);
        if (serialPort.is_open()) {
            serialPort << timestamp << " - " << data << std::endl;
            std::lock_guard<std::mutex> lock(dataMutex);
            receivedData.push_back(timestamp + " - " + data);

            // 송신 데이터 출력
            std::cout << "[RS232 송신] " << timestamp << " - " << data << std::endl;
            serialPort.close();
        } else {
            std::cerr << "RS232 포트를 열 수 없습니다: " << sendPort << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(sendIntervalMs));  // 송신 간격
    }
}

void RS232Communication::receiveData() {
    while (connected) {
        std::ifstream serialPort(receivePort);
        if (serialPort.is_open()) {
            std::string receivedMessage;
            while (std::getline(serialPort, receivedMessage)) {
                std::string timestamp = getCurrentTimestamp();

                if (verifyChecksum(receivedMessage)) {
                    std::cout << "[RS232 수신] " << timestamp << " - " << receivedMessage << std::endl;
                    printNMEAMessage(receivedMessage);
                    lastReceivedTime = timestamp;
                    lastReceivedTimestamp = std::chrono::system_clock::now();
                } else {
                    std::cerr << "[RS232 수신 오류] 잘못된 체크섬: " << receivedMessage << std::endl;
                }
            }
            serialPort.close();
        } else {
            std::cerr << "RS232 포트를 열 수 없습니다: " << receivePort << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));  // 수신 대기 시간
    }
}

void RS232Communication::printNMEAMessage(const std::string& message) {
    std::size_t gpgga_pos = message.find("$GPGGA");
    std::size_t gphdt_pos = message.find("$GPHDT");
    std::size_t gpvtg_pos = message.find("$GPVTG");

    QString data;

    if (gpgga_pos != std::string::npos) {
        // GPGGA 메시지 처리
        std::string cleanedMessage = message.substr(gpgga_pos);
        std::vector<std::string> fields = parseNMEAMessage(cleanedMessage);

        if (fields.size() >= 15) {
            data += "[GPGGA 포맷]\n";
            data += "   - UTC 시간: " + QString::fromStdString(fields[1]) + "\n";
            data += "   - 위도: " + QString::fromStdString(fields[2]) + " " + QString::fromStdString(fields[3]) + "\n";
            data += "   - 경도: " + QString::fromStdString(fields[4]) + " " + QString::fromStdString(fields[5]) + "\n";
            data += "   - 고정 품질: " + QString::fromStdString(fields[6]) + "\n";
            data += "   - 사용 중 위성 수: " + QString::fromStdString(fields[7]) + "\n";
            data += "   - HDOP: " + QString::fromStdString(fields[8]) + "\n";
            data += "   - 고도: " + QString::fromStdString(fields[9]) + " " + QString::fromStdString(fields[10]) + "\n";
            data += "   - 지구 표면 간격: " + QString::fromStdString(fields[11]) + " " + QString::fromStdString(fields[12]) + "\n";
            data += "   - DGPS 데이터 지연: " + QString::fromStdString(fields[13]) + "\n";
            data += "   - DGPS 기준 ID: " + QString::fromStdString(fields[14]) + "\n";
        }
    } else if (gphdt_pos != std::string::npos) {
        // GPHDT 메시지 처리
        std::string cleanedMessage = message.substr(gphdt_pos);
        std::vector<std::string> fields = parseNMEAMessage(cleanedMessage);

        if (fields.size() >= 3) {
            data += "[GPHDT 포맷]\n";
            data += "   - 헤딩: " + QString::fromStdString(fields[1]) + " 도\n";
            data += "   - 방향: " + QString::fromStdString(fields[2]) + "\n";
        }
    } else if (gpvtg_pos != std::string::npos) {
        // GPVTG 메시지 처리
        std::string cleanedMessage = message.substr(gpvtg_pos);
        std::vector<std::string> fields = parseNMEAMessage(cleanedMessage);

        if (fields.size() >= 9) {
            data += "[GPVTG 포맷]\n";
            data += "   - 진북 기준 트랙 각도: " + QString::fromStdString(fields[1]) + " 도\n";
            data += "   - 속도 (노트): " + QString::fromStdString(fields[5]) + " 노트\n";
            data += "   - 속도 (킬로미터/시간): " + QString::fromStdString(fields[7]) + " km/h\n";
        }
    } else {
        data += "[알 수 없는 포맷]\n";
    }

    emit dataReceived(data);
}

std::vector<std::string> RS232Communication::parseNMEAMessage(const std::string& message) {
    std::vector<std::string> fields;
    std::stringstream ss(message);
    std::string field;
    while (std::getline(ss, field, ',')) {
        if (field.empty()) {
            fields.push_back("<빈 값>");
        } else {
            fields.push_back(field);
        }
    }
    return fields;
}

std::string RS232Communication::generateNMEAData() {
    std::random_device rd;
    std::uniform_int_distribution<> dist(0, 2);

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
    int hours = generateRandomInt(0, 23);
    int minutes = generateRandomInt(0, 59);
    int seconds = generateRandomInt(0, 59);
    int milliseconds = generateRandomInt(0, 99);

    double latitude = generateRandomDouble(0.0, 90.0);
    char lat_dir = (generateRandomInt(0, 1) == 0) ? 'N' : 'S';
    double longitude = generateRandomDouble(0.0, 180.0);
    char lon_dir = (generateRandomInt(0, 1) == 0) ? 'E' : 'W';

    std::string fix = std::to_string(generateRandomInt(0, 2));
    std::string nsat = std::to_string(generateRandomInt(0, 12));
    std::string hdop = formatDouble(generateRandomDouble(0.0, 99.9));
    std::string altitude = formatDouble(generateRandomDouble(-1000.0, 10000.0));
    std::string sep = formatDouble(generateRandomDouble(-9999.9, 9999.9));
    std::string dgps_age = formatDouble(generateRandomDouble(0.0, 999.9));
    std::string dgps_id = std::to_string(generateRandomInt(0, 1023));

    std::ostringstream oss;
    oss << "$GPGGA,"
        << std::setw(2) << std::setfill('0') << hours
        << std::setw(2) << std::setfill('0') << minutes
        << std::setw(2) << std::setfill('0') << seconds
        << "." << std::setw(2) << std::setfill('0') << milliseconds << ","
        << std::fixed << std::setprecision(4) << latitude << "," << lat_dir << ","
        << std::fixed << std::setprecision(4) << longitude << "," << lon_dir << ","
        << fix << ","
        << nsat << ","
        << hdop << ","
        << altitude << ",M," << sep << ",M,,"
        << "*" << calculateChecksum(oss.str());

    return oss.str();
}

std::string RS232Communication::generateGPHDT() {
    std::random_device rd;
    std::uniform_real_distribution<> dist(0.0, 360.0);
    std::ostringstream oss;
    oss << "$GPHDT," << formatDouble(dist(rd)) << ",T*" << calculateChecksum(oss.str());
    return oss.str();
}

std::string RS232Communication::generateGPVTG() {
    std::random_device rd;
    std::uniform_real_distribution<> dist(0.0, 360.0);
    std::uniform_real_distribution<> speedDist(0.0, 999.9);
    std::ostringstream oss;
    oss << "$GPVTG," << formatDouble(dist(rd)) << ",T,"
        << formatDouble(dist(rd)) << ",M,"
        << formatDouble(speedDist(rd)) << ",N,"
        << formatDouble(speedDist(rd) * 1.852) << ",K*"
        << calculateChecksum(oss.str());
    return oss.str();
}

std::string RS232Communication::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void RS232Communication::monitorConnection() {
    while (connected) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = now - lastReceivedTimestamp;

        if (elapsed_seconds.count() < 2) {
            std::cout << "[RS232 연결 상태] 양호" << std::endl;
        } else if (elapsed_seconds.count() < 5) {
            std::cout << "[RS232 연결 상태] 미흡" << std::endl;
        } else {
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

std::string RS232Communication::calculateChecksum(const std::string& sentence) {
    unsigned char checksum = 0;
    bool start = false;

    for (char ch : sentence) {
        if (ch == '$') {
            start = true;
            continue;
        }
        if (ch == '*') break;

        if (start) checksum ^= static_cast<unsigned char>(ch);
    }

    std::ostringstream hexStream;
    hexStream << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(checksum);
    return hexStream.str();
}

bool RS232Communication::verifyChecksum(const std::string& sentence) {
    // 타임스탬프 분리
    size_t pos = sentence.find("$");  // 첫 번째 '$' 위치 찾기
    if (pos == std::string::npos) {
        std::cerr << "NMEA Sentence 없음: " << sentence << std::endl;
        return false;
    }

    std::string timestamp = sentence.substr(0, pos);  // 앞쪽은 타임스탬프
    std::string nmeaSentence = sentence.substr(pos);  // '$'부터 시작하는 NMEA 문장

    // 정규식 적용 (NMEA 문장만 검사)
    std::regex checksumRegex("^(\\$[^*]+)\\*([0-9A-Fa-f]{2})$");
    std::smatch match;

    // std::cout << "타임스탬프: [" << timestamp << "]" << std::endl;
    // std::cout << "NMEA Sentence: [" << nmeaSentence << "]" << std::endl;

    if (std::regex_match(nmeaSentence, match, checksumRegex)) {
        // std::cout << "정규식 매칭 성공!" << std::endl;
        // std::cout << "본문: [" << match[1].str() << "]" << std::endl;
        // std::cout << "받은 체크섬: [" << match[2].str() << "]" << std::endl;

        // 체크섬 검증
        if (calculateChecksum(match[1].str()) == match[2].str()) {
            // std::cout << "체크섬 검증 성공!" << std::endl;
            return true;
        } else {
            std::cerr << "[RS232] 체크섬 불일치! 예상: [" << calculateChecksum(match[1].str())
                << "] / 받은 값: [" << match[2].str() << "]" << std::endl;
        }
    } else {
        std::cerr << "정규식 매칭 실패!" << std::endl;
    }

    return false;
}

void RS232Communication::enableRS232Send(bool enable) {
    rs232SendEnabled.store(enable);
}


void RS232Communication::setSendPeriod(int intervalMs) {
    sendIntervalMs.store(intervalMs);
}
