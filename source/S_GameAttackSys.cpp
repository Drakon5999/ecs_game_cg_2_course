#include "EcsSystems.h"
#include "stl.h"
static const sf::Vector2i cellSize = sf::Vector2i(40, 40);
static const sf::Vector2i vpCells = sf::Vector2i(21, 15);
static const sf::Vector2i vpCellsPos = sf::Vector2i(1, 1);
static const sf::Vector2i wndCells = sf::Vector2i(30, 17);
static const sf::Vector2i wSize = sf::Vector2i(cellSize.x * wndCells.x, cellSize.y * wndCells.y);
static const sf::Vector2i vpPos = sf::Vector2i(cellSize.x * vpCellsPos.x, cellSize.y * vpCellsPos.y);
static const sf::Vector2i vpSize = sf::Vector2i(cellSize.x * vpCells.x, cellSize.y * vpCells.y);
static const sf::Vector2i curWeaponPos = sf::Vector2i(vpPos.x + vpSize.x + cellSize.x * 1, vpPos.y + cellSize.y * 4);
static const sf::Vector2i ownedItemsPos = sf::Vector2i(vpPos.x + vpSize.x + cellSize.x * 1, vpPos.y + cellSize.y * 6);
static const sf::Vector2i healthPos = sf::Vector2i(vpPos.x + vpSize.x + cellSize.x * 5, vpPos.y + cellSize.y * 0);
static const sf::Vector2i staminaPos = sf::Vector2i(vpPos.x + vpSize.x + cellSize.x * 5, vpPos.y + cellSize.y * 1);
static const sf::Vector2i manaPos = sf::Vector2i(vpPos.x + vpSize.x + cellSize.x * 5, vpPos.y + cellSize.y * 2);
static const int itemsBarWidth = wndCells.x - vpCells.x - vpCellsPos.x - 1;
static const int itemsBarHeight = 4;

void GameAttackSys::OnUpdate()
{
    ecs::Apply<CompPosPattern, CompActivity, CompDamage, CompOwner>([&](ecs::IEntity *area)
    {
        CompActivity *act = area->GetComp<CompActivity>();
        if (!act->active)
            return;
        if (act->disableAfterOneTick)
            act->active = false;
        CompPosPattern *positions = area->GetComp<CompPosPattern>();
        ecs::IEntity *owner = ecs::gEntityManager.GetEntityByDesc(area->GetComp<CompOwner>()->owner);

        ecs::Apply<CompPos, CompHealth, CompArmor>([&](ecs::IEntity *creature)
        {
            CompParty *creatParty = creature->GetComp<CompParty>();
            CompParty *ownerParty = owner->GetComp<CompParty>();
            if (creatParty && ownerParty && creatParty->party == ownerParty->party)
                return;

            CompPos *pos = creature->GetComp<CompPos>();
            if (positions->positions.IsPosExists(pos->v.x, pos->v.y))
            {
                CompDamage *damage = area->GetComp<CompDamage>();
                CompArmor *resist = creature->GetComp<CompArmor>();
                float physResist = 0.0f;
                float magicResist = 0.0f;
                if (resist)
                {
                    physResist = resist->physResist;
                    magicResist = resist->magicResist;
                }
                CompHealth *h = creature->GetComp<CompHealth>();
                float damageValue = stl::max(damage->physDamage * (1.0f - physResist), 0.0f)
                                  + stl::max(damage->magicDamage * (1.0f - magicResist), 0.0f);

                h->value -= damageValue;
                stl::stringstream damageName;
                damageName << '-' << damageValue;
                auto viewCorner = game::gRenderer.GetViewCorner();
                auto screenCords = pos->v - viewCorner;
                ecs::gEntityManager.CreateEntity<Message>(
                        sf::Vector2i((screenCords.x + 3) * cellSize.x, vpSize.y - (screenCords.y + 2) * cellSize.y),
                        1/*ttl*/, 0xFF0000FFU/*color*/, 22/*size*/, 2/*layer*/, damageName.str());
                if (h->value <= 0.0f)
                    ecs::gEntityManager.DestroyEntity(creature->GetDesc());

            }
        });
    });
}