#include "GameData.h"
#include <queue>
#include "Wall.h"
#include "Enemy.h"
#include "Consumable.h"
#include "Door.h"
#include "Configureader.h"
#include "DialogStruct.h"
#include <set>
#include "Stairs.h"
#include "GlobalEvent.h"
#include "GlobalDefs.h"
#include <string>

USING_NS_CC;
using namespace twsutil;

static GameData* gameData = nullptr;

GameData::GameData() {
	init();
}

void GameData::init()
{
	newGame = true;
	floorChange = false;
	upstair = nullptr;
	downstair = nullptr;
	blockCounter = 0;
	blocked = false;

	hero = std::make_unique<HeroX>(213, "Hero", 1000, 20, 100, 0);
	floor = std::make_unique<LabelBinder<int>>(1);

	Configureader::ReadFloorEvents(FLOOREVENTS);
	Configureader::ReadItemData(ITEMS);
	Configureader::ReadGlobalEvents(GLOBALEVENT);

	CCLOG("GameData configuration reading done");

	loadFloor(3);

	CCLOG("GameData floor loaded");
}


//kinda singleton
GameData& GameData::getInstance() {
	if (!gameData) {
		gameData = new GameData();
	}
	return *gameData;
}


std::shared_ptr<MyEvent> GameData::getEvent(int x, int y)
{
	return FloorEvents[x][y];
}

//get fresh usable event data given the id
std::unique_ptr<MyEvent> GameData::getEventData(int id)
{
	if (id < 0 || id >= MAXEVENT) {
		return nullptr;
	}
	return Configureader::getEvent(id);
}

int GameData::getEventID(int floorNum, int x, int y) {
	return FLOOREVENTS[floorNum][x][y];
}


//get event based on the location of the current floor
std::unique_ptr<MyEvent> GameData::getEventData(int x, int y)
{
	return getEventData(FLOOREVENTS[floor->V()][x][y]);
}

void GameData::showDialog(std::queue<DialogStruct>& dq, std::function<void(int)> callback)
{
	//if (callback)
	//add to queue of dialogCallback, each dialog dismiss will pop from the queue
	dialogCallbackQ.push(callback);
	if (dialogQ.empty())
		dialogQ = dq; //make a copy
	else { //append dq to dialogQ
		while (!dq.empty()) {
			dialogQ.push(dq.front());
			dq.pop();
		}
		return;
	}

	//draw the topmost dialog
	if (!dialogQ.empty()) {
		DialogStruct& ds = dialogQ.front();
		flScn->drawDialog(ds.text, ds.dialogType, ds.options);
		dialogQ.pop();
	}
}

void GameData::showDialog(const DialogStruct& ds, std::function<void(int)> callback)
{
	dialogQ = std::queue<DialogStruct>(); //empty the queue
	dialogCallbackQ.push(callback);
	flScn->drawDialog(ds.text, ds.dialogType, ds.options);
}

//this is called from floorscene to return the choice of the dialog
void GameData::dialogCompleted(int choice)
{
	//check if there is another dialog to display
	if (!dialogQ.empty()) {
		DialogStruct& ds = dialogQ.front();
		flScn->drawDialog(ds.text, ds.dialogType, ds.options);
		dialogQ.pop();
	}
	else { //if not, call callback
		if (!dialogCallbackQ.empty()) {
			auto callback = dialogCallbackQ.front();
			dialogCallbackQ.pop();
			if (callback)
				callback(choice);
		}
	}
}

void GameData::gameover()
{
	CCLOG("enabled input on gameover");
	blocked = false;
	// Director::getInstance()->getEventDispatcher()->setEnabled(true);
	showDialog(DialogStruct(GStr("gameover"), DIALOGTYPE::NONE), [this](int notUsed)->void {
		delete gameData;
		Director::getInstance()->popScene();
	});

}

void GameData::attachEnemyInfo(Fightable * enemy)
{
	enemy->hp.attach(eHpLabel);
	enemy->atk.attach(eAtkLabel);
	enemy->def.attach(eDefLabel);
	eDescLabel->setString(enemy->getDescription());
	auto sprite = enemy->getSprite(true);
	flScn->drawEnemyPortrait(sprite);
}

