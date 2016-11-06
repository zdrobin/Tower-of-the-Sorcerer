#include "Fightable.h"
#include "GameData.h"
USING_NS_CC;

Fightable::Fightable(int id,std::string desc,int hp,int atk,int def):MyEvent(id,desc),hp(hp),atk(atk),def(def){}


int Fightable::hitBy(Fightable* hitter){
	if (hitter->atk-def>0)
		return hitter->atk-def;
	return 0;
}

int Fightable::hit(Fightable* target){
	return target->hitBy(this);
}

void Fightable::decHp(int amt)
{
	hp-=amt;
	if (hp<=0)
		hp=0;
}


//fight until the enemy is dead
//this method is implemented for Fight only because,
//IIRC, Zeno will engage the character (or some other Mob will)
//accept callback so that I'll return hp after each hit to the caller
int Fightable::fight(Fightable* target,std::function<void(Fightable&)> hpCallback1,std::function<void(Fightable&)> hpCallback2){
	int totalDamageTaken=0;
	if (hit(target)==0)
		return -1;
	while (1){
		int dmg=hit(target);
		target->decHp(dmg);
		if (hpCallback1!=nullptr)
			hpCallback1(*target);
		if (target->hp<=0)
			break;
		dmg=hitBy(target);
		decHp(dmg);
		if (hpCallback2!=nullptr)
			hpCallback2(*this);
		totalDamageTaken+=dmg;
	}
	return totalDamageTaken;
}

const int Fightable::getHp(){
	return hp;
}
const int Fightable::getAtk(){
	return atk;
}
const int Fightable::getDef(){
	return def;
}

//I don't like this... override it in enemy?
bool Fightable::triggerEvent(){
	return true;
}

//think about extensions later... I don't care right now
bool Fightable::stepOnEvent(){
	GameData::getInstance()->hero.fight(this,
		[](Fightable &f) { log("hp %d",f.getHp()); },
		[](Fightable &f) { log("hp %d",f.getHp()); });
	//GameData::getInstance()->killEvent(std::pair<int,int>(x,y));
	return true;
}

Fightable::~Fightable(){
}
