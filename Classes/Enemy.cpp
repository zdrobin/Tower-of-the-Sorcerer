#include "Enemy.h"
#include "TransformCoordinate.h"
USING_NS_CC;

Enemy::Enemy(int id,std::string desc,int secondImageID,int hp,int atk,int def):Fightable(id,desc,hp,atk,def),secondImageID(secondImageID){}


bool Enemy::canAtk(){
	return true;
}

Sprite* Enemy::getSprite(){
	std::stringstream ss2;
	ss2<<"tile ("<<id<<").png";
	auto sprite2=Sprite::create(ss2.str());
	std::pair<int,int> pxy=TransformCoordinate::transform(x,y);
	sprite2->setPosition(pxy.first,pxy.second);
	sprite2->setAnchorPoint(Vec2(0,0));
	std::stringstream ss3;
	ss3<<"tile ("<<secondImageID<<").png";
	//add animation
	Vector<SpriteFrame*> animFrames;
	animFrames.reserve(2);
	animFrames.pushBack(SpriteFrame::create(ss3.str(),Rect(0,0,40/Director::getInstance()->getContentScaleFactor(),40/Director::getInstance()->getContentScaleFactor())));
	animFrames.pushBack(SpriteFrame::create(ss2.str(),Rect(0,0,40/Director::getInstance()->getContentScaleFactor(),40/Director::getInstance()->getContentScaleFactor())));
	Animation* animation = Animation::createWithSpriteFrames(animFrames,0.3f);
	Animate* animate = Animate::create(animation);
	sprite2->setScale(Director::getInstance()->getContentScaleFactor());
	sprite2->runAction(RepeatForever::create(animate));
	sprite=sprite2;
	return sprite2;
}


Enemy::~Enemy(){
}

Enemy * Enemy::clone()
{
	return new Enemy(*this);
}
