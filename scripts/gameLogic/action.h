//
// Created by User on 14.12.2025.
//

#ifndef PROJECT_NAME_ACTION_H
#define PROJECT_NAME_ACTION_H

enum class ActionType
{
    Attack,
    Magic,
    Item,
    Skip
};

enum class TargetType
{
    Enemy,
    Ally,
    Self,
    AllEnemies,
    AllAllies
};

struct Action
{
    int actorIndex {-1};     // индекс героя
    uint32_t playerId = 0;
    ActionType type {ActionType::Skip};
    TargetType targetType {TargetType::Enemy};
    int targetIndex {-1};    // -1 если Self / AoE
    int payloadId {-1};      // skillId / itemId, -1 если не используется

    Action() = default;

    Action(
        int actor,
        ActionType t,
        TargetType tt,
        int target = -1,
        int payload = -1
    )
        : actorIndex(actor),
          type(t),
          targetType(tt),
          targetIndex(target),
          payloadId(payload)
    {}
};

#endif //PROJECT_NAME_ACTION_H