void GameData::obtainItem(int idx)
{
	if (idx < 0 || idx >= MAXITEMS)
		return;
	if (!ITEMS[idx])
		return;
	ITEMS[idx]->attachTo(flScn);
}

//the other way to do this is do it for each floor load
//and delete from enemy list
void GameData::showFloorEnemyStats()
{
	hero->StopAllFinal(nullptr);
	//store the set of unique pointers
	//then dynamic cast them to enemy
	std::set<std::shared_ptr<MyEvent>> eventSet;
	for (int i = 0; i < 11; i++)
		for (int j = 0; j < 11; j++) {
			// this gets a fresh copy, if the enemy still exists
			auto evt = getEvent(i, j);
			if (evt) {
				eventSet.insert(evt);
			}
		}

	//sprite, description, hp, atk, def, expectedDmg
	std::vector<std::tuple<Sprite*, std::string, int, int, int, int>> displayInfo;

	for (auto& evt : eventSet) {
		if (auto e = std::dynamic_pointer_cast<Enemy>(evt)) {
			int expectedDmg = hero->fight(e.get(), nullptr, nullptr);
			displayInfo.emplace_back(e->getSprite(true), e->getDescription(), e->getHp(), e->getAtk(), e->getDef(), expectedDmg);
		}
	}

	flScn->showFloorEnemyStats(displayInfo);
}

//used for HeroItem id 1
bool GameData::fastStairs()
{
	bool isBesideStair = false;
	//check if it is beside a stair
	int curX = hero->getX();
	int curY = hero->getY();
	int dirX[] = { 1,0,-1,0 };
	int dirY[] = { 0,1,0,-1 };
	for (int i = 0; i < 4; i++) {
		int newX = curX + dirX[i];
		int newY = curY + dirY[i];
		if (0 <= newX&&newX < 11 && 0 <= newY&&newY < 11) {
			auto evt = getEvent(newX, newY);
			if (dynamic_cast<Stairs *>(evt.get())) {
				isBesideStair = true;
				break;
			}
		}
	}
	if (isBesideStair) {
		CCLOG("is beside stair");
		showDialog(DialogStruct("Go up or down stairs?", DIALOGTYPE::LIST, { "UP","DOWN","CANCEL" }), [this](int choice) {
			CCLOG("made a choice");
			if (choice == 0) { //up
				if (upstair != nullptr) {
					upstair->triggerEvent();
				}
			}
			else if (choice == 1) {
				if (downstair != nullptr) {
					downstair->triggerEvent();
				}
			}
			else { //cancel
				return;
			}
			fastStairs();
		});
	}
	else {
		log("you need to be beside a staircase");
	}
	return isBesideStair;
}

//used for item id 2
void GameData::replayDialog() {
	//TODO fix this
	showDialog(DialogStruct("this is a matrix", DIALOGTYPE::MATRIX, { "1","2","3","4","5","6","7","8","9" }), [this](int choice) {
		log("you choosed " + std::to_string(choice));
	});
}


void GameData::loadFloor(int nextFloor) {
	floor->setVal(nextFloor); //set the text (and the internal value)
	std::shared_ptr<Stairs> stairs = nullptr;
	downstair = nullptr;
	upstair = nullptr;

	//remove hero sprite from current scene
	//hero doesn't get deleted, so we need to remove the sprite manually
	if (hero->sprite != nullptr)
		hero->sprite->removeFromParent();
	hero->sprite = nullptr;


	for (int i = 0; i < 11; i++)
		for (int j = 0; j < 11; j++) {
			FloorEvents[i][j] = nullptr;
			std::unique_ptr<MyEvent> event = getEventData(i, j);
			if (event) {
				//CCLOG("processing %d %d",i,j);
				FloorEvents[i][j] = std::move(event);
				FloorEvents[i][j]->setXY(i, j);
				if (stairs = std::dynamic_pointer_cast<Stairs>(FloorEvents[i][j])) {
					int sf = stairs->getTargetFloor();
					if (sf < nextFloor)
						downstair = stairs;
					else
						upstair = stairs;
				}
			}
		}
	CCLOG("floor set to %d", nextFloor);
}


