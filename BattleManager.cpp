#include "BattleManager.h"
#include "Unit.h"
#include "TextRenderer.h"
#include "Items.h"
#include "Cursor.h"
#include "EnemyManager.h"
#include "Globals.h"
#include "TextRenderer.h"
#include "Camera.h"
#include <iostream>

void BattleManager::SetUp(Unit* attacker, Unit* defender, BattleStats attackerStats, BattleStats defenderStats, bool canDefenderAttack, Camera& camera)
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
	battleQueue.clear();
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
	camera.SetCenter(defender->sprite.getPosition());
}
 
void BattleManager::Update(float deltaTime, std::mt19937* gen, std::uniform_int_distribution<int>* distribution)
{
	if (battleActive)
	{
		if (unitDied)
		{
			if (deadUnit->Dying(deltaTime))
			{
				deadUnit->isDead = true;
				TileManager::tileManager.removeUnit(deadUnit->sprite.getPosition().x, deadUnit->sprite.getPosition().y);
				deadUnit = nullptr;
				unitDied = false;
				EndAttack();
			}
		}
		else
		{
			actionTimer += deltaTime;
			if (actionTimer >= actionDelay)
			{
				actionTimer = 0;

				if (battleQueue.size() > 0)
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
				else if (deadUnit)
				{
					unitDied = true;
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
						//No one has accost, we're done
						else
						{
							EndAttack();
						}
					}
					//We are in an accost round, and it should only fire once(I think)
					else
					{
						EndAttack();
					}
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
	int foeDefense = theseStats.attackType == 0 ? foe->defense : foe->magic;
	if (attack.wrathAttack)
	{
		std::cout << thisUnit->name << " activates wrath\n";
		crit = 100;
		accuracy = 100;
	}
	else if (foe->hasSkill(Unit::PRAYER))
	{
		int dealtDamage = theseStats.attackDamage;
		dealtDamage -= foeDefense;
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
	DoBattleAction(thisUnit, foe, accuracy, crit, theseStats.attackDamage, foeDefense, distribution, gen);
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

void BattleManager::DoBattleAction(Unit* thisUnit, Unit* otherUnit, int accuracy, int crit, int firstDamage, int foeDefense, std::uniform_int_distribution<int>* distribution, std::mt19937* gen)
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
		dealtDamage -= foeDefense;
		int remainingHealth = otherUnit->currentHP - dealtDamage;
		thisUnit->GetEquippedItem()->remainingUses--;
		otherUnit->currentHP = remainingHealth;
		//Need to figure this out
		if (otherUnit->currentHP <= 0)
		{
			otherUnit->currentHP = 0;
			deadUnit = otherUnit;
			battleQueue.clear();
			//EndAttack();
		}
		std::cout << thisUnit->name << " Attacks\n";
	}
	else
	{
		std::cout << thisUnit->name << " Misses\n";
	}
}

void BattleManager::EndAttack()
{
	battleActive = false;
	subject.notify(attacker, defender);
}

void BattleManager::EndBattle(Cursor* cursor, EnemyManager* enemyManager, Camera& camera)
{
	if (attacker->team == 0)
	{
		if (!attacker->isDead && attacker->isMounted() && attacker->mount->remainingMoves > 0)
		{
			//If the player attacked we need to return control to the cursor
			cursor->GetRemainingMove();
		}
		else
		{
			cursor->FinishMove();
			cursor->focusedUnit = nullptr;
		}
	}
	//Not crazy about any of this
	else
	{
		if (!attacker->isDead && attacker->isMounted() && attacker->mount->remainingMoves > 0)
		{
			enemyManager->CantoMove();
		}
		else
		{
			enemyManager->FinishMove();
		}
	}
	camera.SetCenter(attacker->sprite.getPosition());
}

void BattleManager::Draw(TextRenderer* text, Camera& camera)
{
	if (!unitDied)
	{
		int yOffset = 150;
		//The hp should be drawn based on which side each unit is. So if the attacker is to the left of the defender, the hp should be on the left, and vice versa
		glm::vec2 attackerDraw;
		glm::vec2 defenderDraw;
		if (attacker->sprite.getPosition().x < defender->sprite.getPosition().x)
		{
			attackerDraw = glm::vec2(200, yOffset);
			defenderDraw = glm::vec2(500, yOffset);
		}
		else
		{
			defenderDraw = glm::vec2(200, yOffset);
			attackerDraw = glm::vec2(500, yOffset);
		}
		glm::vec2 drawPosition = glm::vec2(attacker->sprite.getPosition()) - glm::vec2(8.0f, yOffset);
		drawPosition = camera.worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
		text->RenderText(attacker->name, attackerDraw.x, attackerDraw.y, 1, glm::vec3(0.0f));
		attackerDraw.y += 22.0f;
		text->RenderText("HP", attackerDraw.x, attackerDraw.y, 1, glm::vec3(0.1f, 0.11f, 0.22f));
		attackerDraw.x += 25;
		text->RenderText(intToString(attacker->currentHP) + "/" + intToString(attacker->maxHP), attackerDraw.x, attackerDraw.y, 1, glm::vec3(0.0f));

		drawPosition = glm::vec2(defender->sprite.getPosition()) - glm::vec2(8.0f, yOffset);
		drawPosition = camera.worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
		text->RenderText(defender->name, defenderDraw.x, defenderDraw.y, 1, glm::vec3(0.0f));
		defenderDraw.y += 22.0f;
		text->RenderText("HP", defenderDraw.x, defenderDraw.y, 1, glm::vec3(0.1f, 0.11f, 0.22f));
		defenderDraw.x += 25;
		text->RenderText(intToString(defender->currentHP) + "/" + intToString(defender->maxHP), defenderDraw.x, defenderDraw.y, 1, glm::vec3(0.0f));
	}
}
