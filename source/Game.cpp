#include "Game.h"
#include "stl.h"
#include "GameStructures.h"
#include "GameDataLoad.h"
#include "ECS.h"
#include "Timer.h"
#include "EcsSystems.h"

#include "Renderer.h"
#include "GameLevelData.h"

namespace game
{

LevelData gLevelData;

static bool swordPattern[3 * 2] = {
    0, 0,
    1, 1,
    0, 0
};

static bool clubPattern[3 * 2] = {
    1, 0,
    0, 1,
    1, 0
};

stl::vector<Weapon> gGameWeapons;

#define UPDATE_PERIOD 0.2f

void Game::Init(const stl::shared_ptr<sf::RenderWindow> &window)
{
    this->window = window;
    gRenderer.Init(window);
    SendEvent(EventRendererWasInited());

    Weapon w;
    int id;

    w.areaSize = sf::Vector2u(3, 2);
    w.staminaConsumption = 4.0f;
    w.manaConsumption = 8.0f;
    w.magicDamage = 5.0f;
    w.physDamage = 10.0f;
    w.shift = 0;
    w.pattern.resize(sizeof(swordPattern) / sizeof(bool));
    for (unsigned i = 0; i < w.pattern.size(); ++i)
        w.pattern[i] = swordPattern[i];
    id = gRenderer.AddTexture("tileset/Sliced/items_16x16/oryx_16bit_fantasy_items_218.png");
    w.tileId = gRenderer.AddTile(id);
    gGameWeapons.push_back(w);

    w.staminaConsumption = 15.0f;
    w.manaConsumption = 0.0f;
    w.magicDamage = 0.0f;
    w.physDamage = 20.0f;
    w.pattern.resize(sizeof(clubPattern) / sizeof(bool));
    for (unsigned i = 0; i < w.pattern.size(); ++i)
        w.pattern[i] = clubPattern[i];
    id = gRenderer.AddTexture("tileset/Sliced/items_16x16/oryx_16bit_fantasy_items_190.png");
    w.tileId = gRenderer.AddTile(id);
    gGameWeapons.push_back(w);
}

void Game::Release()
{
    //window.reset();
    gRenderer.Release();
    gGameWeapons.clear();
    ecs::gEntityManager.DestroyAllEntities();
}

GameResults Game::StartLevel(const char *levelFilename)
{
    gLevelData = LoadLevel(levelFilename);

    

    ecs::Apply<CompOwnedItems, CompCurrentAttackArea>([&](ecs::IEntity *ent) {
        CompOwnedItems *ammo = ent->GetComp<CompOwnedItems>();
        ammo->weapons.push_back(&gGameWeapons[0]);
        ammo->weapons.push_back(&gGameWeapons[1]);
        ammo->curWeaponId = 0;
        CompCurrentAttackArea *area = ent->GetComp<CompCurrentAttackArea>();
    });

    gTime.Reset();
    volatile float lastTime = gTime.GetTime();
    while (window->isOpen())
    {
        gRenderer.BeginDraw(sf::Color(0x000000FFU));
        ecs::gEntityManager.UpdateEntities();
        gRenderer.EndDraw();
        while (lastTime + UPDATE_PERIOD > gTime.GetTime())
            ;
        lastTime = gTime.GetTime();
        stl::this_thread::yield();
    }
    return GameResults();
}

}