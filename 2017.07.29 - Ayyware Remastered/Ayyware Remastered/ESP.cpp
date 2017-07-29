/*
AyyWare 2 - Extreme Alien Technology
By Syn
*/

#include "ESP.h"
#include "Interfaces.h"
#include "Render.h"
#include "Menu.h"

void CEsp::Setup()
{
	BombCarrier = nullptr;
}

// Yeah dude we're defo gunna do some sick moves for the esp yeah
void CEsp::Move(CUserCmd *pCmd) {}

// Main ESP Drawing loop
void CEsp::Draw()
{
	IClientEntity *pLocal = HackManager.pLocal();

	// Loop through all active entitys
	for (int i = 0; i < Interfaces::EntList->GetHighestEntityIndex(); i++)
	{
		// Get the entity
		IClientEntity *pEntity = Interfaces::EntList->GetClientEntity(i);
		player_info_t pinfo;

		// The entity isn't some laggy peice of shit or something
		if (pEntity &&  pEntity != pLocal && !pEntity->IsDormant())
		{
			// Is it a player?!
			if (Menu::Window.VisualsTab.FiltersPlayers.GetState() && Interfaces::Engine->GetPlayerInfo(i, &pinfo) && pEntity->IsAlive())
			{
				DrawPlayer(pEntity, pinfo);
			}

			// ~ Other ESP's here (items and shit) ~ //
			ClientClass* cClass = (ClientClass*)pEntity->GetClientClass();

			// Dropped weapons
			if (Menu::Window.VisualsTab.FiltersWeapons.GetState() && cClass->m_ClassID != (int)CSGOClassID::CBaseWeaponWorldModel && ((strstr(cClass->m_pNetworkName, "Weapon") || cClass->m_ClassID == (int)CSGOClassID::CDEagle || cClass->m_ClassID == (int)CSGOClassID::CAK47)))
			{
				DrawDrop(pEntity, cClass);
			}

			// If entity is the bomb
			if (Menu::Window.VisualsTab.FiltersC4.GetState())
			{
				if (cClass->m_ClassID == (int)CSGOClassID::CPlantedC4)
					DrawBombPlanted(pEntity, cClass);

				if (cClass->m_ClassID == (int)CSGOClassID::CC4)
					DrawBomb(pEntity, cClass);
			}

			// If entity is a chicken
			if (Menu::Window.VisualsTab.FiltersChickens.GetState())
			{
				if (cClass->m_ClassID == (int)CSGOClassID::CChicken)
					DrawChicken(pEntity, cClass);
			}
		}
	}
}

//  Yeah m8
void CEsp::DrawPlayer(IClientEntity* pEntity, player_info_t pinfo)
{
	ESPBox Box;
	Color Color;

	// Show own team false? well gtfo teammate
	if (Menu::Window.VisualsTab.FiltersEnemiesOnly.GetState() && (pEntity->GetTeamNum() == HackManager.pLocal()->GetTeamNum()))
		return;

	if (GetPlayerBox(pEntity, Box))
	{
		Color = GetPlayerColor(pEntity);

		if (Menu::Window.VisualsTab.OptionsBox.GetState())
			DrawBox(Box, Color);

		if (Menu::Window.VisualsTab.OptionsName.GetState())
			DrawName(pinfo, Box);

		if (Menu::Window.VisualsTab.OptionsHealth.GetState())
			DrawHealth(pEntity, Box);

		if (Menu::Window.VisualsTab.OptionsInfo.GetState() || Menu::Window.VisualsTab.OptionsWeapon.GetState())
			DrawInfo(pEntity, Box);

		if (Menu::Window.VisualsTab.OptionsAimSpot.GetState())
			DrawCross(pEntity);

		if (Menu::Window.VisualsTab.OptionsSkeleton.GetState())
			DrawSkeleton(pEntity);
	}
}