int GameData::setFloor(int f) {
	loadFloor(f);
	flScn->loadFloor();
	floorChange = true;
	return floor->V();
}

cocos2d::Sprite * GameData::getSprite(int x, int y)
{
	if (!FloorEvents[x][y])
		return nullptr;
	return FloorEvents[x][y]->getSprite();
}

void GameData::moveHero(DIR dir)
{
	auto newXY = hero->getDirXY(dir);
	auto x = newXY.first;
	auto y = newXY.second;
	if (x < 0 || x >= 11 || y < 0 || y >= 11)
	{
		return;
	}
	moveHero(newXY);
}

//this method is only being called from FloorScene
//no one else should call this
void GameData::moveHero(std::pair<int, int> dest) {

	auto newPath = pathFind(dest);
	if (newPath.empty())
		return;

	logLabel->setVisible(false);
	logLabel->setString(""); //reset log text on movement

	heroMovementPath = newPath;
	CCLOG("next step updated to %d", heroMovementPath.size());

	if (!hero->moving()) {
		CCLOG("next step activated");
		hero->setMoving(true);
		nextStep();
	}
}

void GameData::nextStep() {
	CCLOG("next step with %d steps left", heroMovementPath.size());
	if (heroMovementPath.empty()) {
		auto eventPtr = getEvent(hero->getX(), hero->getY());
		if (eventPtr) {
			eventPtr->stepOnEvent();
		}
		finalMovementCleanup(false);
		releaseBlock();
		return;
	}
	// Safely, make a step
	auto step = heroMovementPath.back();
	// Non interruptible
	if (heroMovementPath.size()==1) {
		// Block input
		block();
		// Must consider event triggers for the last step
		std::shared_ptr<MyEvent> eventPtr = getEvent(step.first, step.second);
		if (eventPtr) {
			// Not allowed to move!
			if (!eventPtr->triggerEvent()) { 
				flScn->movementActive = false;
				heroMovementPath.clear();
				nextStep(); 
				return;
			}
		}
	}

	heroMovementPath.pop_back();
	// Move step must have a callback to nextStep
	hero->moveOnestep(step);
}


void GameData::continousMovement()
{
	flScn->continousMovement();
}

//called after final movement
void GameData::finalMovementCleanup(bool cont)
{
	if (blockCounter==0)
		showLog();
	triggerGlobalEvents();
	CCLOG("enabled input on finalmovementcleanup");
	if (cont)
		continousMovement();
}


void GameData::killEvent(std::pair<int, int> place) {
	FLOOREVENTS[floor->V()][place.first][place.second] = 0;
	FloorEvents[place.first][place.second] = nullptr;
}

void GameData::setEvent(int id, int x, int y, int f)
{
	if (f == -1)
		f = floor->V();
	FLOOREVENTS[floor->V()][x][y] = id;
	if (f == floor->V()) {
		auto curEvt = FloorEvents[x][y];
		if (curEvt&&id != 0) { //if there is a new sprite, then replace it now
			curEvt->sprite->removeFromParentAndCleanup(true);
			curEvt->sprite = nullptr; //remove sprite but do not delete it yet?
		}
		auto newEvt = getEventData(id);
		if (newEvt) {
			FloorEvents[x][y] = std::move(newEvt);
			FloorEvents[x][y]->setXY(x, y);
			if (FloorEvents[x][y])
				flScn->attachFloorSprite(FloorEvents[x][y]->getSprite());
		} else {
			FloorEvents[x][y] = nullptr;
		}
	}

	//reload floor
	//flScn->loadFloor();
}


PATH GameData::pathFind(std::pair<int, int> dest) {
	return pathFind(dest.first, dest.second);
}

