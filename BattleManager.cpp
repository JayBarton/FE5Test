#include "BattleManager.h"
#include "Unit.h"
#include "TextRenderer.h"
#include "Items.h"
#include <iostream>

void BattleManager::SetUp(Unit* attacker, Unit* defender, BattleStats attackerStats, BattleStats defenderStats, bool canDefenderAttack)
{
	battleActive = true;
	this->attacker = attacker;
	this->defender = defender;
	this->canDefenderAttack = canDefenderAttack;

	if (!attacker->GetWeaponData(attacker->inventory[attacker->equippedWeapon]).isTome)
	{
		attackerDamage = attackerStats.attackDamage - defender->defense;
	}
	else
	{
		attackerDamage = attackerStats.attackDamage - defender->magic;
	}
	if (!defender->GetWeaponData(defender->inventory[defender->equippedWeapon]).isTome)
	{
		defenderDamage = defenderStats.attackDamage - attacker->defense;
	}
	else
	{
		defenderDamage = defenderStats.attackDamage - attacker->magic;
	}
	this->attackerStats = attackerStats;
	this->defenderStats = defenderStats;
	attackerTurn = true;
	defenderTurn = false;
	checkDouble = true;
	actionTimer = 0.0f;
}

void BattleManager::Update(float deltaTime, std::mt19937* gen, std::uniform_int_distribution<int>* distribution)
{
	if (battleActive)
	{
		actionTimer += deltaTime;
		if (actionTimer >= actionDelay)
		{
			actionTimer = 0;

			if (attackerTurn)
			{
				DoBattleAction(attacker, defender, attackerStats.hitAccuracy, attackerStats.hitCrit, attackerDamage, distribution, gen);
				attackerTurn = false;
				defenderTurn = true;
				if (!canDefenderAttack)
				{
					defenderTurn = false;
				}
			}
			else if(defenderTurn)
			{
				DoBattleAction(defender, attacker, defenderStats.hitAccuracy, defenderStats.hitCrit, defenderDamage, distribution, gen);
				defenderTurn = false;
			}
			else if (checkDouble)
			{
				checkDouble = false;
				if (attackerStats.attackSpeed >= defenderStats.attackSpeed + 4)
				{
					std::cout << attacker->name << " attacks again\n";
					DoBattleAction(attacker, defender, attackerStats.hitAccuracy, attackerStats.hitCrit, attackerDamage, distribution, gen);
				}
				else if (defenderStats.attackSpeed >= attackerStats.attackSpeed + 4)
				{
					std::cout << defender->name << " attacks again\n";
					DoBattleAction(defender, attacker, defenderStats.hitAccuracy, defenderStats.hitCrit, defenderDamage, distribution, gen);
				}
				else
				{
					battleActive = false;
				}
			}
			else
			{
				battleActive = false;
			}
		}
	}
}

void BattleManager::DoBattleAction(Unit* thisUnit, Unit* otherUnit, int accuracy, int crit, int damage, std::uniform_int_distribution<int>* distribution, std::mt19937* gen)
{
	auto roll = (*distribution)(*gen);
	std::cout << "roll " << roll << std::endl;
	//Do roll to determine if hit
	if (roll <= accuracy)
	{
		thisUnit->inventory[thisUnit->equippedWeapon]->remainingUses--;
		//if the hit lands, do another roll to determine if it is a critical hit. I don't know this for a fact, but I am assuming if crit rate is zero, we don't bother
		otherUnit->currentHP -= damage;
		if (otherUnit->currentHP <= 0)
		{
			battleActive = false;
		}
		std::cout << thisUnit->name << " Attacks\n";
	}
	else
	{
		std::cout << thisUnit->name << " Misses\n";
	}
}

void BattleManager::Draw(TextRenderer* text)
{

}
