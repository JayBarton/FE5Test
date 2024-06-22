#include "BattleManager.h"
#include "Unit.h"
#include "TextRenderer.h"
#include "Items.h"
#include "Cursor.h"
#include "EnemyManager.h"
#include "Globals.h"
#include "TextRenderer.h"
#include "Camera.h"
#include "SpriteRenderer.h"
#include <iostream>

#include "Cursor.h"

#include "TextAdvancer.h"
#include "InfoDisplays.h"
#include "InputManager.h"

void BattleManager::SetUp(Unit* attacker, Unit* defender, BattleStats attackerStats, 
	BattleStats defenderStats, int attackDistance, bool canDefenderAttack, Camera& camera, bool aiDelay /*= false*/, bool capturing /*= false*/)
{
	this->aiDelay = aiDelay;
	this->capturing = capturing;
	battleActive = true;
	this->attacker = attacker;
	this->defender = defender;
	this->canDefenderAttack = canDefenderAttack;
	if (capturing && defender->GetEquippedWeapon().type < 0)
	{
		//If defender is unarmed and we attempt to capture, it's an automatic success
		unitCaptured = true;
		deadUnit = defender;
		drawInfo = false;
	}
	else
	{
		drawInfo = true;
		this->attackDistance = attackDistance;
		this->attackerStats = attackerStats;
		this->defenderStats = defenderStats;
		attackerTurn = true;
		defenderTurn = false;
		checkDouble = true;
		actionTimer = 0.0f;
		battleQueue.clear();
		battleQueue.reserve(3);

		battleQueue.push_back(Attack{ 1, 0 });
		if (attacker->GetEquippedWeapon().consecutive)
		{
			battleQueue.push_back(Attack{ 1, 0 });
		}
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
					if (defender->GetEquippedWeapon().consecutive)
					{
						battleQueue.push_back(Attack{ 0, 0 });
					}
				}
				else
				{
					if (attacker->hasSkill(Unit::WRATH))
					{
						battleQueue[0].wrathAttack = true;
					}
					battleQueue.insert(battleQueue.begin(), defenderAttack);
					if (defender->GetEquippedWeapon().consecutive)
					{
						battleQueue.insert(battleQueue.begin(), Attack{ 0, 0 });
					}
				}
			}
			else
			{
				if (defender->hasSkill(Unit::WRATH))
				{
					defenderAttack.wrathAttack = true;
				}
				battleQueue.push_back(defenderAttack);
				if (defender->GetEquippedWeapon().consecutive)
				{
					battleQueue.push_back(Attack{ 0, 0 });
				}
			}
		}
		if (attackerStats.attackSpeed >= defenderStats.attackSpeed + 4)
		{
			battleQueue.push_back(Attack{ 1, 0 });
			if (attacker->GetEquippedWeapon().consecutive)
			{
				battleQueue.push_back(Attack{ 1, 0 });
			}
		}
		else if (defenderStats.attackSpeed >= attackerStats.attackSpeed + 4)
		{
			//Want a better check than this
			if (canDefenderAttack)
			{
				battleQueue.push_back(Attack{ 0, 0 });
				if (defender->GetEquippedWeapon().consecutive)
				{
					battleQueue.push_back(Attack{ 0, 0 });
				}
			}
		}
		accostQueue = battleQueue;
		for (int i = 0; i < accostQueue.size(); i++)
		{
			accostQueue[i].wrathAttack = false;
			accostQueue[i].vantageAttack = false;
		}

		camera.SetCenter(defender->sprite.getPosition());
		//U G H
		if (attacker->sprite.getPosition().y < defender->sprite.getPosition().y)
		{
			attacker->sprite.currentFrame = 15;
			attacker->sprite.startingFrame = 15;
			defender->sprite.currentFrame = 7;
			defender->sprite.startingFrame = 7;
		}
		else if (attacker->sprite.getPosition().y > defender->sprite.getPosition().y)
		{
			attacker->sprite.currentFrame = 7;
			attacker->sprite.startingFrame = 7;
			defender->sprite.currentFrame = 15;
			defender->sprite.startingFrame = 15;
		}
		else if (attacker->sprite.getPosition().x < defender->sprite.getPosition().x)
		{
			attacker->sprite.currentFrame = 11;
			attacker->sprite.startingFrame = 11;
			defender->sprite.currentFrame = 3;
			defender->sprite.startingFrame = 3;
		}
		else
		{
			attacker->sprite.currentFrame = 3;
			attacker->sprite.startingFrame = 3;
			defender->sprite.currentFrame = 11;
			defender->sprite.startingFrame = 11;
		}
		attacker->sprite.moveAnimate = true;
		defender->sprite.moveAnimate = true;
	}
}
 
