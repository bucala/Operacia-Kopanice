#include "RealTime/OKRTMission.h"

const FOKRTMission& OKMissions::Get(int32 Id)
{
    static const FOKRTMission Missions[]={
        {0,TEXT("01 / Zimny viadukt"),TEXT("Myjavske kopanice / sabotaz"),
         TEXT("Ziskajte TNT, presunte oboch clenov cez most a aktivujte detonator z vychodneho brehu. Potom ustupte k vychodu. Hliadka sleduje most iba pocas kratkych zastavok."),
         EOKMissionGoal::Bridge,{300,1380,100},{540,1260,0},{1440,720,0},{1620,180,0},{540,1440,0},1980},
        {1,TEXT("02 / Lesny kurier"),TEXT("Biele Karpaty / prieskum"),
         TEXT("Preniknite do zasnezeneho tabora a ziskajte dokumenty zo zasobovacej bedne. Tim musi spolocne dosiahnut severovychodny vychod. Zapadny les poskytuje obchadzku strazenej cesty."),
         EOKMissionGoal::Documents,{1400,6200,100},{2100,2200,0},{2100,2200,0},{5200,900,0},{1700,5800,0},8640},
        {2,TEXT("03 / Tiche velitelstvo"),TEXT("Kopaniciarska osada / infiltracia"),
         TEXT("Ziskajte TNT pri juznom sklade. Priblizte sa k velitelskemu vozidlu zo zapadu a vyradte stanoviste. Hluk sabotaze prilaka hliadky; po akcii ustupte s oboma clenmi k severozapadnemu vychodu."),
         EOKMissionGoal::CommandPost,{1400,6600,100},{1700,5900,0},{4300,2600,0},{1400,1200,0},{1700,6300,0},8640}
    };
    return Missions[FMath::Clamp(Id,0,Count-1)];
}
