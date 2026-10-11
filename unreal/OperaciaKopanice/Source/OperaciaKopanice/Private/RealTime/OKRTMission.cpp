#include "RealTime/OKRTMission.h"

const FOKRTMission& OKMissions::Get(int32 Id)
{
    static const FOKRTMission Missions[]={
        {0,TEXT("01 / Zimny viadukt"),TEXT("Myjavske kopanice / sabotaz"),
         TEXT("Ziskajte TNT, presunte oboch clenov cez most a aktivujte detonator z vychodneho brehu. Potom ustupte k vychodu. Hliadka sleduje most iba pocas kratkych zastavok."),
         EOKMissionGoal::Bridge,{300,1380,100},{540,1260,0},{1440,720,0},{1620,180,0},{540,1440,0},1980,
         {
             {{660,700,100},-90,EOKGuardDuty::Patrol,{{{660,700,0},-90,12},{{840,700,0},0,2},{{840,700,0},-90,1}}},
             {{1740,1200,100},90,EOKGuardDuty::Patrol,{{{1740,1200,0},90,8},{{1740,1020,0},180,2},{{1740,1020,0},0,1}}}
         }},
        {1,TEXT("02 / Lesny kurier"),TEXT("Biele Karpaty / prieskum"),
         TEXT("Preniknite do zasnezeneho tabora a ziskajte dokumenty zo zasobovacej bedne. Tim musi spolocne dosiahnut severovychodny vychod. Zapadny les poskytuje obchadzku strazenej cesty."),
         EOKMissionGoal::Documents,{1400,6200,100},{2100,2200,0},{2100,2200,0},{5200,900,0},{1700,5800,0},8640,
         {
             {{3500,2400,100},0,EOKGuardDuty::Patrol,{{{3500,2400,0},0,12},{{3680,2400,0},90,2},{{3680,2400,0},-90,1}}},
             {{2600,4400,100},0,EOKGuardDuty::Patrol,{{{2600,4400,0},0,12},{{2780,4400,0},90,2},{{2780,4400,0},-90,1}}},
             {{5800,5400,100},0,EOKGuardDuty::HoldPost,{{{5800,5400,0},0,12}}}
         }},
        {2,TEXT("03 / Tiche velitelstvo"),TEXT("Kopaniciarska osada / infiltracia"),
         TEXT("Ziskajte TNT pri juznom sklade. Priblizte sa k velitelskemu vozidlu zo zapadu a vyradte stanoviste. Hluk sabotaze prilaka hliadky; po akcii ustupte s oboma clenmi k severozapadnemu vychodu."),
         EOKMissionGoal::CommandPost,{1400,6600,100},{1700,5900,0},{4300,2600,0},{1400,1200,0},{1700,6300,0},8640,
         {
             {{4300,3900,100},0,EOKGuardDuty::Patrol,{{{4300,3900,0},0,12},{{4480,3900,0},90,2},{{4480,3900,0},-90,1}}},
             {{5700,2500,100},0,EOKGuardDuty::HoldPost,{{{5700,2500,0},0,12}}},
             {{3200,1400,100},0,EOKGuardDuty::Patrol,{{{3200,1400,0},0,12},{{3380,1400,0},90,2},{{3380,1400,0},-90,1}}}
         }}
    };
    return Missions[FMath::Clamp(Id,0,Count-1)];
}

EOKObjectiveStage OKMissions::Stage(const FOKRTMission& Mission,const FOKRTMissionState& State)
{
    if (State.bTargetComplete) return EOKObjectiveStage::Extraction;
    if (Mission.Goal==EOKMissionGoal::Documents) return EOKObjectiveStage::Target;
    if (!State.bSupplies) return EOKObjectiveStage::Supply;
    if (Mission.Goal==EOKMissionGoal::Bridge && (State.PartyCount==0 || State.SafeBankCount!=State.PartyCount))
        return EOKObjectiveStage::Crossing;
    return EOKObjectiveStage::Target;
}
bool OKMissions::CanExecute(const FOKRTMission& Mission,const FOKRTMissionState& State,EOKInteraction Interaction)
{
    if (State.PartyCount==0 || State.AliveCount!=State.PartyCount) return false;
    switch (Interaction)
    {
    case EOKInteraction::CollectTNT: return Mission.Goal!=EOKMissionGoal::Documents && !State.bSupplies;
    case EOKInteraction::CollectDocuments: return Mission.Goal==EOKMissionGoal::Documents && !State.bTargetComplete;
    case EOKInteraction::DetonateBridge: return Mission.Goal==EOKMissionGoal::Bridge && State.bSupplies &&
        !State.bTargetComplete && State.SafeBankCount==State.PartyCount;
    case EOKInteraction::SabotageCommandPost: return Mission.Goal==EOKMissionGoal::CommandPost && State.bSupplies && !State.bTargetComplete;
    default: return false;
    }
}
bool OKMissions::CanExtract(const FOKRTMissionState& State)
{
    return State.bTargetComplete && State.PartyCount>0 && State.AliveCount==State.PartyCount && State.ExitedCount==State.PartyCount;
}
