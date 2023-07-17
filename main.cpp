// Server side C/C++ program to demonstrate Socket
// programming
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <string>
#include "lib/json.hpp"
#include <stdio.h>
#include <mach/mach_init.h>
#include <mach/mach_error.h>
#include <mach/mach_host.h>
#include <mach/vm_map.h>
#include <stdlib.h> //to use bash shell
#include <iostream>

#define PORT 8080
#define DEVICE_NAME "pc-service"

// CPU MEM
static unsigned long long _previousTotalTicks = 0;
static unsigned long long _previousIdleTicks = 0;

// Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in between
// You'll need to call this at regular intervals, since it measures the load between
// the previous call and the current one.
float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks);
float GetCPULoad()
{
    host_cpu_load_info_data_t cpuinfo;
    mach_msg_type_number_t count = HOST_CPU_LOAD_INFO_COUNT;
    if (host_statistics(mach_host_self(), HOST_CPU_LOAD_INFO, (host_info_t)&cpuinfo, &count) == KERN_SUCCESS)
    {
        unsigned long long totalTicks = 0;
        for (int i = 0; i < CPU_STATE_MAX; i++)
            totalTicks += cpuinfo.cpu_ticks[i];
        return CalculateCPULoad(cpuinfo.cpu_ticks[CPU_STATE_IDLE], totalTicks);
    }
    else
        return -1.0f;
}

float CalculateCPULoad(unsigned long long idleTicks, unsigned long long totalTicks)
{
    unsigned long long totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
    unsigned long long idleTicksSinceLastTime = idleTicks - _previousIdleTicks;
    float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);
    _previousTotalTicks = totalTicks;
    _previousIdleTicks = idleTicks;
    return ret;
}
// END CPU

using json = nlohmann::json;

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buf[1024] = {0};
    json jsonResponse;

    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    address.sin_port = htons(PORT);

    // Binding newly created socket to given IP and verification
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        printf("socket bind failed...\n");
        exit(0);
    }
    else
        printf("Socket successfully binded..\n");

    // Now server is ready to listen and verification
    if ((listen(server_fd, 5)) != 0)
    {
        printf("Listen failed...\n");
        exit(0);
    }
    else
        printf("Server listening..\n");
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                             (socklen_t *)&addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    if (argc > 1 && strcmp(argv[1], "-configure") == 0)
    {
        std::cout << "binding mode...\n";
        bzero(buf, sizeof(buf));
        ssize_t rec = 0;
        do
        {
            int result = recv(new_socket, &buf[rec], sizeof(buf) - rec, 0);
            if (result == -1)
            {
                // Handle error ...
                break;
            }
            else if (result == 0)
            {
                // Handle disconnect ...
                break;
            }
            else
            {
                rec += result;
            }
        } while (rec < sizeof(buf));
        printf("buffer: %s\n", buf);
        jsonResponse = json::parse(buf);
        jsonResponse["device-name"] = DEVICE_NAME;
        jsonResponse["success"] = true;
        std::string resp = jsonResponse.dump();
        send(new_socket, resp.c_str(), strlen(resp.c_str()), 0);
    }
    else if (argc > 1 && strcmp(argv[1], "-set") == 0)
    {
        bzero(buf, sizeof(buf));
        ssize_t rec = 0;
        do
        {
            int result = recv(new_socket, &buf[rec], sizeof(buf) - rec, 0);
            if (result == -1)
            {
                // Handle error ...
                break;
            }
            else if (result == 0)
            {
                // Handle disconnect ...
                break;
            }
            else
            {
                rec += result;
            }
        } while (rec < sizeof(buf));
        std::cout << buf << "\n";
        jsonResponse = json::parse(buf);
        std::string modeS = jsonResponse["mode"];
        if (strcmp(modeS.c_str(), "set") == 0)
        {
            std::string programS = jsonResponse["program"];
            int status = system(programS.c_str());
        }
    }
    else
    {
        bzero(buf, sizeof(buf));
        ssize_t rec = 0;
        do
        {
            int result = recv(new_socket, &buf[rec], sizeof(buf) - rec, 0);
            if (result == -1)
            {
                // Handle error ...
                break;
            }
            else if (result == 0)
            {
                // Handle disconnect ...
                break;
            }
            else
            {
                rec += result;
            }
        } while (rec < sizeof(buf));
        printf("%s\n", buf);
        jsonResponse["mode"] = "get";
        jsonResponse["device"] = "pc-service";
        while (1)
        {
            try
            {
                jsonResponse["sensors"]["cpu"] = GetCPULoad();
                std::cout << "communicating sensors data\n";
                std::string resp = jsonResponse.dump();
                send(new_socket, resp.c_str(), strlen(resp.c_str()), MSG_NOSIGNAL);
                printf("message: %s\n", resp.c_str());
                sleep(1);
            }
            catch (char e)
            {
            }
        }
    }

    // closing the connected socket
    close(new_socket);
    // closing the listening socket
    shutdown(server_fd, SHUT_RDWR);
    return 0;
}
