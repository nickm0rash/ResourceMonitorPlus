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

std::wstring NetworkInfo::GetPublicIPAddress() const
{
    
    //TODO: Implement this function
    return std::wstring();
}
