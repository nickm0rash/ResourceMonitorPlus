#pragma once

#include <string>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

class NetworkInfo {
	public:
	NetworkInfo(); //Constructor
	~NetworkInfo(); //Destructor

	std::wstring GetLocalIPAddress(bool inet6) const;
	std::wstring GetPublicIPAddress() const;
};