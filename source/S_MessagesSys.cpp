#include "EcsSystems.h"
#include "Renderer.h"
#include "stl.h"

void GameMessagesSys::OnUpdate()
{
    ecs::Apply<CompMessage, CompPos, CompTimeToLive, CompName, CompActivity>([&](ecs::IEntity *message) {
      auto *textData = message->GetComp<CompMessage>();
      auto *pos = message->GetComp<CompPos>();
      auto *ttl = message->GetComp<CompTimeToLive>();
      auto *name = message->GetComp<CompName>();
      auto *activity = message->GetComp<CompActivity>();

      if (ttl->ticks > 0) {
          if (!activity->active) {
              activity->active = true;
              name->value = game::gRenderer.GuiAddText(textData->layer, pos->v,
                      textData->text.c_str(), textData->size, textData->color);
          }
          ttl->ticks -= 1;
      } else {
          ecs::gEntityManager.DestroyEntity(message->GetDesc());
      }
    });
}