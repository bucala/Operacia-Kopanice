#include "Demo/OKDemoRules.h"
#include <cassert>
#include <iostream>
#include <queue>
#include <set>
#include <string>
#include <tuple>

int main()
{
    using namespace OKDemo;
    FState initial;
    assert(!Apply(initial, EAction::Interact) && initial.Turn == 0);
    initial.Player = {0, 0};
    assert(!Apply(initial, EAction::North) && initial.Turn == 0);
    assert(IsBlocked({6,4}, true) && !IsBlocked({6,4}, false));
    initial.Player = {5,4};
    assert(Apply(initial, EAction::Wait) && initial.Outcome == EOutcome::Caught);
    assert(!Apply(initial, EAction::East));
    for (int32 L=0;L<LevelCount;++L)
    {
        auto State=InitialState(L);
        const auto Route=FindSolution(L);
        assert(!Route.IsEmpty() && Route.Num()<60);
        for (auto A : Route) assert(Apply(State,A));
        assert(State.Outcome==EOutcome::Won);
        std::cout << "PASS: " << GetLevel(L).Name << " (" << Route.Num() << " turns)\n";
    }

    using Key = std::tuple<int,int,int,bool,bool>;
    std::set<Key> visited;
    std::queue<std::pair<FState,std::string>> queue;
    queue.push({FState{}, ""});
    const char* names[] = {"N", "E", "S", "W", "Wait", "Use"};
    while (!queue.empty())
    {
        auto [state, path] = queue.front(); queue.pop();
        if (state.Outcome == EOutcome::Won)
        {
            assert(state.bTNT && state.bBridgeDestroyed);
            assert(state.Turn < 60);
            assert(!Apply(state, EAction::Wait));
            std::cout << "PASS: contracts and mission solvability (" << state.Turn << " turns)\n" << path << '\n';
            return 0;
        }
        if (state.Outcome == EOutcome::Caught) continue;
        if (!visited.insert({state.Player.X, state.Player.Y, state.Turn%4, state.bTNT, state.bBridgeDestroyed}).second) continue;
        for (int action=0; action<6; ++action)
        {
            FState next = state;
            if (Apply(next, static_cast<EAction>(action)))
                queue.push({next, path + names[action] + " "});
        }
    }
    std::cerr << "FAIL: mission has no winning route; searched " << visited.size() << " states\n";
    return 1;
}
