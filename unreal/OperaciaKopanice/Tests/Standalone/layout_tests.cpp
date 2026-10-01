#include "Demo/OKObjectLayout.h"
#include <cassert>
#include <iostream>
#include <limits>

int main()
{
    using namespace OKLayout;
    const FIntPoint Expected[]={{4,4},{6,5},{8,5},{3,2},{5,2},{5,3},{6,3},{7,6},{18,11}};
    for (int32 I=0;I<9;++I)
    {
        assert(Footprint(EObject(I))==Expected[I]);
        for (const int32 Map : {LargeMapSize,ExtendedMapSize})
            for (int32 Rotation=-4;Rotation<=4;++Rotation)
            {
                FPlacement P{EObject(I),{0,0},Rotation};
                const auto S=P.Size();
                assert(S==FIntPoint(Rotation%2 ? Expected[I].Y : Expected[I].X,
                    Rotation%2 ? Expected[I].X : Expected[I].Y));
                P.Origin={Map-S.X,Map-S.Y};
                assert(P.Fits({Map,Map}));
                int32 Count=0;
                for (int32 Y=0;Y<Map;++Y)
                    for (int32 X=0;X<Map;++X) Count+=P.Contains({X,Y});
                assert(Count==S.X*S.Y);
                P.Origin.X++;
                assert(!P.Fits({Map,Map}));
                P.Origin={-1,0};
                assert(!P.Fits({Map,Map}));
            }
    }
    const FPlacement Cabin{EObject::Cabin,{1,0}};
    const FPlacement Touching{EObject::Car,{5,0}};
    const FPlacement Overlap{EObject::Car,{4,3}};
    assert(!Cabin.Overlaps(Touching) && !Touching.Overlaps(Cabin));
    assert(Cabin.Overlaps(Overlap) && Overlap.Overlaps(Cabin));
    assert(CanPlace(Touching,{48,48},{Cabin},{}));
    assert(!CanPlace(Overlap,{48,48},{Cabin},{}));
    assert(!CanPlace(Touching,{48,48},{Cabin},{{7,1}}));
    assert(!CanPlace({EObject(255),{0,0}},{48,48},{},{}));
    assert(!FPlacement({EObject::Cabin,{std::numeric_limits<int32>::max(),0}}).Fits({96,96}));
    assert(UniformScale(EObject::Car,100,200)==1.8);
    assert(UniformScale(EObject::Car,0,200)==0);
    assert(UniformScale(EObject::Car,std::numeric_limits<double>::infinity(),200)==0);
    assert(UniformScale(EObject::Car,std::numeric_limits<double>::quiet_NaN(),200)==0);
    std::cout << "PASS: nine footprints, cardinal rotations, 48/96 bounds, overlaps, reserved cells and uniform scale\n";
}
