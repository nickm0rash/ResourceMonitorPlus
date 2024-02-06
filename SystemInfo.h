#pragma once

#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <future>
#include <atomic>
#include <Winsock2.h>
#include <WS2tcpip.h>

class SystemInfo {
public:
	SystemInfo(); //Constructor
	~SystemInfo(); //Destructor	

	void UpdateMetricsAsync();  //Asynchronously update all metrics.

	double GetCpuUsage() const;
	double GetMemoryUsage() const;
	double GetDiskReadUsage() const;
	double GetDiskWriteUsage() const;
	double GetNetworkSentUsage() const;
	double GetNetworkReceivedUsage() const;
	std::string GetLocalIPv4Address() const;
	std::string GetLocalIPv6Address() const;
	std::vector<std::string> GetRunningProcesses() const;

	void PrintRunningProcesses(); //Print running processes to console DEBUG.

private:
	mutable std::mutex m_mutex; //Mutex to protect shared resources.	


	//Methods to update metrics.
	void UpdateCpuUsage();
	void UpdateMemoryUsage();
	void UpdateDiskUsage();
	void UpdateNetworkUsage();
	void UpdateRunningProcesses();

	//Member variables to store metrics.
	std::atomic<double> m_cpuUsage;
	std::atomic<double> m_memoryUsage;
	std::atomic<double> m_diskReadUsage;
	std::atomic<double> m_diskWriteUsage;
	std::atomic<double> m_networkSentUsage;
	std::atomic<double> m_networkReceivedUsage;
	std::vector<std::string> m_runningProcesses;
	std::vector<std::string> localIPv4Address;
	std::vector<std::string> localIPv6Address;

};