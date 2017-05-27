#pragma once
#include "MyEvent.h"

class Wall :
	public MyEvent
{
public:
	Wall(int imageIdx, const std::string& desc);
	bool triggerEvent();
	Wall* clone();
	~Wall();
};