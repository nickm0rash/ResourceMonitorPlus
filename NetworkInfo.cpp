#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include "NetworkInfo.h"

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")

NetworkInfo::NetworkInfo() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        // Handle error
    }
}

NetworkInfo::~NetworkInfo() {
    WSACleanup();
}



std::wstring ConvertToWideString(const std::string& input) {
    std::wstring wideString;
    wideString.reserve(input.length());
    for (char c : input) {
        wideString.push_back(static_cast<wchar_t>(c));
    }
    return wideString;
}

std::wstring NetworkInfo::GetLocalIPAddress(bool useIPv6 = false) const{
    ULONG flags = GAA_FLAG_INCLUDE_PREFIX;
    ULONG family = useIPv6 ? AF_INET6 : AF_INET;
    ULONG bufferSize = 0;
    std::wstring ipAddress = L"No Address Found";

    // First call to determine buffer size needed
    DWORD dwRetVal = GetAdaptersAddresses(family, flags, nullptr, nullptr, &bufferSize);
    if (dwRetVal != ERROR_BUFFER_OVERFLOW) return ipAddress; // Unexpected error

    std::vector<BYTE> buffer(bufferSize);
    PIP_ADAPTER_ADDRESSES pAddresses = reinterpret_cast<PIP_ADAPTER_ADDRESSES>(buffer.data());

    // Second call to retrieve data
    dwRetVal = GetAdaptersAddresses(family, flags, nullptr, pAddresses, &bufferSize);
    if (dwRetVal != NO_ERROR) return ipAddress; // Unable to get addresses

    for (PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses; pCurrAddresses != nullptr; pCurrAddresses = pCurrAddresses->Next) {
        for (PIP_ADAPTER_UNICAST_ADDRESS pUnicast = pCurrAddresses->FirstUnicastAddress; pUnicast != nullptr; pUnicast = pUnicast->Next) {
            char ipStr[INET6_ADDRSTRLEN] = {0};

            if (pUnicast->Address.lpSockaddr->sa_family == family) {
                inet_ntop(family, family == AF_INET ? 
                    (void*)&((struct sockaddr_in*)pUnicast->Address.lpSockaddr)->sin_addr : 
                    (void*)&((struct sockaddr_in6*)pUnicast->Address.lpSockaddr)->sin6_addr,
                    ipStr, sizeof(ipStr));

                // Skip unspecified addresses (0.0.0.0 and ::)
                if (strcmp(ipStr, "0.0.0.0") != 0 && strcmp(ipStr, "::") != 0) {
                    return ConvertToWideString(ipStr);
                }
            }
        }
    }

    return ipAddress; // No suitable address found
}

std::wstring NetworkInfo::GetPublicIPAddress() const {
    WSADATA wsaData;
    SOCKET Socket = INVALID_SOCKET;
    struct addrinfo* result = NULL, * ptr = NULL, hints;
    int resultCode;
    char buffer[10000];
    std::string response;

    // Initialize Winsock
    resultCode = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (resultCode != 0) {
        std::cerr << "WSAStartup failed with error: " << resultCode << "\n";
        return L"";
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    resultCode = getaddrinfo("api.ipify.org", "80", &hints, &result);
    if (resultCode != 0) {
        std::cerr << "getaddrinfo failed with error: " << resultCode << "\n";
        WSACleanup();
        return L"";
    }

    // Attempt to connect to the first address returned by the call to getaddrinfo
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        Socket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        if (Socket == INVALID_SOCKET) {
            std::cerr << "socket failed with error: " << WSAGetLastError() << "\n";
            WSACleanup();
            return L"";
        }

        // Connect to server.
        resultCode = connect(Socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (resultCode == SOCKET_ERROR) {
            closesocket(Socket);
            Socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    freeaddrinfo(result);

    if (Socket == INVALID_SOCKET) {
        std::cerr << "Unable to connect to server!\n";
        WSACleanup();
        return L"";
    }

    // Send an HTTP GET request
    const char* sendbuf = "GET / HTTP/1.1\r\nHost: api.ipify.org\r\nConnection: close\r\n\r\n";
    resultCode = send(Socket, sendbuf, (int)strlen(sendbuf), 0);
    if (resultCode == SOCKET_ERROR) {
        std::cerr << "send failed with error: " << WSAGetLastError() << "\n";
        closesocket(Socket);
        WSACleanup();
        return L"";
    }

    // Receive until the peer closes the connection
    do {
        resultCode = recv(Socket, buffer, sizeof(buffer), 0);
        if (resultCode > 0) {
            response.append(buffer, resultCode);
        }
        else if (resultCode == 0) {
            std::cout << "Connection closed\n";
        }
        else {
            std::cerr << "recv failed with error: " << WSAGetLastError() << "\n";
        }
    } while (resultCode > 0);

    // Clean up
    closesocket(Socket);
    WSACleanup();

    // Parse response to extract IP address
    auto ipStart = response.find("\r\n\r\n");
    if (ipStart != std::string::npos) {
        std::string ipStr = response.substr(ipStart + 4); // Skip header
        return ConvertToWideString(ipStr);
    }

    return L"No IP Found";
}

