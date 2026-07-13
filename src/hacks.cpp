#include "gui.h"
#include "hacks.h"
#include "globals.h"

#include <thread>
#include <math.h>
#include <cmath>
#include <cfloat>

void hacks::VisualThread(const Memory& mem) noexcept
{
	//Chams
	bool chamsReseted = false;

	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		//GLOW
		const auto localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
		const auto glowObjectManager = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwGlowObjectManager);

		if (!localPlayer || !glowObjectManager)
			continue;

		const auto localTeam = mem.Read<std::int32_t>(localPlayer + offsets::iTeamNum);

		for (auto i = 0; i <= 32; ++i)
		{
			const auto entity = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);
			if (!entity)
				continue;

			const auto stateLife = mem.Read<std::int32_t>(entity + offsets::lifeState);
			if (stateLife != 0)
				continue;

			const auto entityTeam = mem.Read<std::int32_t>(entity + offsets::iTeamNum);

			//Glow
			if (globals::glow)
			{
				const auto glowIndex = mem.Read<std::uintptr_t>(entity + offsets::iGlowIndex);

				if (entityTeam == localTeam)
				{
					mem.Write(glowObjectManager + (glowIndex * 0x38) + 0x8, globals::friendlyGlowColor[0]); //r
					mem.Write(glowObjectManager + (glowIndex * 0x38) + 0xC, globals::friendlyGlowColor[1]); //g
					mem.Write(glowObjectManager + (glowIndex * 0x38) + 0x10, globals::friendlyGlowColor[2]); //b
					mem.Write(glowObjectManager + (glowIndex * 0x38) + 0x14, globals::friendlyGlowColor[3]); //a
				}
				else
				{
					mem.Write(glowObjectManager + (glowIndex * 0x38) + 0x8, globals::enemyGlowColor[0]); //r
					mem.Write(glowObjectManager + (glowIndex * 0x38) + 0xC, globals::enemyGlowColor[1]); //g
					mem.Write(glowObjectManager + (glowIndex * 0x38) + 0x10, globals::enemyGlowColor[2]); //b
					mem.Write(glowObjectManager + (glowIndex * 0x38) + 0x14, globals::enemyGlowColor[3]); //a
				}

				mem.Write<bool>(glowObjectManager + (glowIndex * 0x38) + 0x27, true);
				mem.Write<bool>(glowObjectManager + (glowIndex * 0x38) + 0x28, true);
				mem.Write<bool>(glowObjectManager + (glowIndex * 0x38) + 0x29, false);
			}

			//Chams
			if (globals::chams)
			{
				chamsReseted = false;

				if (entityTeam == localTeam)
				{
					Color color = ToColor(globals::friendlyChamsColor);
					mem.Write(entity + offsets::clrRender, color);
				}
				else
				{
					Color color = ToColor(globals::enemyChamsColor);
					mem.Write(entity + offsets::clrRender, color);
				}

				const auto _this = static_cast<std::uintptr_t>(globals::engineAddress + offsets::model_ambient_min - 0x2c);
				mem.Write<std::int32_t>(globals::engineAddress + offsets::model_ambient_min, *reinterpret_cast<std::uintptr_t*>(&globals::chamsBright) ^ _this);
			}
			else
			{
				if (!chamsReseted)
				{
					globals::chamsBright = 0.0f;

					const auto _this = static_cast<std::uintptr_t>(globals::engineAddress + offsets::model_ambient_min - 0x2c);
					mem.Write<std::int32_t>(globals::engineAddress + offsets::model_ambient_min, *reinterpret_cast<std::uintptr_t*>(&globals::chamsBright) ^ _this);

					chamsReseted = true;
				}
			}

			//Radar
			if (globals::radar)
			{
				//If own team do not trigger shown on radar
				if (entityTeam == localTeam)
					continue;

				mem.Write<bool>(entity + offsets::bSpotted, true);
			}
		}
	}
}