// Gets the 2D bounding box for the entity
// Returns false on failure nigga don't fail me
bool CEsp::GetBox(IClientEntity* pEntity, CEsp::ESPBox &result)
{
	// Variables
	Vector  vOrigin, min, max, sMin, sMax, sOrigin,
		flb, brt, blb, frt, frb, brb, blt, flt;
	float left, top, right, bottom;

	// Get the locations
	vOrigin = pEntity->GetOrigin();
	min = pEntity->collisionProperty()->GetMins() + vOrigin;
	max = pEntity->collisionProperty()->GetMaxs() + vOrigin;

	// Points of a 3d bounding box
	Vector points[] = { Vector(min.x, min.y, min.z),
		Vector(min.x, max.y, min.z),
		Vector(max.x, max.y, min.z),
		Vector(max.x, min.y, min.z),
		Vector(max.x, max.y, max.z),
		Vector(min.x, max.y, max.z),
		Vector(min.x, min.y, max.z),
		Vector(max.x, min.y, max.z) };

	// Get screen positions
	if (!Render::WorldToScreen(points[3], flb) || !Render::WorldToScreen(points[5], brt)
		|| !Render::WorldToScreen(points[0], blb) || !Render::WorldToScreen(points[4], frt)
		|| !Render::WorldToScreen(points[2], frb) || !Render::WorldToScreen(points[1], brb)
		|| !Render::WorldToScreen(points[6], blt) || !Render::WorldToScreen(points[7], flt))
		return false;

	// Put them in an array (maybe start them off in one later for speed?)
	Vector arr[] = { flb, brt, blb, frt, frb, brb, blt, flt };

	// Init this shit
	left = flb.x;
	top = flb.y;
	right = flb.x;
	bottom = flb.y;

	// Find the bounding corners for our box
	for (int i = 1; i < 8; i++)
	{
		if (left > arr[i].x)
			left = arr[i].x;
		if (bottom < arr[i].y)
			bottom = arr[i].y;
		if (right < arr[i].x)
			right = arr[i].x;
		if (top > arr[i].y)
			top = arr[i].y;
	}

	// Width / height
	result.x = left;
	result.y = top;
	result.w = right - left;
	result.h = bottom - top;

	return true;
}

// Less accurate in terms of the size but the positioning should be better
bool CEsp::GetPlayerBox(IClientEntity* pEntity, ESPBox &result)
{
	Vector wOrig, wHead, sOrig, sHead;
	wOrig = pEntity->GetOrigin();
	wHead = pEntity->GetHeadPos() + Vector(0,0,7.f);

	if (!Render::WorldToScreen(wOrig, sOrig) || !Render::WorldToScreen(wHead, sHead))
		return false;

	float Height = sOrig.y - sHead.y;
	float Width = Height * 0.6;

	result.x = sHead.x - (Width / 2.f);
	result.y = sHead.y;
	result.w = Width;
	result.h = Height;
}

// Get an entities color depending on team and vis ect
Color CEsp::GetPlayerColor(IClientEntity* pEntity)
{
	int TeamNum = pEntity->GetTeamNum();
	bool IsVis = IsVisible(HackManager.pLocal(), pEntity, (int)CSGOHitboxID::Head);

	Color color;

	if (TeamNum == TEAM_CS_T)
	{
		if (IsVis)
			color = Color(235, 200, 0, 255);
		else
			color = Color(235, 50, 0, 255);
	}
	else
	{
		if (IsVis)
			color = Color(120, 210, 26, 255);
		else
			color = Color(15, 110, 220, 255);
	}


	return color;
}

// 2D  Esp box
void CEsp::DrawBox(CEsp::ESPBox size, Color color)
{
	//if (PlayerBoxes->GetStringIndex() == 1)
	//{
	//	// Full Box
	//	renderMan->Outline(size.x, size.y, size.w, size.h, color);
	//	renderMan->Outline(size.x - 1, size.y - 1, size.w + 2, size.h + 2, COLORCODE(10, 10, 10, 150));
	//	renderMan->Outline(size.x + 1, size.y + 1, size.w - 2, size.h - 2, COLORCODE(10, 10, 10, 150));
	//}
	//else
	//{
	// Corner Box
	int VertLine = (((float)size.w) * (0.20f));
	int HorzLine = (((float)size.h) * (0.20f));

	Render::Clear(size.x, size.y - 1, VertLine, 1, Color(10, 10, 10, 150));
	Render::Clear(size.x + size.w - VertLine, size.y - 1, VertLine, 1, Color(10, 10, 10, 150));
	Render::Clear(size.x, size.y + size.h - 1, VertLine, 1, Color(10, 10, 10, 150));
	Render::Clear(size.x + size.w - VertLine, size.y + size.h - 1, VertLine, 1, Color(10, 10, 10, 150));

	Render::Clear(size.x - 1, size.y, 1, HorzLine, Color(10, 10, 10, 150));
	Render::Clear(size.x - 1, size.y + size.h - HorzLine, 1, HorzLine, Color(10, 10, 10, 150));
	Render::Clear(size.x + size.w - 1, size.y, 1, HorzLine, Color(10, 10, 10, 150));
	Render::Clear(size.x + size.w - 1, size.y + size.h - HorzLine, 1, HorzLine, Color(10, 10, 10, 150));

	Render::Clear(size.x, size.y, VertLine, 1, color);
	Render::Clear(size.x + size.w - VertLine, size.y, VertLine, 1, color);
	Render::Clear(size.x, size.y + size.h, VertLine, 1, color);
	Render::Clear(size.x + size.w - VertLine, size.y + size.h, VertLine, 1, color);

	Render::Clear(size.x, size.y, 1, HorzLine, color);
	Render::Clear(size.x, size.y + size.h - HorzLine, 1, HorzLine, color);
	Render::Clear(size.x + size.w, size.y, 1, HorzLine, color);
	Render::Clear(size.x + size.w, size.y + size.h - HorzLine, 1, HorzLine, color);
	//}
}

