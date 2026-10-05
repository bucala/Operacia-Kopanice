"""Refine existing terrain materials without reimporting terrain or textures."""
import unreal as ue
lib=ue.MaterialEditingLibrary
root='/Game/Kopanice/WinterGround/'
# Shared authored road centre lines, in world centimetres, match build_winter_ground.py.
segments=[(-1300,1260,540,1260),(540,1260,790,760),(790,760,1450,720),
          (1450,720,1620,180),(1620,180,2800,-400),(540,1260,400,780),
          (400,780,300,450),(1450,720,1660,1260),(1660,1260,2500,1600),
          (900,450,900,990),(1260,450,1260,990)]
road='float d=100000;\n'
for ax,ay,bx,by in segments:
    road+='{{ float2 a=float2({0},{1}), v=float2({2},{3}); d=min(d,length(P.xy-a-v*saturate(dot(P.xy-a,v)/dot(v,v)))); }}\n'.format(ax,ay,bx-ax,by-ay)

snow='''float2 q=P.xy*.004; float2 i=floor(q), f=frac(q); f=f*f*(3-2*f);
float4 h=frac(sin(float4(dot(i,float2(127.1,311.7)),dot(i+float2(1,0),float2(127.1,311.7)),dot(i+float2(0,1),float2(127.1,311.7)),dot(i+1,float2(127.1,311.7))))*43758.5453);
float n=lerp(lerp(h.x,h.y,f.x),lerp(h.z,h.w,f.x),f.y);
float3 snow=lerp(float3(.62,.71,.79),float3(.86,.89,.92),.52+n*.48);
'''
codes={
'Wood':snow+'''
float grain=.72+.25*sin(P.x*.12+sin(P.y*.011)*2)*sin(P.x*.34+P.y*.004)+.09*sin(P.y*.043);
float dust=smoothstep(.68,.94,n+.08*sin(P.x*.021+P.y*.035));
return lerp(float3(.19,.14,.10)*grain,snow,dust*.75);
''',
'Stone':road+snow+'''
float irregular=9*sin(P.x*.041+sin(P.y*.033))+5*sin(P.y*.071);
float edge=smoothstep(85,160,d+irregular);
float stone=.72+.32*frac(sin(dot(floor(P.xy/38),float2(127.1,311.7)))*43758.5453);
return lerp(float3(.23,.26,.27)*stone,snow,edge);
''',
'Snow':road+snow+'''
float river=abs(P.x-(1080+saturate(max(-90-P.y,P.y-1530)/600)*230*sin(P.y*.0025)));
float wet=(1-smoothstep(78,120,river))*.32;
float track=(1-smoothstep(75,160,d))*.15;
return lerp(snow,float3(.31,.40,.44),saturate(wet+track));
''',
'Water':'''float2 q=float2(P.x,P.y-T*22);
float centre=1080+saturate(max(-90-P.y,P.y-1530)/600)*230*sin(P.y*.0025);
float shore=smoothstep(54,91,abs(P.x-centre)+2*sin(P.y*.12));
float a=sin(q.y*.09+sin(q.x*.061)+sin(q.y*.037+q.x*.019));
float b=sin(q.y*.19+q.x*.075+sin(q.x*.12-q.y*.034));
float foam=pow(saturate(a*.45+b*.22+.25),7);
float3 deep=float3(.025,.065,.075)+float3(.016,.03,.032)*(a*.5+.5);
return lerp(deep,float3(.21,.32,.34),shore*.65)+foam*(.025+shore*.10);
'''}
for name,code in codes.items():
    mat=ue.load_asset(root+'M_'+name)
    assert mat
    node=lib.get_material_property_input_node(mat,ue.MaterialProperty.MP_BASE_COLOR)
    assert isinstance(node,ue.MaterialExpressionCustom),name
    node.set_editor_property('code',code)
    lib.recompile_material(mat)
    ue.EditorAssetLibrary.save_loaded_asset(mat,only_if_is_dirty=False)
    ue.log('OK_SURFACE_REFINED '+name)