void hacks::AimThread(const Memory& mem) noexcept
{
	//no recoil
	auto oldPunch = Vector2{ };

	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));

		//aimbot variables
		int localPlayerIndex = -1;
		int closestDistanceIndex = -1;

		int bestVisibleIndex = -1;
		float bestVisibleDistance = FLT_MAX;

		int bestAnyIndex = -1;
		float bestAnyDistance = FLT_MAX;

		const auto& localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
		if (!localPlayer)
			continue;

		if (globals::aimbot)
		{
			const auto localTeam = mem.Read<std::int32_t>(localPlayer + offsets::iTeamNum);

			for (int i = 0; i <= 32; i++)
			{
				const auto entity = mem.Read<uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);

				if (entity == localPlayer)
				{
					localPlayerIndex = i;
					break;
				}
			}

			if (localPlayerIndex != -1)
			{
				for (auto i = 0; i <= 32; ++i)
				{
					const auto entity = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + i * 0x10);
					if (entity)
					{
						//Skip if entity is player
						if ((uint32_t)entity != (uint32_t)localPlayer)
						{
							const auto entityTeam = mem.Read<std::int32_t>(entity + offsets::iTeamNum);

							//Skip if on player's team
							if (entityTeam != localTeam)
							{
								//Continue if entity and player aren't dead
								const auto playerHealth = mem.Read<std::int32_t>(localPlayer + offsets::iHealth);
								const auto entityHealth = mem.Read<std::int32_t>(entity + offsets::iHealth);

								if (playerHealth && entityHealth)
								{
									//Get positions
									Vector3 playerPos = mem.Read<Vector3>(localPlayer + offsets::vecOrigin);
									Vector3 entityOrigin = mem.Read<Vector3>(entity + offsets::vecOrigin);

									//check if visible (prioritize visible)
									uint32_t spottedMask = mem.Read<uint32_t>(entity + offsets::bSpottedByMask);
									if ((spottedMask & (1 << localPlayerIndex)))
									{
										//Calculate distance
										float distance = GetDistance(entityOrigin, playerPos);
										if (distance < bestVisibleDistance)
										{
											bestVisibleDistance = distance;
											bestVisibleIndex = i;
											bestAnyIndex = i;
										}
									}
									else
									{
										//Calculate distance
										float distance = GetDistance(entityOrigin, playerPos);
										if (distance < bestAnyDistance)
										{
											bestAnyDistance = distance;
											bestAnyIndex = i;
										}
									}
								}
							}
						}
					}
				}
			}

			if (bestVisibleIndex != -1)
			{
				closestDistanceIndex = bestVisibleIndex;
			}
			else if (bestAnyIndex != -1)
			{
				closestDistanceIndex = bestAnyIndex;
			}

			if (closestDistanceIndex != -1)
			{
				//Get target
				const auto target = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + closestDistanceIndex * 0x10);
				if (target)
				{
					//Check if target is alive (after shooting it might be dead)
					const auto targetHealth = mem.Read<std::int32_t>(target + offsets::iHealth);
					if (targetHealth)
					{
						//Get target head position
						uint32_t boneMatrix = mem.Read<uint32_t>(target + offsets::dwBoneMatrix);
						static Vector3 targetPos;

						int boneIndex = 8; // head
						targetPos.x = mem.Read<float>(boneMatrix + 0x30 * boneIndex + 0x0C);
						targetPos.y = mem.Read<float>(boneMatrix + 0x30 * boneIndex + 0x1C);
						targetPos.z = mem.Read<float>(boneMatrix + 0x30 * boneIndex + 0x2C);

						//calculate offset (camera is not in player's feet)
						Vector3 playerPos = mem.Read<Vector3>(localPlayer + offsets::vecOrigin);
						Vector3 viewOffset = mem.Read<Vector3>(localPlayer + offsets::vecViewOffset);
						Vector3 myPos = playerPos + viewOffset;

						//Convert direction into angles
						Vector3 deltaVec = { targetPos.x - myPos.x, targetPos.y - myPos.y, targetPos.z - myPos.z };
						float deltaVecLength = sqrt(deltaVec.x * deltaVec.x + deltaVec.y * deltaVec.y + deltaVec.z * deltaVec.z);

						float pitch = -asin(deltaVec.z / deltaVecLength) * (180 / 3.141592653589);
						float yaw = atan2(deltaVec.y, deltaVec.x) * (180 / 3.141592653589);

						//Write new view angles
						Vector2 newAngles;
						if (pitch >= -89 && pitch <= 89 && yaw >= -180 && yaw <= 180)
						{
							newAngles.x = pitch;
							newAngles.y = yaw;
						}
						if (!GetAsyncKeyState(5))
						{
							const auto& clientState = mem.Read<std::uintptr_t>(globals::engineAddress + offsets::dwClientState);
							mem.Write<Vector2>(clientState + offsets::dwClientState_ViewAngles, newAngles);


							//Check if target is not behind wall
							const auto& crosshairId = mem.Read<std::int32_t>(localPlayer + offsets::iCrosshairId);
							if (crosshairId && crosshairId <= 32)
							{
								//Dont shoot if teammate got onto crosshair
								const auto& crosshairEntity = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + (crosshairId - 1) * 0x10);
								const auto crosshairEntityTeam = mem.Read<std::int32_t>(crosshairEntity + offsets::iTeamNum);
								if (crosshairEntityTeam != localTeam)
								{
									//shoot
									mem.Write<std::uintptr_t>(globals::clientAddress + offsets::dwForceAttack, 6);
									std::this_thread::sleep_for(std::chrono::milliseconds(80));
									mem.Write<std::uintptr_t>(globals::clientAddress + offsets::dwForceAttack, 4);
								}
							}
						}
					}
					else
					{
						closestDistanceIndex = -1;
					}
				}
			}
		}

		if (globals::triggerbot)
		{
			bool shouldRun = !globals::triggerbotBindEnabled || GetAsyncKeyState(0x51);

			if (shouldRun)
			{
				const auto& localHealth = mem.Read<std::int32_t>(localPlayer + offsets::iHealth);

				//continue if player is alive
				if (localHealth)
				{
					const auto& crosshairId = mem.Read<std::int32_t>(localPlayer + offsets::iCrosshairId);

					if (crosshairId && crosshairId <= 32)
					{
						const auto& entity = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwEntityList + (crosshairId - 1) * 0x10);

						//continue if entity is alive
						if (mem.Read<std::int32_t>(entity + offsets::iHealth))
						{
							//continue if entity is not on our team
							if (mem.Read<std::int32_t>(entity + offsets::iTeamNum) != mem.Read<std::int32_t>(localPlayer + offsets::iTeamNum))
							{
								mem.Write<std::uintptr_t>(globals::clientAddress + offsets::dwForceAttack, 6);
								std::this_thread::sleep_for(std::chrono::milliseconds(20));
								mem.Write<std::uintptr_t>(globals::clientAddress + offsets::dwForceAttack, 4);
							}
						}
					}
				}
			}
		}

		//no recoil
		if (globals::norecoil)
		{
			const auto& shotsFired = mem.Read<std::int32_t>(localPlayer + offsets::iShotsFired);

			if (shotsFired)
			{
				const auto& clientState = mem.Read<std::uintptr_t>(globals::engineAddress + offsets::dwClientState);
				const auto& viewAngles = mem.Read<Vector2>(clientState + offsets::dwClientState_ViewAngles);

				const auto& aimPunch = mem.Read<Vector2>(localPlayer + offsets::m_aimPunchAngle);

				auto newAngles = Vector2
				{
					viewAngles.x + oldPunch.x - aimPunch.x * 2.f,
					viewAngles.y + oldPunch.y - aimPunch.y * 2.f
				};

				if (newAngles.x > 89.f)
					newAngles.x = 89.f;

				if (newAngles.x < -89.f)
					newAngles.x = -89.f;

				while (newAngles.y > 180.f)
					newAngles.y -= 360.f;

				while (newAngles.y < -180.f)
					newAngles.y += 360.f;


				mem.Write<Vector2>(clientState + offsets::dwClientState_ViewAngles, newAngles);

				oldPunch.x = aimPunch.x * 2.f;
				oldPunch.y = aimPunch.y * 2.f;
			}
			else
			{
				oldPunch.x = oldPunch.y = 0.f;
			}
		}
	}
}

void hacks::MoveThread(const Memory& mem) noexcept
{
	while (gui::isRunning)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(5));

		const auto& localPlayer = mem.Read<std::uintptr_t>(globals::clientAddress + offsets::dwLocalPlayer);
		if (!localPlayer)
			continue;

		if (globals::bhop)
		{
			const auto onGround = mem.Read<bool>(localPlayer + offsets::fFlags);

			if (GetAsyncKeyState(VK_SPACE) && onGround & (1 << 0))
				mem.Write<BYTE>(globals::clientAddress + offsets::dwForceJump, 6);
		}
	}
}