//find a path from the current location (heroX,heroY) to the specified path
//use simple bfs will gurantee shortest path, since distance between each block is always 1
//returns the path in reverse
//this is called if the destination is not an event
//change to accept 1 obstacle.
PATH GameData::pathFind(int dx, int dy)
{

	if (dx == -1 || dy == -1 || hero->getX() == dx&&hero->getY() == dy) {
		return { }; //do nothing
	}

	//||getEvent(dx,dy)

	int vis[11][11] = { 0 };
	vis[hero->getX()][hero->getY()] = 1;
	std::vector<std::pair<int, int> > path;

	std::vector<std::pair<int, int> > bfsQ;
	std::vector<int> parent;

	//for every bfsQ push, we need a parent push
	bfsQ.push_back(std::pair<int, int>(hero->getX(), hero->getY()));
	parent.push_back(-1); //no parent for the root
	std::size_t idx = 0;

	const int dirX[] = { 1,0,-1,0 };
	const int dirY[] = { 0,1,0,-1 };
	int found = 0;
	while (idx != bfsQ.size()) {
		auto curPos = bfsQ[idx];
		for (int i = 0; i < 4; i++) {
			int newX = dirX[i] + curPos.first;
			int newY = dirY[i] + curPos.second;
			if (newX >= 0 && newX < 11 && newY >= 0 && newY < 11 && !vis[newX][newY]) {
				auto event = getEvent(newX, newY);
				if (!event || (newX == dx&&newY == dy)) { //can only walk on NULL, do not try to trigger on any event
					//this could give away 'hidden' events... so ok we could add an extra parameter or something I don't know
					//TODO add hidden field to bypass pathFind as a non-obstacle(evnt)
					bfsQ.push_back(std::pair<int, int>(newX, newY));
					parent.push_back(idx);
					vis[newX][newY] = 1;
					if (newX == dx&&newY == dy) {
						found = 1;
						break;
					}
				}
			}
		}
		if (found)
			break;
		idx++;
	}
	if (found) {
		auto prev = parent.back();  //this is the destination
		path.push_back(bfsQ.back());
		while (prev != -1) {
			path.push_back(bfsQ[prev]);
			prev = parent[prev];
		}
		path.pop_back(); //this is just the src loc
		// std::reverse(path.begin(), path.end());
		// path.push_back(std::pair<int, int>(dx, dy)); //push destination to the path.
	}
	return path;
}

void GameData::log(const std::string& message, bool inst)
{
	CCLOG("log message: %s", message.c_str());
	logLabel->setString(message);
	logLabel->setVisible(inst);
	if (inst) { //if there are async actions, you may not want to display the log right away
		CCLOG("log set to be instant");
		logLabel->setOpacity(0);
		logLabel->runAction(FadeIn::create(0.75));
	}
}

void GameData::showLog() {
	if (logLabel->isVisible()) {
		return;
	}
	logLabel->setVisible(true);
	if (logLabel->getString() != "") {
		logLabel->setOpacity(0);
		logLabel->runAction(FadeIn::create(0.75));
	}

}

//called after
//1. final hero movement
//2. used a HeroItem
// TODO if global event has animations (eg. blocking) then need new mechanism to block
void GameData::triggerGlobalEvents() {
	//CCLOG("trying to trigger global events");
	//check if there are global events on cur floor
	if (!GLOBALEVENT[floor->V()].empty()) {
		auto& gList = GLOBALEVENT[floor->V()];
		//go through the list and try to trigger it
		//remove from list on successful event trigger
		//this means, I will have saving problems...
		for (auto iter = gList.begin(); iter != gList.end();) {
			if ((*iter)->tryTrigger()) {
				iter = gList.erase(iter);
			}
			else {
				++iter;
			}
		}
	}
	//CCLOG("done trying global events");
}

void GameData::block() {
	blockCounter++;
	blocked = true;
}

void GameData::releaseBlock() {
	blockCounter--;
	if (blockCounter == 0) {
		blocked = false;
		hero->setMoving(false);
		continousMovement();
	}
	if (blockCounter < 0) {
		// CC_ASSERT(false);
		blockCounter = 0;
	}
		
}

bool GameData::isBlocked()
{
	return blocked;
}


GameData::~GameData() {
	CCLOG("game data cleaning up");

	hero = nullptr;
	floor = nullptr;
}
