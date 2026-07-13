#pragma once
#include <cstdint>
#include <cstddef>

namespace globals 
{
	inline std::uintptr_t clientAddress = 0;
	inline std::uintptr_t engineAddress = 0;

	//Glow
	inline bool glow = false;
	inline float friendlyGlowColor[] = { 0.f, 1.f, 0.f, 1.f };
	inline float enemyGlowColor[] = { 1.f, 0.f, 0.f, 1.f };

	//Chams
	inline bool chams = false;
	inline float friendlyChamsColor[] = { 0.f, 1.f, 0.f};
	inline float enemyChamsColor[] = { 1.f, 0.f, 0.f};
	inline float chamsBright = 25.f;

	//Radar
	inline bool radar = false;

	//Aimbot
	inline bool aimbot = false;

	//Recoil
	inline bool norecoil = false;

	//Trigger
	inline bool triggerbot = false;
	inline bool triggerbotBindEnabled = false;

	//Bhop
	inline bool bhop = false;
}

namespace offsets
{
	constexpr auto dwLocalPlayer = 0xDEF97C;
	constexpr auto dwEntityList = 0x4E051DC;
	constexpr auto dwGlowObjectManager = 0x535FCB8;
	constexpr auto dwForceAttack = 0x3233024;
	constexpr auto iTeamNum = 0xF4;
	constexpr auto iGlowIndex = 0x10488;
	constexpr auto lifeState = 0x25F;
	constexpr auto iHealth = 0x100;
	constexpr auto bSpotted = 0x93D;
	constexpr auto iCrosshairId = 0x11838;
	constexpr auto iShotsFired = 0x103E0;
	constexpr auto dwClientState = 0x59F19C;
	constexpr auto dwClientState_MaxPlayer = 0x388;
	constexpr auto dwClientState_ViewAngles = 0x4D90;
	constexpr auto m_aimPunchAngle = 0x303C;
	constexpr auto fFlags = 0x104;
	constexpr auto dwForceJump = 0x52C0F50;
	constexpr auto model_ambient_min = 0x5A1194;
	constexpr auto clrRender = 0x70;
	constexpr auto vecOrigin = 0x138;
	constexpr auto vecViewOffset = 0x108;
	constexpr auto dwBoneMatrix = 0x26A8;
	constexpr auto bSpottedByMask = 0x980;
}

struct Color
{
	std::uint8_t r{ }, g{ }, b{ };
};

inline Color ToColor(const float color[4])
{
	return {
		static_cast<uint8_t>(color[0] * 255.f),
		static_cast<uint8_t>(color[1] * 255.f),
		static_cast<uint8_t>(color[2] * 255.f)
	};
}

struct Color2
{
	std::uint8_t r{ }, g{ }, b{ }, a{ };
};

struct Vector2
{
	float x = { }, y = { };
};

struct Vector3
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

	Vector3 operator-(const Vector3& other) const
	{
		return { x - other.x, y - other.y, z - other.z };
	}

	Vector3 operator+(const Vector3& other) const
	{
		return { x + other.x, y + other.y, z + other.z };
	}

	float Length() const
	{
		return sqrtf(x * x + y * y + z * z);
	}
};

inline float GetDistance(Vector3 first, Vector3 secound)
{
	return (first - secound).Length();
};