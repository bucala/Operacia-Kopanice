#include "Demo/OKDemoRules.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOKObjectLayout,"OperaciaKopanice.Demo.ObjectLayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOKObjectLayout::RunTest(const FString& Parameters)
{
    using namespace OKLayout;
    const FIntPoint Expected[]={{4,4},{6,5},{8,5},{3,2},{5,2},{5,3},{6,3},{7,6},{18,11}};
    for (int32 I=0;I<9;++I)
    {
        TestTrue(TEXT("Approved footprint"),Footprint(EObject(I))==Expected[I]);
        for (int32 Map : {LargeMapSize,ExtendedMapSize})
            for (int32 Rotation=-4;Rotation<=4;++Rotation)
            {
                FPlacement P{EObject(I),{0,0},Rotation};
                const auto S=P.Size();
                TestTrue(TEXT("Rotated footprint"),S==FIntPoint(Rotation%2 ? Expected[I].Y : Expected[I].X,
                    Rotation%2 ? Expected[I].X : Expected[I].Y));
                P.Origin={Map-S.X,Map-S.Y};
                TestTrue(TEXT("Fits exact boundary"),P.Fits({Map,Map}));
                TestTrue(TEXT("Last occupied cell"),P.Contains({Map-1,Map-1}));
                TestFalse(TEXT("Exclusive boundary"),P.Contains({Map,Map}));
                ++P.Origin.X;
                TestFalse(TEXT("Reject out of bounds"),P.Fits({Map,Map}));
            }
    }
    const FPlacement Cabin{EObject::Cabin,{1,0}},Car{EObject::Car,{5,0}};
    TestTrue(TEXT("Adjacent placement allowed"),CanPlace(Car,{48,48},{Cabin},{}));
    TestFalse(TEXT("Overlap rejected"),CanPlace({EObject::Car,{4,3}},{48,48},{Cabin},{}));
    TestFalse(TEXT("Objective reserved"),CanPlace(Car,{48,48},{Cabin},{{7,1}}));
    TestEqual(TEXT("Preserve proportions"),UniformScale(EObject::Car,100,200),1.8);
    TestEqual(TEXT("Reject empty bounds"),UniformScale(EObject::Car,0,200),0.0);
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOKDemoSolvable,"OperaciaKopanice.Demo.AllMissionsSolvable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOKDemoSolvable::RunTest(const FString& Parameters)
{
    for (int32 Level=0;Level<OKDemo::LevelCount;++Level)
    {
        auto State=OKDemo::InitialState(Level);
        const auto& Spec=OKDemo::GetLevel(Level);
        for (FIntPoint Cell : {Spec.Start,Spec.TNT,Spec.Detonator,Spec.Exit})
            TestFalse(TEXT("Objective on walkable ground"),OKDemo::IsBlocked(Cell,false,Level));
        TestFalse(TEXT("Safe spawn"),OKDemo::IsThreatened(State.Player,State));
        auto Route=OKDemo::FindSolution(Level);
        TestTrue(TEXT("Solution found under 60 turns"),!Route.IsEmpty() && Route.Num()<60);
        for (auto Action : Route)
        {
            TestTrue(TEXT("Route action accepted"),OKDemo::Apply(State,Action));
            TestFalse(TEXT("Player never enters a blocked cell"),OKDemo::IsBlocked(State.Player,State.bBridgeDestroyed,Level));
            TestTrue(TEXT("Player survives route"),State.Outcome!=OKDemo::EOutcome::Caught);
        }
        TestTrue(TEXT("Sabotage and extraction complete"),State.bTNT && State.bBridgeDestroyed && State.Outcome==OKDemo::EOutcome::Won);
        const int32 Turn=State.Turn;
        TestFalse(TEXT("Finished mission freezes"),OKDemo::Apply(State,OKDemo::EAction::Wait));
        TestEqual(TEXT("Finished turn retained"),State.Turn,Turn);
        AddInfo(FString::Printf(TEXT("%s: %d turns"),Spec.Name,Route.Num()));
    }
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FOKDemoContracts,"OperaciaKopanice.Demo.ActionContracts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FOKDemoContracts::RunTest(const FString& Parameters)
{
    OKDemo::FState State;
    TestFalse(TEXT("Invalid interaction"),OKDemo::Apply(State,OKDemo::EAction::Interact));
    TestEqual(TEXT("Invalid action costs no turn"),State.Turn,0);
    for (int32 X=1;X<=4;++X)
        for (int32 Y=0;Y<4;++Y)
            TestTrue(TEXT("Complete 4x4 cabin footprint blocked"),OKDemo::IsBlocked({X,Y},false));
    State.Player={0,1};
    TestFalse(TEXT("Cannot enter cabin"),OKDemo::Apply(State,OKDemo::EAction::East));
    for (int32 Level=0;Level<OKDemo::LevelCount;++Level)
    {
        const int32 Row=OKDemo::GetLevel(Level).BridgeRow;
        TestFalse(TEXT("Intact crossing"),OKDemo::IsBlocked({6,Row},false,Level));
        TestTrue(TEXT("Destroyed crossing"),OKDemo::IsBlocked({6,Row},true,Level));
        TestTrue(TEXT("River impassable"),OKDemo::IsBlocked({6,(Row+1)%9},false,Level));
    }
    State.Player={5,4};
    TestTrue(TEXT("Wait resolves"),OKDemo::Apply(State,OKDemo::EAction::Wait));
    TestTrue(TEXT("Guard turns and detects player"),State.Outcome==OKDemo::EOutcome::Caught);
    return true;
}
#endif
