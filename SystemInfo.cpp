//SystemInfo.cpp : This file encapsulates system information and retrieval logic.

#include "SystemInfo.h"
#include <Pdh.h>
#include <Winsock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <sstream>


#pragma comment(lib, "pdh.lib")
#pragma comment(lib, "ws2_32.lib")


//Global variables for performance data.
PDH_HQUERY cpuQuery;
PDH_HQUERY diskQuery;
PDH_HQUERY networkQuery;

PDH_HCOUNTER cpuTotal;
PDH_HCOUNTER diskRead;
PDH_HCOUNTER diskWrite;
PDH_HCOUNTER networkSent;
PDH_HCOUNTER networkReceived;


//Initialize atomic variables.
SystemInfo::SystemInfo() {
    m_cpuUsage = 0.0;
    m_memoryUsage = 0.0;
    m_diskReadUsage = 0.0;
    m_diskWriteUsage = 0.0;
    m_networkSentUsage = 0.0;
    m_networkReceivedUsage = 0.0;

    //Intialize PDH queries and counters.
    PdhOpenQuery(NULL, NULL, &cpuQuery);
    PdhAddEnglishCounter(cpuQuery, L"\\Processor(_Total)\\% Processor Time", NULL, &cpuTotal);
    PdhCollectQueryData(cpuQuery);

    PdhOpenQuery(NULL, NULL, &diskQuery);
    PdhAddCounter(diskQuery, L"\\PhysicalDisk(_Total)\\Disk Read Bytes/sec", NULL, &diskRead);
    PdhAddCounter(diskQuery, L"\\PhysicalDisk(_Total)\\Disk Write Bytes/sec", NULL, &diskWrite);
    PdhCollectQueryData(diskQuery);

    PdhOpenQuery(NULL, NULL, &networkQuery);
    PdhAddCounter(networkQuery, L"\\Network Interface(*)\\Bytes Sent/sec", NULL, &networkSent);
    PdhAddCounter(networkQuery, L"\\Network Interface(*)\\Bytes Received/sec", NULL, &networkReceived);
    PdhCollectQueryData(networkQuery);
}

//Destructor
SystemInfo::~SystemInfo() {
    PdhCloseQuery(cpuQuery);
    PdhCloseQuery(diskQuery);
    PdhCloseQuery(networkQuery);
}

void SystemInfo::UpdateMetricsAsync() {
    //Start a new thread for each metric.
    auto cpuFuture = std::async(std::launch::async, &SystemInfo::UpdateCpuUsage, this);
    auto memoryFuture = std::async(std::launch::async, &SystemInfo::UpdateMemoryUsage, this);
    auto diskFuture = std::async(std::launch::async, &SystemInfo::UpdateDiskUsage, this);
    auto networkFuture = std::async(std::launch::async, &SystemInfo::UpdateNetworkUsage, this);
    auto processesFuture = std::async(std::launch::async, &SystemInfo::UpdateRunningProcesses, this);

    //Wait for all threads to finish.
    cpuFuture.wait();
    memoryFuture.wait();
    diskFuture.wait();
    networkFuture.wait();
    processesFuture.wait();

}


//Getters for each metric.
double SystemInfo::GetCpuUsage() const {
    return m_cpuUsage.load();
}

double SystemInfo::GetMemoryUsage() const {
    return m_memoryUsage.load();
}

double SystemInfo::GetDiskReadUsage() const {
    return m_diskReadUsage.load();
}

double SystemInfo::GetDiskWriteUsage() const {
    return m_diskWriteUsage.load();
}

double SystemInfo::GetNetworkSentUsage() const {
    return m_networkSentUsage.load();
}

double SystemInfo::GetNetworkReceivedUsage() const {
    return m_networkReceivedUsage.load();
}

std::vector<std::string> SystemInfo::GetRunningProcesses() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_runningProcesses;
}

