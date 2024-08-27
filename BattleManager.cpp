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

#include "SBatch.h"
#include "Settings.h"

#include "TextAdvancer.h"

void BattleManager::SetUp(Unit* attacker, Unit* defender, BattleStats attackerStats, 
	BattleStats defenderStats, int attackDistance, bool canDefenderAttack, Camera& camera, bool aiDelay /*= false*/, bool capturing /*= false*/)
{
	//check if battle scene should be playing, if so set to true
	if (Settings::settings.mapAnimations == 0 || Settings::settings.mapAnimations == 2 && (attacker->battleAnimations || defender->battleAnimations))
	{
		battleScene = true;
	}
	else
	{
		battleScene = false;
	}
	this->aiDelay = aiDelay;
	this->capturing = capturing;
	battleActive = true;
	this->attacker = attacker;
	this->defender = defender;
	this->canDefenderAttack = canDefenderAttack;
	drawInfo = false;

	if (capturing && defender->GetEquippedWeapon().type < 0)
	{
		//If defender is unarmed and we attempt to capture, it's an automatic success
		unitCaptured = true;
		deadUnit = defender;
		drawInfo = false;
		battleScene = false;
	}
	else
	{
		this->attackDistance = attackDistance;
		this->attackerStats = attackerStats;
		this->defenderStats = defenderStats;
		attackerDisplayHealth = attacker->currentHP;
		defenderDisplayHealth = defender->currentHP;
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

		talkingUnit = nullptr;
		sceneUnitID = -1;

		if (defender->battleMessage != "")
		{
			talkingUnit = defender;
		}
		else if (attacker->battleMessage != "")
		{
			talkingUnit = attacker;
		}

		if (battleScene)
		{	
			transitionX = -286.0f;
			ResourceManager::GetShader("sprite").Use().SetVector2f("cameraPosition", (camera.position - glm::vec2(camera.halfWidth, camera.halfHeight)));
			ResourceManager::GetShader("sprite").SetFloat("maskX", transitionX);
			ResourceManager::GetShader("sprite").SetInteger("battleScreen", 1);
			transitionIn = true;
			fadeAlpha = 0.2f;
			GetFacing();

			std::string hit;
			std::string dmg;
			if (canDefenderAttack)
			{
				hit = intToString(defenderStats.hitAccuracy);
				dmg = intToString(defenderStats.attackDamage);
			}
			else
			{
				hit = "--";
				dmg = "--";
			}
			if (!aiDelay)
			{
				ResourceManager::PlaySound("battleTransition");
				ResourceManager::FadeOutPause(500);
				rightDisplayHealth = &attackerDisplayHealth;
				leftDisplayHealth = &defenderDisplayHealth;
				leftMaxHealth = defender->maxHP;
				rightMaxHealth = attacker->maxHP;

				rightStats = { intToString(attackerStats.hitAccuracy), intToString(attackerStats.attackDamage), attacker->getDefense(), attacker->level };

				leftStats = { hit, dmg, defender->getDefense(), defender->level };
			}
			else
			{
				rightDisplayHealth = &defenderDisplayHealth;
				leftDisplayHealth = &attackerDisplayHealth;
				rightMaxHealth = defender->maxHP;
				leftMaxHealth = attacker->maxHP;
				rightStats = { hit, dmg, defender->getDefense(), defender->level };
				leftStats = { intToString(attackerStats.hitAccuracy), intToString(attackerStats.attackDamage), attacker->getDefense(), attacker->level };
			}
		}
		else
		{
			camera.SetCenter(defender->sprite.getPosition());
			if (!aiDelay)
			{
				drawInfo = true;
				if (talkingUnit)
				{
					displays->UnitBattleMessage(talkingUnit, false, false);
				}
			}

			GetFacing();
			attacker->sprite.moveAnimate = true;
			defender->sprite.moveAnimate = true;
			fadeBoxIn = true;
		}
	}
}