void BattleManager::Update(float deltaTime, std::mt19937* gen, std::uniform_int_distribution<int>* distribution, InfoDisplays& displays, InputManager& inputManager)
{
	if (aiDelay)
	{
		delayTimer += deltaTime;
		if (delayTimer >= delay)
		{
			delayTimer = 0.0f;
			aiDelay = false;
		}
	}
	else if (battleActive)
	{
		if (unitDied)
		{
			if (displays.state == NONE)
			{
				if (deadUnit->Dying(deltaTime))
				{
					deadUnit->isDead = true;
					if (deadUnit->carriedUnit)
					{
						unitToDrop = deadUnit->carriedUnit;
						unitToDrop->sprite.SetPosition(deadUnit->sprite.getPosition());
					}
					auto deadPosition = deadUnit->sprite.getPosition();
					TileManager::tileManager.removeUnit(deadPosition.x, deadPosition.y);
					EndAttack();
					unitDiedSubject.notify(deadUnit);

					deadUnit = nullptr;
					unitDied = false;
				}
			}
		}
		else if (unitCaptured)
		{
			//capture animation. Needs to play after the experience display if the player captures. 
			// Not actually sure how this should work when capturing an enemy without a weapon, hard to test 
			if (deadUnit->carriedUnit)
			{
				unitToDrop = deadUnit->carriedUnit;
				unitToDrop->sprite.SetPosition(deadUnit->sprite.getPosition());
			}
			TileManager::tileManager.removeUnit(deadUnit->sprite.getPosition().x, deadUnit->sprite.getPosition().y);
			attacker->carryUnit(deadUnit);
			//In FE5 if a player unit is captured by the enemy, the enemy instantly takes their inventory.
			//This seems bogus to me but it's how it works there so it's how it works here.
			if (attacker->team != 0)
			{
				for (int i = 0; i < deadUnit->inventory.size(); i++)
				{
					if (attacker->inventory.size() <= 8)
					{
						attacker->inventory.push_back(deadUnit->inventory[i]);
						deadUnit->inventory.erase(deadUnit->inventory.begin());
						i--;
					}
				}
			}
			EndAttack();
			deadUnit = nullptr;
			unitCaptured = false;
		}
		else if (captureAnimation)
		{
			defender->movementComponent.Update(deltaTime, inputManager);
			if (!defender->movementComponent.moving)
			{
				defender->hide = true;
				displays.subject.notify(0);
				captureAnimation = false;
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
					if (capturing)
					{
						unitCaptured = true;
						drawInfo = false;
					}
					else
					{
						unitDied = true;
						drawInfo = false;
						if (deadUnit->deathMessage != "")
						{
							displays.PlayerUnitDied(deadUnit);
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
	int foeDefense = theseStats.attackType == 0 ? foe->getDefense() : foe->getMagic();
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
			if (roll <= foe->getLuck() * 3)
			{
				theseStats.hitAccuracy = 0;
			}
		}
	}
	DoBattleAction(thisUnit, foe, accuracy, crit, theseStats, attack, foeDefense, distribution, gen);
}

void BattleManager::DoBattleAction(Unit* thisUnit, Unit* otherUnit, int accuracy, int crit, BattleStats& theseStats, Attack& attack, int foeDefense, std::uniform_int_distribution<int>* distribution, std::mt19937* gen)
{
	auto roll = (*distribution)(*gen);
	std::cout << "roll " << roll << std::endl;
	//Do roll to determine if hit
	if (roll <= 100)
	{
		int dealtDamage = theseStats.attackDamage;
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
		std::cout << thisUnit->name << " Attacks\n";

		//Need to figure this out
		if (otherUnit->currentHP <= 0)
		{
			otherUnit->currentHP = 0;
			deadUnit = otherUnit;
			battleQueue.clear();
		}
		else
		{
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
	}
	else
	{
		//Ranged attacks and tomes should eat a use even if they miss
		//Not the most elegant or extensible solution to this problem, but it works for now
		if (attackDistance > 1 || thisUnit->GetEquippedWeapon().isTome)
		{
			thisUnit->GetEquippedItem()->remainingUses--;
		}
		std::cout << thisUnit->name << " Misses\n";
	}
}

void BattleManager::EndAttack()
{
	if (capturing && !attacker->carriedUnit)
	{
		attacker->carryingMalus = 1;
	}
	subject.notify(attacker, defender, capturing);
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
	DropHeldUnit();
	battleActive = false;
	camera.SetCenter(attacker->sprite.getPosition());
}

void BattleManager::DropHeldUnit()
{
	if (unitToDrop)
	{
		auto deadPosition = unitToDrop->sprite.getPosition();
		unitToDrop->placeUnit(deadPosition.x, deadPosition.y);
		unitToDrop->hide = false;
		unitToDrop->carryingUnit = nullptr;
		unitToDrop = nullptr;
	}
}

void BattleManager::CaptureUnit()
{
	captureAnimation = true;
	//This is being called twice right now because I can't figure out how to get the timing right
	DropHeldUnit();
}

void BattleManager::Draw(TextRenderer* text, Camera& camera, SpriteRenderer* Renderer, Cursor* cursor)
{
	if (aiDelay)
	{
		Renderer->setUVs(cursor->uvs[1]);
		Texture2D displayTexture = ResourceManager::GetTexture("cursor");

		Renderer->DrawSprite(displayTexture, defender->sprite.getPosition(), 0.0f, cursor->dimensions, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	}
	else if (drawInfo)
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