std::string SystemInfo::GetLocalIPv4Address() const {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return "0.0.0.0";
    }

    ADDRINFO hints{}, * result = nullptr;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET; // IPv4 addresses only
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(nullptr, nullptr, &hints, &result) != 0) {
        WSACleanup();
        return "0.0.0.0";
    }

    char ipStr[INET_ADDRSTRLEN];
    for (ADDRINFO* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
        inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(ptr->ai_addr)->sin_addr, ipStr, sizeof(ipStr));
        break; // TODO: Pick LAN address, not first address.
    }

    freeaddrinfo(result);
    WSACleanup();
    return ipStr;
}

	
std::string SystemInfo::GetLocalIPv6Address() const {
	WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return "0.0.0.0";
    }

    ADDRINFO hints{}, * result = nullptr;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET6; // IPv6 addresses only
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    if (getaddrinfo(nullptr, nullptr, &hints, &result) != 0) {
		WSACleanup();
        return "0.0.0.0";
    }

    char ipStr[INET6_ADDRSTRLEN];
    for (ADDRINFO* ptr = result; ptr != nullptr; ptr = ptr->ai_next) {
		inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(ptr->ai_addr)->sin6_addr, ipStr, sizeof(ipStr));
		break; // TODO: Pick LAN address, not first address.
	}
    freeaddrinfo(result);
    WSACleanup();
    return ipStr;
}

//Update for each metric.
void SystemInfo::UpdateCpuUsage() {
    PDH_FMT_COUNTERVALUE counterVal;
	PdhCollectQueryData(cpuQuery);
	PdhGetFormattedCounterValue(cpuTotal, PDH_FMT_DOUBLE, NULL, &counterVal);
	m_cpuUsage.store(counterVal.doubleValue);
}

void SystemInfo::UpdateMemoryUsage() {
	MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(MEMORYSTATUSEX);
	GlobalMemoryStatusEx(&memInfo);
	m_memoryUsage.store(memInfo.dwMemoryLoad);
}

void SystemInfo::UpdateDiskUsage() {
	PDH_FMT_COUNTERVALUE counterVal;
	PdhCollectQueryData(diskQuery);
	PdhGetFormattedCounterValue(diskRead, PDH_FMT_LARGE, NULL, &counterVal);
	m_diskReadUsage.store(counterVal.largeValue);
	PdhGetFormattedCounterValue(diskWrite, PDH_FMT_LARGE, NULL, &counterVal);
	m_diskWriteUsage.store(counterVal.largeValue);
}

void SystemInfo::UpdateNetworkUsage() {
	PDH_FMT_COUNTERVALUE counterVal;
	PdhCollectQueryData(networkQuery);
	PdhGetFormattedCounterValue(networkSent, PDH_FMT_LARGE, NULL, &counterVal);
	m_networkSentUsage.store(counterVal.largeValue);
	PdhGetFormattedCounterValue(networkReceived, PDH_FMT_LARGE, NULL, &counterVal);
	m_networkReceivedUsage.store(counterVal.largeValue);
}


//Update list of running processes, using the Windows Tool Help library (TlHelp32.h).
void SystemInfo::UpdateRunningProcesses() {
	std::lock_guard<std::mutex> lock(m_mutex);
	m_runningProcesses.clear();

	HANDLE hProcessSnap;
	PROCESSENTRY32 pe32;

	hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
		return;
	}

	pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
		CloseHandle(hProcessSnap);
		return;
	}
    do {
        char buffer[MAX_PATH];
        WideCharToMultiByte(CP_UTF8, 0, pe32.szExeFile, -1, buffer, MAX_PATH, NULL, NULL);
        m_runningProcesses.push_back(buffer);
	} while (Process32Next(hProcessSnap, &pe32));

	CloseHandle(hProcessSnap);
}

//DEBUGGING FUNCTION
void SystemInfo::PrintRunningProcesses() {
    UpdateMetricsAsync();
    std::wstringstream ss;
    ss << L"CPU Usage: " << GetCpuUsage() << L"%\n";
    ss << L"Memory Usage: " << GetMemoryUsage() << L"%\n";
    ss << L"Disk Read: " << GetDiskReadUsage() << L" bytes/sec\n";
    ss << L"Disk Write: " << GetDiskWriteUsage() << L" bytes/sec\n";
    ss << L"Network Sent: " << GetNetworkSentUsage() << L" bytes/sec\n";
    ss << L"Network Received: " << GetNetworkReceivedUsage() << L" bytes/sec\n";
    std::vector<std::string> processes = GetRunningProcesses();
    for (auto& process : processes) {
		ss << process.c_str() << L", ";
	}
    MessageBoxW(NULL, ss.str().c_str(), L"Running Processes", MB_OK);
}
