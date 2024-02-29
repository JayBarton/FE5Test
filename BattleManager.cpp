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

	this->attackerStats = attackerStats;
	this->defenderStats = defenderStats;
	attackerTurn = true;
	defenderTurn = false;
	checkDouble = true;
	actionTimer = 0.0f;
	battleQueue.reserve(3);

	battleQueue.push_back(Attack{1, 0});
	if (canDefenderAttack)
	{
		Attack defenderAttack{ 0 };

		if (defender->hasSkill(Unit::VANTAGE))
		{
			defenderAttack.vantageAttack = true;
			if (attacker->hasSkill(Unit::VANTAGE))
			{
				battleQueue[0].vantageAttack = true;
				if (defender->hasSkill(Unit::WRATH))
				{
					defenderAttack.wrathAttack = true;
				}
				battleQueue.push_back(defenderAttack);
			}
			else
			{
				if (attacker->hasSkill(Unit::WRATH))
				{
					battleQueue[0].wrathAttack = true;
				}
				battleQueue.insert(battleQueue.begin(), defenderAttack);
			}
		}
		else
		{
			if (defender->hasSkill(Unit::WRATH))
			{
				defenderAttack.wrathAttack = true;
			}
			battleQueue.push_back(defenderAttack);
		}
	}
	if (attackerStats.attackSpeed >= defenderStats.attackSpeed + 4)
	{
		battleQueue.push_back(Attack{ 1, 0 });
	}
	else if (defenderStats.attackSpeed >= attackerStats.attackSpeed + 4)
	{
		//Want a better check than this
		if (canDefenderAttack)
		{
			battleQueue.push_back(Attack{ 0, 0 });
		}
	}
	accostQueue = battleQueue;
	for (int i = 0; i < accostQueue.size(); i++)
	{
		accostQueue[i].wrathAttack = false;
		accostQueue[i].vantageAttack = false;
	}
}
 
void BattleManager::Update(float deltaTime, std::mt19937* gen, std::uniform_int_distribution<int>* distribution)
{
	if (battleActive)
	{
		actionTimer += deltaTime;
		if (actionTimer >= actionDelay)
		{
			actionTimer = 0;

			if(battleQueue.size() > 0)
			{
				auto attack = battleQueue[0];
				battleQueue.erase(battleQueue.begin());
				if (attack.firstAttacker)
				{
					PreBattleChecks(attacker, attackerStats, defender, attack, distribution, gen);
				}
				else
				{
					PreBattleChecks(defender, defenderStats, attacker, attack, distribution, gen);
				}
			}
			else
			{
				//if either unit has accost and accost has not fired
				//reset battle queue
				if (!accostFired)
				{
					if (attacker->hasSkill(Unit::ACCOST))
					{
						if (attacker->currentHP > defender->currentHP && attackerStats.attackSpeed > defenderStats.attackSpeed)
						{
							battleQueue = accostQueue;
							accostFired = true;
							std::cout << attacker->name << " Accosts\n";

						}
					}
					else if (defender->hasSkill(Unit::ACCOST))
					{
						if (defender->currentHP > attacker->currentHP && defenderStats.attackSpeed > attackerStats.attackSpeed)
						{
							battleQueue = accostQueue;
							accostFired = true;
							std::cout << defender->name << " Accosts\n";
						}
					}
					else
					{
						battleActive = false;
						attacker->hasMoved = true;
					}
				}
				else
				{
					battleActive = false;
					attacker->hasMoved = true;
				}
			}
		}
	}
}

void BattleManager::PreBattleChecks(Unit* thisUnit, BattleStats& theseStats, Unit* foe, Attack& attack, std::uniform_int_distribution<int>* distribution, std::mt19937* gen)
{
	if (attack.vantageAttack)
	{
		//Check if the next attack in the queue is also vantage
		//Sort of clumsy
		//I want some sort of notification to say both units activated Vantage, and the first cancelled the second
		if (battleQueue[0].vantageAttack)
		{
			battleQueue[0].vantageAttack = false;
			std::cout << foe->name << " activates vantage\n";
		}
		std::cout << thisUnit->name << " activates vantage\n";
	}
	auto crit = theseStats.hitCrit;
	auto accuracy = theseStats.hitAccuracy;
	if (attack.wrathAttack)
	{
		std::cout << thisUnit->name << " activates wrath\n";
		crit = 100;
		accuracy = 100;
	}
	else if (foe->hasSkill(Unit::PRAYER))
	{
		int dealtDamage = theseStats.attackDamage;
		if (!thisUnit->GetWeaponData(thisUnit->GetEquippedItem()).isMagic)
		{
			dealtDamage -= foe->defense;
		}
		else
		{
			dealtDamage -= foe->magic;
		}
		int remainingHealth = foe->currentHP - dealtDamage;
		if (remainingHealth <= 0 && theseStats.hitAccuracy > 0)
		{
			//prayer roll
			auto roll = (*distribution)(*gen);
			std::cout << "Prayer roll" << roll << std::endl;
			if (roll <= foe->luck * 3)
			{
				theseStats.hitAccuracy = 0;
			}
		}
	}
	DoBattleAction(thisUnit, foe, accuracy, crit, theseStats.attackDamage, distribution, gen);
	if (thisUnit->hasSkill(Unit::CONTINUE) && !attack.continuedAttack)
	{
		auto roll = (*distribution)(*gen);
		std::cout << "continue roll " << roll << std::endl;
		if (roll <= theseStats.attackSpeed * 2)
		{
			std::cout << thisUnit->name << " continues " << std::endl;

			battleQueue.insert(battleQueue.begin(), Attack{ attack.firstAttacker, 1 });
		}
	}
}

void BattleManager::DoBattleAction(Unit* thisUnit, Unit* otherUnit, int accuracy, int crit, int firstDamage, std::uniform_int_distribution<int>* distribution, std::mt19937* gen)
{
	auto roll = (*distribution)(*gen);
	std::cout << "roll " << roll << std::endl;
	//Do roll to determine if hit
	if (roll <= accuracy)
	{
		int dealtDamage = firstDamage;
		if (crit > 0)
		{
			//if the hit lands, do another roll to determine if it is a critical hit. 
			//I don't know this for a fact, but I am assuming if crit rate is zero or 100, we don't bother
			if (crit == 100)
			{
				std::cout << "CRITICAL" << std::endl;
				dealtDamage *= 2;
			}
			else
			{
				roll = (*distribution)(*gen);
				std::cout << "crit roll " << roll << std::endl;
				if (roll <= crit)
				{
					std::cout << "CRITICAL" << std::endl;
					dealtDamage *= 2;
				}
			}
		}
		if (!thisUnit->GetWeaponData(thisUnit->GetEquippedItem()).isMagic)
		{
			dealtDamage -= otherUnit->defense;
		}
		else
		{
			dealtDamage -= otherUnit->magic;
		}
		int remainingHealth = otherUnit->currentHP - dealtDamage;
		thisUnit->GetEquippedItem()->remainingUses--;
		otherUnit->currentHP = remainingHealth;
		if (otherUnit->currentHP <= 0)
		{
			battleActive = false;
			attacker->hasMoved = true;
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
