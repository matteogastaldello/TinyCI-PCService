# TinyCI-PCService

## Introduction
This repo has been designed to test TinyCI infrastructure and show an alternative use of our infrastructure (communicating to a PC instead of an embedded device).

Note. The code for retrieving CPU utilization is SO specific (in particular it is designed for macOS). Instead, the communication part should be working on all *NIX devices)

## How to get started (on macOS only)
Compilation:

```console
gcc main.cpp -o outFolder/main.out -std=c++11  -lstdc++
```

Running the program:
- *configure mode*: designed to pair the pcService with the TinyCI-HUB. Configure mode need to be active before activating discovery mode on the Hub (from the platform). Command to launch is ```outFolder/main.out -configure```
- *set mode*: designed to launch shell command on the PC specifying dynamically on the TinyCI-Dashboard. *configure mode* need to be launched before setting this mode.  Command to launch is ```outFolder/main.out -set```
- *get mode*: designed to send CPU utilization data to the TinyCI-Dashboard (that graph and display the data). *configure mode* need to be launched before setting this mode. Command to launch is ```outFolder/main.out -get```

## Communication with the TinyCI-HUB

Code for the *configuration mode*

```C++
int server_fd, new_socket, valread;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buf[1024] = {0}; //buf lenght need to be fixed to 1024 due to socket receiving. This value can be changed only if modified accordingly on TinyCI-HUB (when the hub send data)
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
        //Receiving data from socket
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
        /*
          in configuration mode response must be the received json from the hub with some field added:
          {
            "device-name": "your device name",
            "success": true
          }
        */
        std::string resp = jsonResponse.dump();
        send(new_socket, resp.c_str(), strlen(resp.c_str()), 0);
    }
```

For set mode and get mode refer the main.cpp file on this repo. All the JSON required for communication are defined in [TinyCI-Dashboard Readme](https://github.com/NatFederico/TinyCI-Dashboard)

# Authors

Matteo Gastaldello

Federico Natali

Matteo Sabella

# Acknowledgments

<a href="https://www.unitn.it/">
  <img src="/assets/logo_unitn_it.png" width="300px">
</a>
