#pragma once

#include <cstdint>
#include <cstring>
#include <string>

class SsdData;
class Sheet;

class SsdViewer
{
	SsdData *m_ssd = nullptr;
	Sheet *m_sheet = nullptr;


public:
	SsdViewer();

	~SsdViewer();

	void Interface();
};