std::wstring stringToWide(const std::string& text)
{
	std::wstring wide(text.length(), L' ');
	std::copy(text.begin(), text.end(), wide.begin());

	return wide;
}

// Player name
void CEsp::DrawName(player_info_t pinfo, CEsp::ESPBox size)
{
	RECT nameSize = Render::GetTextSize(Render::Fonts::ESP, pinfo.name);
	Render::Text(size.x + (size.w / 2) - (nameSize.right / 2), size.y - 16,
		Color(255, 255, 255, 255), Render::Fonts::ESP, stringToWide(pinfo.name).c_str());
}

void CEsp::DrawHealth(IClientEntity* pEntity, CEsp::ESPBox size)
{
	ESPBox HealthBar = size;
	HealthBar.y += (HealthBar.h + 6);
	HealthBar.h = 4;

	float HealthValue = pEntity->GetHealth();
	float HealthPerc = HealthValue / 100.f;
	float Width = (size.w * HealthPerc);
	HealthBar.w = Width;

	// --  Main Bar -- //

	Vertex_t Verts[4];
	Verts[0].Init(Vector2D(HealthBar.x, HealthBar.y));
	Verts[1].Init(Vector2D(HealthBar.x + size.w + 5, HealthBar.y));
	Verts[2].Init(Vector2D(HealthBar.x + size.w, HealthBar.y + 5));
	Verts[3].Init(Vector2D(HealthBar.x - 5, HealthBar.y + 5));

	Render::PolygonOutline(4, Verts, Color(10, 10, 10, 255), Color(255, 255, 255, 170));

	Vertex_t Verts2[4];
	Verts2[0].Init(Vector2D(HealthBar.x + 1, HealthBar.y + 1));
	Verts2[1].Init(Vector2D(HealthBar.x + HealthBar.w + 4, HealthBar.y + 1));
	Verts2[2].Init(Vector2D(HealthBar.x + HealthBar.w, HealthBar.y + 5));
	Verts2[3].Init(Vector2D(HealthBar.x - 4, HealthBar.y + 5));

	Color c = GetPlayerColor(pEntity);
	Render::Polygon(4, Verts2, c);

	Verts2[0].Init(Vector2D(HealthBar.x + 1, HealthBar.y + 1));
	Verts2[1].Init(Vector2D(HealthBar.x + HealthBar.w + 2, HealthBar.y + 1));
	Verts2[2].Init(Vector2D(HealthBar.x + HealthBar.w, HealthBar.y + 2));
	Verts2[3].Init(Vector2D(HealthBar.x - 2, HealthBar.y + 2));

	Render::Polygon(4, Verts2, Color(255, 255, 255, 40));

}

// Cleans the internal class name up to something human readable and nice
std::string CleanItemName(std::string name)
{
	std::string Name = name;
	// Tidy up the weapon Name
	if (Name[0] == 'C')
		Name.erase(Name.begin());

	// Remove the word Weapon
	auto startOfWeap = Name.find("Weapon");
	if (startOfWeap != std::string::npos)
		Name.erase(Name.begin() + startOfWeap, Name.begin() + startOfWeap + 6);

	return Name;
}

