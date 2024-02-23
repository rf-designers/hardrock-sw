#include "serial/serial.h"
#include <iostream>
#include <sstream>
#include <thread>
#include <chrono>

bool is_hardware_port(const serial::PortInfo& p) {
    return p.hardware_id.rfind("USB", 0) == 0;
}

bool is_amp_on_port(const serial::PortInfo& p) {
    bool result = false;
    try {
        auto s = serial::Serial(p.port, 115200, serial::Timeout::simpleTimeout(50)); // rest of params are default
        s.write(" ;");
        s.write("HRAA;");
        if (s.waitReadable()) {
            auto response = s.readline (5, ";");
            if (response == "HRAA;") {
                result = true;
            }
        }
        s.close();
    } catch (...) {
        std::cerr << "Failed to open port " << p.port << std::endl;
    }
    return result;

}

void upload_firmware_on_port(const serial::PortInfo& p, uint32_t delay_ms) {
    try {
        auto s = serial::Serial(p.port, 115200, serial::Timeout::simpleTimeout(50)); // rest of params are default
        s.write(" ;");
        s.write("HRAA;");
        if (s.waitReadable()) {
            auto response = s.readline (5, ";");
            if (response == "HRAA;") {
                std::cout << "opened!" << std::endl;
                s.write("HRFW;");
                s.flush();
                std::cout << "sent hrfw!" << std::endl;
                s.close();

                std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));

                std::stringstream command;
                command << "avrdude -v -patmega2560 -cwiring -P" << p.port << " -b115200 -D -Uflash:w:.pio/build/megaatmega2560/firmware.hex:i";
                system(command.str().c_str());
            }
        }


        s.close();
    } catch (...) {
        std::cerr << "Failed to open port " << p.port << std::endl;
    }

    // avrdude -v -patmega2560 -cwiring -P/dev/ttyUSB2 -b115200 -D -Uflash:w:.pio/build/megaatmega2560/firmware.hex:i    
}

int main(int argc, char **argv) {
    auto ports = serial::list_ports();
    for (auto &p : ports) {
        if (is_hardware_port(p)) {
            std::cout << p.port << " (desc: " << p.description << ",hw_id: " << p.hardware_id << ")" <<  std::endl;
            if (is_amp_on_port(p)) {
                std::cout << "- amp found on port " << p.port <<  std::endl;

                uint32_t delay_ms = 10;
                if (argc > 1) {
                    delay_ms = std::stol(argv[1]);
                }
                upload_firmware_on_port(p, delay_ms);
            }
        }
    }
    return 0;
}