void BattleManager::GetFacing()
{
	if (attacker->sprite.getPosition().y < defender->sprite.getPosition().y)
	{
		attacker->sprite.facing = 3;
		attacker->sprite.currentFrame = 15;
		attacker->sprite.startingFrame = 15;
		defender->sprite.facing = 1;
		defender->sprite.currentFrame = 7;
		defender->sprite.startingFrame = 7;
	}
	else if (attacker->sprite.getPosition().y > defender->sprite.getPosition().y)
	{
		attacker->sprite.facing = 1;
		attacker->sprite.currentFrame = 7;
		attacker->sprite.startingFrame = 7;
		defender->sprite.facing = 3;
		defender->sprite.currentFrame = 15;
		defender->sprite.startingFrame = 15;
	}
	else if (attacker->sprite.getPosition().x < defender->sprite.getPosition().x)
	{
		attacker->sprite.facing = 2;
		attacker->sprite.currentFrame = 11;
		attacker->sprite.startingFrame = 11;
		defender->sprite.facing = 0;
		defender->sprite.currentFrame = 3;
		defender->sprite.startingFrame = 3;
	}
	else
	{
		attacker->sprite.facing = 0;
		attacker->sprite.currentFrame = 3;
		attacker->sprite.startingFrame = 3;
		defender->sprite.facing = 2;
		defender->sprite.currentFrame = 11;
		defender->sprite.startingFrame = 11;
	}
}
 
