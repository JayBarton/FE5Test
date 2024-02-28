#include "EnemyManager.h"
#include "SpriteRenderer.h"
#include "Items.h"

void EnemyManager::GetPriority()
{
	auto enemy = enemies[currentEnemy];
	auto weapon = enemy->GetWeaponData(enemy->GetEquippedItem());
	auto path = enemy->FindUnitMoveRange();
	auto otherUnits = enemy->inRangeUnits(weapon.minRange, weapon.maxRange, 0); //totally wrong
	std::vector<Target> targets;
	targets.resize(otherUnits.size());

	/*
	* 	float attackDistance = abs(enemy->sprite.getPosition().x - unit->sprite.getPosition().x) + abs(enemy->sprite.getPosition().y - unit->sprite.getPosition().y);
	attackDistance /= TileManager::TILE_SIZE;
	auto enemyWeapon = enemy->GetWeaponData(enemy->GetEquippedItem());
	if (enemyWeapon.maxRange >= attackDistance && enemyWeapon.minRange <= attackDistance)
	{
		enemyCanCounter = true;
	}
	*/

for (int i = 0; i < otherUnits.size(); i++)
	{
		auto otherUnit = otherUnits[i];
	//	if(otherUnit)
	}
}

void EnemyManager::Draw(SpriteRenderer* renderer)
{
	for (int i = 0; i < enemies.size(); i++)
	{
		enemies[i]->Draw(renderer);
	}
}
