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
		if (defender->hasSkill(Unit::WRATH))
		{
			defenderAttack.wrathAttack = true;
		}
		if (defender->hasSkill(Unit::VANTAGE))
		{
			defenderAttack.vantageAttack = true;
			battleQueue.insert(battleQueue.begin(), defenderAttack);
		}
		else
		{
			battleQueue.push_back(defenderAttack);
		}
	}
	if (attackerStats.attackSpeed >= defenderStats.attackSpeed + 4)
	{
		battleQueue.push_back(Attack{ 1, 0 });
	}
	else if (defenderStats.attackSpeed >= attackerStats.attackSpeed + 4)
	{
		battleQueue.push_back(Attack{ 0, 0 });
	}
	accostQueue = battleQueue;
}

//Wrath is still wrong. According to https://fireemblemwiki.org/wiki/Wrath
//Will not activate if the user has Vantage and the foe does not, as it would cause the user to strike first. 
//If the foe has Vantage and the user does not, the user's Wrath activates even if the user initiates combat on the foe, as the foe strikes first. 
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
					if (attack.vantageAttack)
					{
						std::cout << attacker->name << " activates vantage\n";
					}
					if (defender->hasSkill(Unit::PRAYER))
					{
						int dealtDamage = attackerStats.attackDamage;
						if (!attacker->GetWeaponData(attacker->GetEquippedItem()).isMagic)
						{
							dealtDamage -= defender->defense;
						}
						else
						{
							dealtDamage -= defender->magic;
						}
						int remainingHealth = defender->currentHP - dealtDamage;
						if (remainingHealth <= 0 && attackerStats.hitAccuracy > 0)
						{
							//prayer roll
							auto roll = (*distribution)(*gen);
							std::cout << "Prayer roll" << roll << std::endl;
							if (roll <= defender->luck * 3)
							{
								attackerStats.hitAccuracy = 0;
							}
						}
					}
					DoBattleAction(attacker, defender, attackerStats.hitAccuracy, attackerStats.hitCrit, attackerStats.attackDamage, distribution, gen);
					if (attacker->hasSkill(Unit::CONTINUE) && !attack.continuedAttack)
					{
						auto roll = (*distribution)(*gen);
						std::cout << "continue roll " << roll << std::endl;
						if (roll <= attackerStats.attackSpeed * 2)
						{
							std::cout << attacker->name << " continues " << std::endl;

							battleQueue.insert(battleQueue.begin(), Attack{ 1, 1 });
						}
					}
				}
				else
				{
					if (attack.vantageAttack)
					{
						std::cout << defender->name << " activates vantage\n";
					}
					auto crit = defenderStats.hitCrit;
					auto accuracy = defenderStats.hitAccuracy;
					if (attack.wrathAttack)
					{
						std::cout << defender->name << " activates wrath\n";
						crit = 100;
						accuracy = 100;
					}
					else
					{
						if (attacker->hasSkill(Unit::PRAYER))
						{
							int dealtDamage = attackerStats.attackDamage;
							if (!defender->GetWeaponData(defender->GetEquippedItem()).isMagic)
							{
								dealtDamage -= attacker->defense;
							}
							else
							{
								dealtDamage -= attacker->magic;
							}
							int remainingHealth = attacker->currentHP - dealtDamage;
							if (remainingHealth <= 0 && defenderStats.hitAccuracy > 0)
							{
								//prayer roll
								auto roll = (*distribution)(*gen);
								std::cout << "Prayer roll" << roll << std::endl;
								if (roll <= attacker->luck * 3)
								{
									defenderStats.hitAccuracy = 0;
									accuracy = 0;
								}
							}
						}
					}
					DoBattleAction(defender, attacker, accuracy, crit, defenderStats.attackDamage, distribution, gen);
					if (defender->hasSkill(Unit::CONTINUE) && !attack.continuedAttack)
					{
						auto roll = (*distribution)(*gen);
						std::cout << "continue roll " << roll << std::endl;
						if (roll <= defenderStats.attackSpeed * 2)
						{
							std::cout << defender->name << " continues " << std::endl;

							battleQueue.insert(battleQueue.begin(), Attack{ 1, 1 });
						}
					}
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

					}
				}
				else
				{
					battleActive = false;
				}
			}
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