// Anything else: weapons, class state? idk
void CEsp::DrawInfo(IClientEntity* pEntity, CEsp::ESPBox size)
{
	std::vector<std::string> Info;

	// Player Weapon ESP
	IClientEntity* pWeapon = Interfaces::EntList->GetClientEntityFromHandle((HANDLE)pEntity->GetActiveWeaponHandle());
	if (Menu::Window.VisualsTab.OptionsWeapon.GetState() && pWeapon)
	{
		ClientClass* cClass = (ClientClass*)pWeapon->GetClientClass();
		if (cClass)
		{
			// Draw it
			std::string meme = CleanItemName(cClass->m_pNetworkName);
			RECT nameSize = Render::GetTextSize(Render::Fonts::ESP, meme.c_str());
			Render::Text(size.x + (size.w / 2) - (nameSize.right / 2), size.y + size.h + 16,
				Color(255, 255, 255, 255), Render::Fonts::ESP, meme.c_str());
		}
	}

	// Bomb Carrier
	if (Menu::Window.VisualsTab.OptionsInfo.GetState() && pEntity == BombCarrier)
	{
		Info.push_back("Bomb Carrier");
	}
	

	// Comp Rank
	if (Menu::Window.VisualsTab.OptionsInfo.GetState())
	{
		int CompRank = GetPlayerCompRank(pEntity);
		static const char *Ranks[] =
		{
			" ",
			"Silver I",
			"Silver II",
			"Silver III",
			"Silver IV",
			"Silver Elite",
			"Silver Elite Master",

			"Gold Nova I",
			"Gold Nova II",
			"Gold Nova III",
			"Gold Nova Master",
			"Master Guardian I",
			"Master Guardian II",

			"Master Guardian Elite",
			"Distinguished Master Guardian",
			"Legendary Eagle",
			"Legendary Eagle Master",
			"Supreme Master First Class",
			"Global Elite"
		};
		if (CompRank >= 0 && CompRank <= 18)
			Info.push_back(Ranks[CompRank]);

		if (IsFriendly(pEntity->GetIndex()))
		{
			Info.push_back("Friendly");
		}

		if (IsAimPrio(pEntity->GetIndex()))
		{
			Info.push_back("Priorety Target");
		}
	}

	static RECT Size = Render::GetTextSize(Render::Fonts::Default, "Hi");
	int i = 0;
	for (auto Text : Info)
	{
		Render::Text(size.x + size.w + 3, size.y + (i*(Size.bottom + 2)), Color(255, 255, 255, 255), Render::Fonts::ESP, Text.c_str());
		i++;
	}

}

// Little cross on their heads
void CEsp::DrawCross(IClientEntity* pEntity)
{
	Vector cross = pEntity->GetHeadPos(), screen;
	static int Scale = 2;
	if (Render::WorldToScreen(cross, screen))
	{
		Render::Clear(screen.x - Scale, screen.y - (Scale * 2), (Scale * 2), (Scale * 4), Color(20, 20, 20, 160));
		Render::Clear(screen.x - (Scale * 2), screen.y - Scale, (Scale * 4), (Scale * 2), Color(20, 20, 20, 160));
		Render::Clear(screen.x - Scale - 1, screen.y - (Scale * 2) - 1, (Scale * 2) - 2, (Scale * 4) - 2, Color(250, 250, 250, 160));
		Render::Clear(screen.x - (Scale * 2) - 1, screen.y - Scale - 1, (Scale * 4) - 2, (Scale * 2) - 2, Color(250, 250, 250, 160));
	}
}

// Draws a dropped CS:GO Item
void CEsp::DrawDrop(IClientEntity* pEntity, ClientClass* cClass)
{
	Vector Box;
	CBaseCombatWeapon* Weapon = (CBaseCombatWeapon*)pEntity;
	IClientEntity* plr = Interfaces::EntList->GetClientEntityFromHandle((HANDLE)Weapon->GetOwnerHandle());
	if (!plr && Render::WorldToScreen(Weapon->GetOrigin(), Box))
	{
		if (Menu::Window.VisualsTab.OptionsBox.GetState())
		{
			Render::Outline(Box.x - 2, Box.y - 2, 4, 4, Color(255, 255, 255, 255));
			Render::Outline(Box.x - 3, Box.y - 3, 6, 6, Color(10, 10, 10, 150));
		}

		if (Menu::Window.VisualsTab.OptionsInfo.GetState())
		{
			std::string ItemName = CleanItemName(cClass->m_pNetworkName);
			RECT TextSize = Render::GetTextSize(Render::Fonts::ESP, ItemName.c_str());
			Render::Text(Box.x - (TextSize.right / 2), Box.y - 16, Color(255, 255, 255, 255), Render::Fonts::ESP, ItemName.c_str());
		}
	}
}

