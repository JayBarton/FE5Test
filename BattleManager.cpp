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
		if (battleScene)
		{	

			transitionX = -286.0f;
			ResourceManager::GetShader("sprite").Use().SetVector2f("cameraPosition", (camera.position - glm::vec2(camera.halfWidth, camera.halfHeight)));
			ResourceManager::GetShader("sprite").SetFloat("maskX", transitionX);
			ResourceManager::GetShader("sprite").SetInteger("battleScreen", 1);
			transitionIn = true;
			fadeAlpha = 0.2f;
			GetFacing();
			if (!aiDelay)
			{
				ResourceManager::PlaySound("battleTransition");
				ResourceManager::FadeOutPause(500);
			}
		}
		else
		{
			GetFacing();
			attacker->sprite.moveAnimate = true;
			defender->sprite.moveAnimate = true;
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
 
void BattleManager::Update(float deltaTime, std::mt19937* gen, std::uniform_int_distribution<int>* distribution, InfoDisplays& displays, InputManager& inputManager)
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
					displays.ClearLevelUpDisplay();
				}
			}
			else if (unitDied || unitCaptured)
			{
				if (displays.state == NONE)
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
								PreBattleChecks(attacker, attackerStats, defender, attack, distribution, gen);
							}
							else
							{
								PreBattleChecks(defender, defenderStats, attacker, attack, distribution, gen);
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
						displays.endBattle.notify(0);
					}
				}
			}
			else
			{
				MapUpdate(displays, deltaTime, inputManager, distribution, gen);
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

void BattleManager::MapUpdate(InfoDisplays& displays, float deltaTime, InputManager& inputManager, std::uniform_int_distribution<int>* distribution, std::mt19937* gen)
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
			displays.endBattle.notify(0);
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
						PreBattleChecks(attacker, attackerStats, defender, attack, distribution, gen);
					}
					else
					{
						PreBattleChecks(defender, defenderStats, attacker, attack, distribution, gen);
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

void BattleManager::Draw(TextRenderer* text, Camera& camera, SpriteRenderer* Renderer, Cursor* cursor, SBatch* Batch, InfoDisplays& displays, int shapeVAO)
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

		ResourceManager::GetShader("sprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		Batch->begin();
		std::vector<Sprite> carrySprites;
		leftUnit->Draw(Batch, leftPosition);
		rightUnit->Draw(Batch, rightPosition);
		Batch->end();
		Batch->renderBatch();

		int yOffset = 375;
		glm::vec2 leftDraw;
		glm::vec2 rightDraw;

		leftDraw = glm::vec2(200, yOffset);
		rightDraw = glm::vec2(512, yOffset);

		text->RenderText(rightUnit->name, rightDraw.x, rightDraw.y, 1);
		rightDraw.y += 22.0f;

		auto rightWeapon = rightUnit->GetEquippedItem();
		if (rightWeapon)
		{
			text->RenderText(rightWeapon->name, rightDraw.x, rightDraw.y, 1);
		}

		text->RenderText("HP", rightDraw.x, 474, 1, glm::vec3(0.1f, 0.11f, 0.22f));
		rightDraw.x += 25;
		text->RenderText(intToString(rightUnit->currentHP) + "/" + intToString(rightUnit->maxHP), rightDraw.x, 474, 1);

		text->RenderText(leftUnit->name, leftDraw.x, leftDraw.y, 1);
		leftDraw.y += 22.0f;
		auto leftWeapon = leftUnit->GetEquippedItem();
		if (leftWeapon)
		{
			text->RenderText(leftWeapon->name, leftDraw.x, leftDraw.y, 1);
		}

		text->RenderText("HP", leftDraw.x, 474, 1, glm::vec3(0.1f, 0.11f, 0.22f));
		leftDraw.x += 25;
		text->RenderText(intToString(leftUnit->currentHP) + "/" + intToString(leftUnit->maxHP), leftDraw.x, 474, 1);

		if (missedText.active)
		{
			glm::vec2 drawPosition = glm::vec2(missedText.position.x/256.0f * 800, missedText.position.y/ 224.0f * 600);
			text->RenderTextCenter(missedText.message, drawPosition.x, drawPosition.y, missedText.scale, 40);
		}
		if (displays.state != NONE)
		{
			displays.Draw(&camera, text, shapeVAO, Renderer);
		}
		ResourceManager::GetShader("Nsprite").Use().SetMatrix4("projection", camera.getOrthoMatrix());
		Renderer->setUVs();
		displayTexture = ResourceManager::GetTexture("BattleFadeIn");
		Renderer->DrawSprite(displayTexture, glm::vec2(0, 0), 0.0f, glm::vec2(286, 224), glm::vec4(1, 1, 1, fadeAlpha));
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

		text->RenderText(attacker->name, attackerDraw.x, attackerDraw.y, 1, glm::vec3(0.0f));
		attackerDraw.y += 22.0f;
		text->RenderText("HP", attackerDraw.x, attackerDraw.y, 1, glm::vec3(0.1f, 0.11f, 0.22f));
		attackerDraw.x += 25;
		text->RenderText(intToString(attacker->currentHP) + "/" + intToString(attacker->maxHP), attackerDraw.x, attackerDraw.y, 1, glm::vec3(0.0f));

		text->RenderText(defender->name, defenderDraw.x, defenderDraw.y, 1, glm::vec3(0.0f));
		defenderDraw.y += 22.0f;
		text->RenderText("HP", defenderDraw.x, defenderDraw.y, 1, glm::vec3(0.1f, 0.11f, 0.22f));
		defenderDraw.x += 25;
		text->RenderText(intToString(defender->currentHP) + "/" + intToString(defender->maxHP), defenderDraw.x, defenderDraw.y, 1, glm::vec3(0.0f));

		if (missedText.active)
		{
			glm::vec2 drawPosition = glm::vec2(missedText.position.x, missedText.position.y);
			drawPosition = camera.worldToRealScreen(drawPosition, SCREEN_WIDTH, SCREEN_HEIGHT);
			text->RenderTextCenter(missedText.message, drawPosition.x, drawPosition.y, missedText.scale, 40);
		}
	}
}
