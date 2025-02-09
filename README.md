# vsensor
Virtual Communication Simulator
### Development Environment
   1. OS: Ubuntu 20.04
   2. Tools: Qt 6.2.4, C++
### Before Running1(Activate VCAN)
 1. VCAN 모듈 로드 
  - `sudo modprobe vcan`
 2. vcan0 디바이스 생성 
  - `sudo ip link add dev vcan0 type vcan`
 3. vcan0 활성화
  - `sudo ip link set up vcan0`
### Before Running2(Activate Virtual Serial port)
 1. socat을 사용하여 가상 직렬 포트 쌍 생성 
  - `socat -d -d pty,raw,echo=0 pty,raw,echo=0`
### Running Example
<img src="![GIFMaker_me](https://github.com/user-attachments/assets/05ff3d01-3aff-4f60-8990-f3f74143f32c)
">