void BattleManager::Update(float deltaTime, std::mt19937* gen, std::uniform_int_distribution<int>* distribution, InputManager& inputManager)
{
	if (aiDelay)
	{
		delayTimer += deltaTime;
		if (delayTimer >= delay)
		{
			delayTimer = 0.0f;
			aiDelay = false;
			if (battleScene)
			{
				ResourceManager::PlaySound("battleTransition");
				ResourceManager::FadeOutPause(500);
			}
			else
			{
				if (talkingUnit)
				{
					displays->UnitBattleMessage(talkingUnit, false, false);
				}
				drawInfo = true;
			}
		}
	}
	else if (battleActive)
	{
		if (missedText.active)
		{
			missedText.position.y -= 48.0f * deltaTime;
			missedText.scale += 1.5f * deltaTime;
			delayTimer += deltaTime;
			if (delayTimer >= 0.25f)
			{
				delayTimer = 0.0f;
				missedText.active = false;
			}
		}
		if (displayHealth && targetHealth < *displayHealth)
		{
			(*displayHealth)--;
			if (*displayHealth == targetHealth)
			{
				displayHealth = nullptr;
			}
		}
		if (battleScene)
		{
			if (transitionIn)
			{
				fadeTimer += deltaTime;
				transitionX += deltaTime * 286;
				fadeAlpha += deltaTime * 0.8f;
				if (fadeTimer >= fadeOutDelay)
				{
					fadeAlpha = 1.0f;
					transitionX = 0;
					fadeTimer = 0;
					transitionIn = false;
					ResourceManager::GetShader("sprite").SetInteger("battleScreen", 0, true);

					leftPosition = glm::vec2(56, 84);
					rightPosition = glm::vec2(200, 84);
					if (attacker->team == 0)
					{
						leftUnit = defender;
						rightUnit = attacker;
					}
					else
					{
						leftUnit = attacker;
						rightUnit = defender;
					}
					leftUnit->sprite.facing = 2;
					leftUnit->sprite.currentFrame = 11;
					leftUnit->sprite.startingFrame = 11;
					rightUnit->sprite.facing = 0;
					rightUnit->sprite.currentFrame = 3;
					rightUnit->sprite.startingFrame = 3;
					attacker->sprite.moveAnimate = true;
					defender->sprite.moveAnimate = true;

					fadeInBattle = true;
					if (attacker->team == 0)
					{
						ResourceManager::PlayMusic("PlayerAttackStart", "PlayerAttackLoop");
					}
					else
					{
						ResourceManager::PlayMusic("EnemyAttack");
					}
				}
			}
			else if (fadeInBattle)
			{
				fadeTimer += deltaTime;
				fadeAlpha -= deltaTime;
				if (fadeTimer >= fadeOutDelay)
				{
					fadeTimer = 0;
					fadeAlpha = 0;
					fadeInBattle = false;
					if (talkingUnit)
					{
						displays->UnitBattleMessage(talkingUnit, true, true);
					}
				}
			}
			else if (fadeOutBattle)
			{
				fadeTimer += deltaTime;
				fadeAlpha += deltaTime;
				if (fadeTimer >= fadeOutDelay)
				{
					fadeTimer = 0;
					fadeAlpha = 1;
					fadeOutBattle = false;
					battleScene = false;
					fadeBackMap = true;
					if (unitCaptured)
					{
						//The things we do for love. Want to fade out the enemy on capture, but still play the capture animation, so they need to be visible again
						deadUnit->sprite.alpha = 1;
					}
					GetFacing();
					displays->ClearLevelUpDisplay();
				}
			}
			else if (unitDied || unitCaptured)
			{
				if (displays->state == NONE)
				{
					if (deadUnit->Dying(deltaTime))
					{
						if (unitDied)
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

							unitDied = false;
						}
						else
						{
							EndAttack();

						}
					}
				}
			}
			else
			{
				if (actingUnit)
				{
					auto newPosition = *actingPosition + movementDirection * 2.5f;
					*actingPosition = newPosition;
					auto distance = glm::length(*actingPosition - startPosition);
					if (distance > 144)
					{
						*actingPosition = startPosition + movementDirection * 144.0f;
						if (!moveBack)
						{
							auto attack = battleQueue[0];
							battleQueue.erase(battleQueue.begin());
							Unit* otherUnit = nullptr;
							if (attack.firstAttacker)
							{
								PreBattleChecks(attacker, attackerStats, defender, attack, &defenderDisplayHealth, distribution, gen);
							}
							else
							{
								PreBattleChecks(defender, defenderStats, attacker, attack, &attackerDisplayHealth, distribution, gen);
							}
							startPosition = *actingPosition;
							moveBack = true;
							movementDirection *= -1;
							if (movementDirection.x > 0)
							{
								actingUnit->sprite.facing = 2;
								actingUnit->sprite.currentFrame = 11;
								actingUnit->sprite.startingFrame = 11;
							}
							else
							{
								actingUnit->sprite.facing = 0;
								actingUnit->sprite.currentFrame = 3;
								actingUnit->sprite.startingFrame = 3;
							}
						}
						else
						{
							if (actingUnit == leftUnit)
							{
								actingUnit->sprite.facing = 2;
								actingUnit->sprite.currentFrame = 11;
								actingUnit->sprite.startingFrame = 11;
							}
							else
							{
								actingUnit->sprite.facing = 0;
								actingUnit->sprite.currentFrame = 3;
								actingUnit->sprite.startingFrame = 3;
							}
							actingUnit = nullptr;
							moveBack = false;
						}
					}
				}
				else
				{
					if (displays->state == NONE)
					{
						actionTimer += deltaTime;
						if (actionTimer >= actionDelay)
						{
							actionTimer = 0;

							if (battleQueue.size() > 0)
							{
								auto attack = battleQueue[0];
								if (attack.firstAttacker)
								{
									actingUnit = attacker;
								}
								else
								{
									actingUnit = defender;
								}
								if (actingUnit == leftUnit)
								{
									movementDirection = glm::ivec2(1, 0);
									actingPosition = &leftPosition;
								}
								else
								{
									movementDirection = glm::ivec2(-1, 0);
									actingPosition = &rightPosition;
								}
								startPosition = *actingPosition;
							}
							else if (deadUnit)
							{
								if (capturing)
								{
									unitCaptured = true;
								}
								else
								{
									unitDied = true;
									if (deadUnit->deathMessage != "")
									{
										displays->PlayerUnitDied(deadUnit, true);
									}
								}
							}
							else
							{
								//if either unit has accost and accost has not fired
								//reset battle queue
								if (!accostFired)
								{
									CheckAccost();
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
		}
		else
		{
			if (fadeBackMap)
			{
				fadeTimer += deltaTime;
				fadeAlpha -= deltaTime;
				if (fadeTimer >= fadeOutDelay)
				{
					fadeTimer = 0;
					fadeAlpha = 0;
					fadeBackMap = false;
					if (unitCaptured)
					{
						PrepareCapture();
						deadUnit = nullptr;
						unitCaptured = false;
						CaptureUnit();
					}
					else
					{
						displays->endBattle.notify(0);
					}
				}
			}
			else
			{
				MapUpdate(deltaTime, inputManager, distribution, gen);
			}
		}
	}
}

void BattleManager::PrepareCapture()
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
}

void BattleManager::MapUpdate(float deltaTime, InputManager& inputManager, std::uniform_int_distribution<int>* distribution, std::mt19937* gen)
{
	if (fadeBoxIn)
	{
		boxThing -= 1.6f;
		if (boxThing <= 0)
		{
			boxThing = 0;
			fadeBoxIn = false;
		}
	}
	else if (fadeBoxOut)
	{
		boxThing += 1.6f;
		if (boxThing >= 16)
		{
			boxThing = 16;
			fadeBoxOut = false;
			drawInfo = false;
			if (unitDied && deadUnit->deathMessage != "")
			{
				displays->PlayerUnitDied(deadUnit, false);
			}
		}
	}
	else if (unitDied)
	{
		if (displays->state == NONE)
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

				unitDied = false;
			}
		}
	}
	else if (unitCaptured)
	{
		PrepareCapture();
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
			displays->endBattle.notify(0);
			captureAnimation = false;
		}
	}
	else
	{
		if (actingUnit)
		{
			auto newPosition = actingUnit->sprite.getPosition() + movementDirection * 2.5f;
			actingUnit->sprite.SetPosition(newPosition);
			auto distance = glm::length(actingUnit->sprite.getPosition() - startPosition);
			if (distance > 8)
			{
				actingUnit->sprite.SetPosition(startPosition + movementDirection * 8.0f);
				if (!moveBack)
				{
					auto attack = battleQueue[0];
					battleQueue.erase(battleQueue.begin());
					if (attack.firstAttacker)
					{
						PreBattleChecks(attacker, attackerStats, defender, attack, &defenderDisplayHealth, distribution, gen);
					}
					else
					{
						PreBattleChecks(defender, defenderStats, attacker, attack, &attackerDisplayHealth, distribution, gen);
					}
					startPosition = actingUnit->sprite.getPosition();
					moveBack = true;
					movementDirection *= -1;
				}
				else
				{
					actingUnit = nullptr;
					moveBack = false;
				}
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
					if (attack.firstAttacker)
					{
						actingUnit = attacker;
					}
					else
					{
						actingUnit = defender;
					}
					if (actingUnit->sprite.facing == 0)
					{
						movementDirection = glm::ivec2(-1, 0);
					}
					else if (actingUnit->sprite.facing == 1)
					{
						movementDirection = glm::ivec2(0, -1);
					}
					else if (actingUnit->sprite.facing == 2)
					{
						movementDirection = glm::ivec2(1, 0);
					}
					else
					{
						movementDirection = glm::ivec2(0, 1);
					}
					startPosition = actingUnit->sprite.getPosition();
				}
				else if (deadUnit)
				{
					if (capturing)
					{
						unitCaptured = true;
						fadeBoxOut = true;
					}
					else
					{
						unitDied = true;
						fadeBoxOut = true;
					}
				}
				else
				{
					//if either unit has accost and accost has not fired
					//reset battle queue
					if (!accostFired)
					{
						CheckAccost();
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

void BattleManager::CheckAccost()
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

void BattleManager::PreBattleChecks(Unit* thisUnit, BattleStats& theseStats, Unit* foe, Attack& attack, int* foeHP, std::uniform_int_distribution<int>* distribution, std::mt19937* gen)
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
	targetHealth = foe->currentHP;
	displayHealth = foeHP;
}

void BattleManager::DoBattleAction(Unit* thisUnit, Unit* otherUnit, int accuracy, int crit, BattleStats& theseStats, Attack& attack, int foeDefense, std::uniform_int_distribution<int>* distribution, std::mt19937* gen)
{
	auto roll = (*distribution)(*gen);
	std::cout << "roll " << roll << std::endl;
	//Do roll to determine if hit
	if (roll <= accuracy)
	{
		int dealtDamage = theseStats.attackDamage;
		int critFactor = 1;
		if (crit > 0)
		{
			//if the hit lands, do another roll to determine if it is a critical hit. 
			//I don't know this for a fact, but I am assuming if crit rate is zero or 100, we don't bother
			if (crit == 100)
			{
				critFactor = 2;
			}
			else
			{
				roll = (*distribution)(*gen);
				std::cout << "crit roll " << roll << std::endl;
				if (roll <= crit)
				{
					critFactor = 2;
				}
			}
		}
		if (critFactor > 1)
		{
			missedText.message = "CRITICAL!";
			missedText.active = true;
			missedText.scale = 0.5f;
			ResourceManager::PlaySound("critHit");
		}
		dealtDamage *= critFactor;
		dealtDamage -= foeDefense;
		thisUnit->GetEquippedItem()->remainingUses--;
		otherUnit->TakeDamage(dealtDamage);

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
			//If I move active out of the base unit and have it only a property of ai units, will need a check here
			otherUnit->active = true;
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
		missedText.message = "Miss!";
		missedText.active = true;
		missedText.scale = 0.5f;
		ResourceManager::PlaySound("miss");
	}
	if (battleScene)
	{
		if (actingUnit == leftUnit)
		{
			missedText.position = rightPosition + glm::vec2(0.0f, 8.0f);

		}
		else
		{
			missedText.position = leftPosition + glm::vec2(0.0f, 8.0f);
		}
	}
	else
	{
		missedText.position = otherUnit->sprite.getPosition() + glm::vec2(0.0f, 8.0f);
	}
}

void BattleManager::EndAttack()
{
	if (capturing && !attacker->carriedUnit)
	{
		attacker->carryingMalus = 1;
	}
	endAttackSubject.notify(attacker, defender);
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
	if (deadUnit)
	{
		unitDiedSubject.notify(deadUnit);
		deadUnit = nullptr;
	}
	ResourceManager::ResumeMusic(1000);
}

void BattleManager::DropHeldUnit()
{
	if (unitToDrop)
	{
		auto deadPosition = unitToDrop->sprite.getPosition();
		unitToDrop->placeUnit(deadPosition.x, deadPosition.y);
		unitToDrop->hide = false;
		unitToDrop->carryingUnit->releaseUnit(); //once I am deleting the unit properly this shouldn't be neccessary
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

void BattleManager::GetUVs()
{
	mapBattleBoxUVs = ResourceManager::GetTexture("UIItems").GetUVs(0, 128, 96, 32, 2, 2, 3);
}

void BattleManager::Draw(TextRenderer* text, Camera& camera, SpriteRenderer* Renderer, Cursor* cursor, SBatch* Batch, int shapeVAO, TextObjectManager* textManager)
{
	if (aiDelay)
	{
		Renderer->setUVs(cursor->uvs[2]);
		Texture2D displayTexture = ResourceManager::GetTexture("UIItems");

		Renderer->DrawSprite(displayTexture, defender->sprite.getPosition() - glm::vec2(3), 0.0f, cursor->dimensions, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
	}
	if (transitionIn || fadeBackMap)
	{
		ResourceManager::GetShader("sprite").Use().SetFloat("maskX", transitionX);
		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		Renderer->setUVs();
		Texture2D displayTexture = ResourceManager::GetTexture("BattleFadeIn");
		Renderer->DrawSprite(displayTexture, glm::vec2(transitionX, 0), 0.0f, glm::vec2(286, 224), glm::vec4(1, 1, 1, fadeAlpha));
	}
	else if (battleScene)
	{
		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		Renderer->setUVs();
		Texture2D displayTexture = ResourceManager::GetTexture("BattleBG");
		Renderer->DrawSprite(displayTexture, glm::vec2(0, 16), 0.0f, glm::vec2(256, 111), glm::vec4(1, 1, 1, 1));

		displayTexture = ResourceManager::GetTexture("BattleSceneBoxes");
		Renderer->DrawSprite(displayTexture, glm::vec2(9, 129), 0.0f, glm::vec2(238, 78));

		ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		Batch->begin();
		std::vector<Sprite> carrySprites;
		leftUnit->Draw(Batch, leftPosition);
		rightUnit->Draw(Batch, rightPosition);
		Batch->end();
		Batch->renderBatch();

		DrawSceneHealthbars(camera, shapeVAO);

		int yOffset = 375;
		glm::vec2 leftDraw;
		glm::vec2 rightDraw;

		leftDraw = glm::vec2(200, yOffset);
		rightDraw = glm::vec2(512, yOffset);

		text->RenderText(rightUnit->name, 512, rightDraw.y, 1);
		rightDraw.y += 22.0f;

		auto rightWeapon = rightUnit->GetEquippedItem();
		if (rightWeapon)
		{
			text->RenderText(rightWeapon->name, 512, 412, 1);
		}

		text->RenderTextRight(intToString(*rightDisplayHealth), 437, 474, 1, 28);
		text->RenderTextRight(rightStats.Hit, 537, 495, 1, 28);
		text->RenderTextRight(rightStats.Atc, 537, 516, 1, 28);
		text->RenderTextRight(intToString(rightStats.Def), 687, 495, 1, 28);
		text->RenderTextRight(intToString(rightStats.Lvl), 687, 516, 1, 28);

		text->RenderText(leftUnit->name, leftDraw.x, leftDraw.y, 1);
		leftDraw.y += 22.0f;
		auto leftWeapon = leftUnit->GetEquippedItem();
		if (leftWeapon)
		{
			text->RenderText(leftWeapon->name, leftDraw.x, leftDraw.y, 1);
		}

		text->RenderTextRight(intToString(*leftDisplayHealth), 62, 474, 1, 28);
		text->RenderTextRight(leftStats.Hit, 162, 495, 1, 28);
		text->RenderTextRight(leftStats.Atc, 162, 516, 1, 28);
		text->RenderTextRight(intToString(leftStats.Def), 312, 495, 1, 28);
		text->RenderTextRight(intToString(leftStats.Lvl), 312, 516, 1, 28);

		if (missedText.active)
		{
			glm::vec2 drawPosition = glm::vec2(missedText.position.x/256.0f * 800, missedText.position.y/ 224.0f * 600);
			text->RenderTextCenter(missedText.message, drawPosition.x, drawPosition.y, missedText.scale, 40);
		}
		if (displays->state != NONE)
		{
			displays->Draw(&camera, text, shapeVAO, Renderer);
		}

		if (textManager->ShowText())
		{
			textManager->DrawOverBattleBox(&camera, shapeVAO);
			textManager->Draw(text, Renderer, &camera);
		}

		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		Renderer->setUVs();
		displayTexture = ResourceManager::GetTexture("BattleFadeIn");
		Renderer->DrawSprite(displayTexture, glm::vec2(0, 0), 0.0f, glm::vec2(286, 224), glm::vec4(1, 1, 1, fadeAlpha));
	}
	else if (drawInfo && !camera.moving)
	{
		int yOffset = 139;
		int boxY = 48;

		if (camera.position.y <= 112)
		{
			yOffset = 353;
			boxY = 128;
		}

		StencilWindow(camera, boxY, shapeVAO);

		glm::vec2 attackerBoxPosition;
		glm::vec2 defenderBoxPosition;
		//The hp should be drawn based on which side each unit is. So if the attacker is to the left of the defender, the hp should be on the left, and vice versa
		glm::vec2 attackerDraw;
		glm::vec2 defenderDraw;
		if (attacker->sprite.getPosition().x < defender->sprite.getPosition().x)
		{
			attackerDraw = glm::vec2(150, yOffset);
			defenderDraw = glm::vec2(450, yOffset);
			attackerBoxPosition = glm::vec2(32, boxY);
			defenderBoxPosition = glm::vec2(128, boxY);
		}
		else
		{
			defenderDraw = glm::vec2(150, yOffset);
			attackerDraw = glm::vec2(450, yOffset);
			defenderBoxPosition = glm::vec2(32, boxY);
			attackerBoxPosition = glm::vec2(128, boxY);
		}

		//Have to draw a shadow type of effect here
		ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		ResourceManager::GetShader("shape").SetFloat("alpha", 1.0f);
		glm::mat4 model = glm::mat4();
		model = glm::translate(model, glm::vec3(38, boxY + 14, 0.0f));

		model = glm::scale(model, glm::vec3(188, 20, 0.0f));

		ResourceManager::GetShader("shape").SetVector3f("shapeColor", glm::vec3(0.062f, 0.062f, 0.062f));

		ResourceManager::GetShader("shape").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		Texture2D displayTexture = ResourceManager::GetTexture("UIItems");
		if (capturing)
		{
			Renderer->setUVs(mapBattleBoxUVs[2]);
		}
		else
		{
			Renderer->setUVs(mapBattleBoxUVs[attacker->team]);
		}
		Renderer->DrawSprite(displayTexture, attackerBoxPosition, 0, glm::vec2(96, 32));
		Renderer->setUVs(mapBattleBoxUVs[defender->team]);
		Renderer->DrawSprite(displayTexture, defenderBoxPosition, 0, glm::vec2(96, 32));

		ResourceManager::GetShader("shapeSpecial").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(attackerBoxPosition.x + 32, boxY + 19, 0.0f));

		int width = 51 * (attackerDisplayHealth / float(attacker->maxHP));
		glm::vec2 scale(width, 4);

		model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

		ResourceManager::GetShader("shapeSpecial").SetVector3f("innerColor", glm::vec3(1, 0.984f, 0.352f));
		ResourceManager::GetShader("shapeSpecial").SetVector3f("outerColor", glm::vec3(0.482f, 0.0627f, 0));
		ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
		ResourceManager::GetShader("shapeSpecial").SetInteger("innerTop", 1);
		ResourceManager::GetShader("shapeSpecial").SetInteger("innerBottom", 3);
		ResourceManager::GetShader("shapeSpecial").SetInteger("shouldSkip", 0);
		ResourceManager::GetShader("shapeSpecial").SetFloat("alpha", 1);
		ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		model = glm::mat4();
		model = glm::translate(model, glm::vec3(defenderBoxPosition.x + 32, boxY + 19, 0.0f));

		width = 51 * (defenderDisplayHealth / float(defender->maxHP));
		scale.x = width;
		model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

		ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
		ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);

		text->RenderText(attacker->name, attackerDraw.x, attackerDraw.y, 1, glm::vec3(1.0f));
		attackerDraw.y += 35.0f;
		attackerDraw.x -= 25;
		text->RenderText(intToString(attackerDisplayHealth), attackerDraw.x, attackerDraw.y, 1, glm::vec3(1.0f));

		text->RenderText(defender->name, defenderDraw.x, defenderDraw.y, 1, glm::vec3(1.0f));
		defenderDraw.y += 35.0f;
		defenderDraw.x -= 25;
		text->RenderText(intToString(defenderDisplayHealth), defenderDraw.x, defenderDraw.y, 1, glm::vec3(1.0f));

		glDisable(GL_STENCIL_TEST);
		if (missedText.active)
		{
			glm::vec2 drawPosition = glm::vec2(missedText.position.x, missedText.position.y);
			drawPosition = camera.worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
			text->RenderTextCenter(missedText.message, drawPosition.x, drawPosition.y, missedText.scale, 40);
		}
	}
}

void BattleManager::DrawSceneHealthbars(Camera& camera, int shapeVAO)
{
	//Max
	ResourceManager::GetShader("shapeSpecial").Use().SetMatrix4("projection", camera.getOrthoMatrix());
	ResourceManager::GetShader("shapeSpecial").SetVector3f("innerColor", glm::vec3(0, 0.8901f, 0.4823f));
	ResourceManager::GetShader("shapeSpecial").SetVector3f("outerColor", glm::vec3(0.0627f, 0.1568f, 0.2235));
	ResourceManager::GetShader("shapeSpecial").SetInteger("innerTop", 3);
	ResourceManager::GetShader("shapeSpecial").SetInteger("innerBottom", 4);
	ResourceManager::GetShader("shapeSpecial").SetInteger("skipLine", 0);
	ResourceManager::GetShader("shapeSpecial").SetInteger("shouldSkip", 1);
	ResourceManager::GetShader("shapeSpecial").SetFloat("alpha", 1);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(36, 176, 0.0f));
	glm::vec2 scale(leftMaxHealth * 2 + 1, 7);
	//This nonsense is all to handle multiple health bars when hp is greater than 40
	//Only supports two, so a max hp of 80. Would require a rewrite if HP could be higher
	int extraX = 0;
	if (leftMaxHealth > 40)
	{
		extraX = (leftMaxHealth - 40) * 2 + 1;
		scale.x = 81;
	}
	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));

	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	if (extraX > 0)
	{
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(36, 168, 0.0f));
		model = glm::scale(model, glm::vec3(extraX, scale.y, 0.0f));

		ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(extraX, scale.y));

		ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(156, 176, 0.0f));
	scale.x = rightMaxHealth * 2 + 1;

	extraX = 0;
	if (rightMaxHealth > 40)
	{
		extraX = (rightMaxHealth - 40) * 2 + 1;
		scale.x = 81;
	}

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	if (extraX > 0)
	{
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(156, 168, 0.0f));
		model = glm::scale(model, glm::vec3(extraX, scale.y, 0.0f));

		ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(extraX, scale.y));

		ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	//Current
	ResourceManager::GetShader("shapeSpecial").SetVector3f("outerColor", glm::vec3(0, 0.8901f, 0.4823f));
	ResourceManager::GetShader("shapeSpecial").SetVector3f("innerColor", glm::vec3(1, 0.9843f, 0.9058f));
	ResourceManager::GetShader("shapeSpecial").SetInteger("innerTop", 2);
	ResourceManager::GetShader("shapeSpecial").SetInteger("innerBottom", 3);
	ResourceManager::GetShader("shapeSpecial").SetInteger("skipLine", 1);

	ResourceManager::GetShader("shapeSpecial").SetFloat("alpha", 0);

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(37, 177, 0.0f));

	scale = glm::vec2(std::max(0, *leftDisplayHealth * 2 - 1), 5);
	extraX = 0;
	if (*leftDisplayHealth > 40)
	{
		extraX = (*leftDisplayHealth - 40) * 2 - 1;
		scale.x = 79;
	}
	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	if (extraX > 0)
	{
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(37, 169, 0.0f));
		model = glm::scale(model, glm::vec3(extraX, scale.y, 0.0f));

		ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(extraX, scale.y));
		ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}

	model = glm::mat4();
	model = glm::translate(model, glm::vec3(157, 177, 0.0f));

	scale = glm::vec2(std::max(0, *rightDisplayHealth * 2 - 1), 5);

	extraX = 0;
	if (*rightDisplayHealth > 40)
	{
		extraX = (*rightDisplayHealth - 40) * 2 - 1;
		scale.x = 79;
	}

	model = glm::scale(model, glm::vec3(scale.x, scale.y, 0.0f));

	ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(scale.x, scale.y));
	ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	if (extraX > 0)
	{
		model = glm::mat4();
		model = glm::translate(model, glm::vec3(157, 169, 0.0f));
		model = glm::scale(model, glm::vec3(extraX, scale.y, 0.0f));

		ResourceManager::GetShader("shapeSpecial").SetVector2f("scale", glm::vec2(extraX, scale.y));
		ResourceManager::GetShader("shapeSpecial").SetMatrix4("model", model);
		glBindVertexArray(shapeVAO);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glBindVertexArray(0);
	}
}

void BattleManager::StencilWindow(Camera& camera, int boxY, int shapeVAO)
{
	//Cannot believe this works and barely understand it.
	//I THINK what is happening here is I turn on all this stuff
	glEnable(GL_STENCIL_TEST);
	glClear(GL_STENCIL_BUFFER_BIT);
	glStencilFunc(GL_ALWAYS, 1, 0xFF);
	glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);
	//Then the shape I draw here is my mask
	ResourceManager::GetShader("shape").Use().SetMatrix4("projection", camera.getOrthoMatrix());
	ResourceManager::GetShader("shape").SetFloat("alpha", 0.0f);
	glm::mat4 model = glm::mat4();
	model = glm::translate(model, glm::vec3(32, boxY + boxThing, 0.0f));

	model = glm::scale(model, glm::vec3(194, 34 - boxThing * 2, 0.0f));

	ResourceManager::GetShader("shape").SetMatrix4("model", model);
	glBindVertexArray(shapeVAO);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glBindVertexArray(0);

	//Then when I turn everything off, everything after is drawn in that mask? No idea man. Works though!
	glStencilFunc(GL_EQUAL, 1, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);
}