// Draws a chicken
void CEsp::DrawChicken(IClientEntity* pEntity, ClientClass* cClass)
{
	ESPBox Box;

	if (GetBox(pEntity, Box))
	{
		player_info_t pinfo; strcpy_s(pinfo.name, "Chicken");
		if (Menu::Window.VisualsTab.OptionsBox.GetState())
			DrawBox(Box, Color(255, 255, 255, 255));

		if (Menu::Window.VisualsTab.OptionsName.GetState())
			DrawName(pinfo, Box);
	}
}

// Draw the planted bomb and timer
void CEsp::DrawBombPlanted(IClientEntity* pEntity, ClientClass* cClass)
{
	// Null it out incase bomb has been dropped or planted
	BombCarrier = nullptr;

	Vector vOrig; Vector vScreen;
	vOrig = pEntity->GetOrigin();
	CCSBomb* Bomb = (CCSBomb*)pEntity;

	if (Render::WorldToScreen(vOrig, vScreen))
	{
		float flBlow = Bomb->GetC4BlowTime();
		float TimeRemaining = flBlow - (Interfaces::Globals->interval_per_tick * HackManager.pLocal()->GetTickBase());
		char buffer[64];
		sprintf_s(buffer, "Bomb: %.1f", TimeRemaining);
		Render::Text(vScreen.x, vScreen.y, Color(250, 42, 42, 255), Render::Fonts::ESP, buffer);
	}
}

// Draw the bomb if it's dropped, or store the player who's carrying 
void CEsp::DrawBomb(IClientEntity* pEntity, ClientClass* cClass)
{
	// Null it out incase bomb has been dropped or planted
	BombCarrier = nullptr;
	CBaseCombatWeapon *BombWeapon = (CBaseCombatWeapon *)pEntity;
	Vector vOrig; Vector vScreen;
	vOrig = pEntity->GetOrigin();
	bool adopted = true;
	HANDLE parent = BombWeapon->GetOwnerHandle();
	if (parent || (vOrig.x == 0 && vOrig.y == 0 && vOrig.z == 0))
	{
		IClientEntity* pParentEnt = (Interfaces::EntList->GetClientEntityFromHandle(parent));
		if (pParentEnt && pParentEnt->IsAlive())
		{
			BombCarrier = pParentEnt;
			adopted = false;
		}
	}

	if (adopted)
	{
		if (Render::WorldToScreen(vOrig, vScreen))
		{
			Render::Text(vScreen.x, vScreen.y, Color(112, 230, 20, 255), Render::Fonts::ESP, "Bomb");
		}
	}
}

void DrawBoneArray(int* boneNumbers, int amount, IClientEntity* pEntity, Color color)
{
	Vector LastBoneScreen;
	for (int i = 0; i < amount; i++)
	{
		Vector Bone = pEntity->GetBonePos(boneNumbers[i]);
		Vector BoneScreen;

		if (Render::WorldToScreen(Bone, BoneScreen))
		{
			if (i>0)
			{
				Render::Line(LastBoneScreen.x, LastBoneScreen.y, BoneScreen.x, BoneScreen.y, color);
			}
		}
		LastBoneScreen = BoneScreen;
	}
}

void DrawBoneTest(IClientEntity *pEntity)
{
	for (int i = 0; i < 127; i++)
	{
		Vector BoneLoc = pEntity->GetBonePos(i);
		Vector BoneScreen;
		if (Render::WorldToScreen(BoneLoc, BoneScreen))
		{
			char buf[10];
			_itoa_s(i, buf, 10);
			Render::Text(BoneScreen.x, BoneScreen.y, Color(255, 255, 255, 180), Render::Fonts::ESP, buf);
		}
	}
}

void CEsp::DrawSkeleton(IClientEntity* pEntity)
{
	studiohdr_t* pStudioHdr = Interfaces::ModelInfo->GetStudiomodel(pEntity->GetModel());

	if (!pStudioHdr)
		return;

	Vector vParent, vChild, sParent, sChild;

	for (int j = 0; j < pStudioHdr->numbones; j++)
	{
		mstudiobone_t* pBone = pStudioHdr->GetBone(j);

		if (pBone && (pBone->flags & BONE_USED_BY_HITBOX) && (pBone->parent != -1))
		{
			vChild = pEntity->GetBonePos(j);
			vParent = pEntity->GetBonePos(pBone->parent);

			if (Render::WorldToScreen(vParent, sParent) && Render::WorldToScreen(vChild, sChild))
			{
				Render::Line(sParent[0], sParent[1], sChild[0], sChild[1], Color(255, 255, 255, 255));
			}
		}
	}
}