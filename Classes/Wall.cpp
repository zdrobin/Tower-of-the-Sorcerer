#include "Wall.h"


Wall::Wall(int imageIdx, const std::string& desc) :MyEvent(imageIdx, desc) {
}

bool Wall::triggerEvent()
{
	performActions();
	return false;
}

Wall * Wall::clone()
{
	return new Wall(*this);
}

Wall::
~Wall()
{
}
