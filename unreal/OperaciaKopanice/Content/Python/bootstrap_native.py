"""Run with UE 5.8 Tools > Execute Python Script. Existing assets are preserved."""
import unreal as ue

ROOT = "/Game/Kopanice"
TOOLS = ue.AssetToolsHelpers.get_asset_tools()
MAT = ue.MaterialEditingLibrary


def asset(name, folder, cls, factory):
    path = ROOT + "/" + folder + "/" + name
    if ue.EditorAssetLibrary.does_asset_exist(path):
        return ue.load_asset(path), False
    result = TOOLS.create_asset(name, ROOT + "/" + folder, cls, factory)
    if not result:
        raise RuntimeError("Could not create " + path)
    return result, True


def save(obj):
    ue.EditorAssetLibrary.save_loaded_asset(obj)


def data_asset(name, cls):
    factory = ue.DataAssetFactory()
    factory.set_editor_property("data_asset_class", cls)
    return asset(name, "Data", cls, factory)


def expression(material, cls, **properties):
    node = MAT.create_material_expression(material, cls)
    for key, value in properties.items():
        node.set_editor_property(key, value)
    return node


def scalar(material, name, value):
    return expression(material, ue.MaterialExpressionScalarParameter, parameter_name=name, default_value=value)


def custom(material, code, inputs, output=ue.CustomMaterialOutputType.CMOT_FLOAT3):
    node = expression(material, ue.MaterialExpressionCustom, code=code, output_type=output,
                      inputs=[ue.CustomInput(input_name=name) for name in inputs])
    for name, source in inputs.items():
        if not MAT.connect_material_expressions(source, "", node, name):
            raise RuntimeError("Could not connect custom input " + name)
    return node


def output(node, prop):
    if not MAT.connect_material_property(node, "", prop):
        raise RuntimeError("Could not connect material output " + str(prop))


def finish(material):
    MAT.layout_material_expressions(material)
    MAT.recompile_material(material)
    save(material)


def make_materials():
    body, new = asset("M_OK_Vehicle_Weathering", "Materials", ue.Material, ue.MaterialFactoryNew())
    if new:
        p = expression(body, ue.MaterialExpressionWorldPosition)
        origin = expression(body, ue.MaterialExpressionObjectPositionWS)
        normal = expression(body, ue.MaterialExpressionVertexNormalWS)
        mud = scalar(body, "MudAmount", .18)
        snow = scalar(body, "RoofSnowAmount", .24)
        wear = scalar(body, "PaintWearAmount", .12)
        scalar(body, "FrostAmount", .32)
        scalar(body, "UseHeightSnowMask", 1)
        color = custom(body, """
float3 q = P - O;
float n = saturate(.5 + .25*sin(q.x*.23)*sin(q.y*.17) + .25*sin(q.z*.41));
float m = saturate((35-q.z)/90) * Mud * (.45+.55*n);
float s = smoothstep(.45,.85,N.z) * Snow;
float w = smoothstep(.7,.95,n)*Wear;
float3 paint = lerp(float3(.13,.16,.10),float3(.23,.22,.20),w);
return lerp(lerp(paint,float3(.09,.06,.035),m),float3(.82,.87,.90),s);
""", {"P": p, "O": origin, "N": normal, "Mud": mud, "Snow": snow, "Wear": wear})
        output(color, ue.MaterialProperty.MP_BASE_COLOR)
        output(scalar(body, "Roughness", .68), ue.MaterialProperty.MP_ROUGHNESS)
        output(scalar(body, "Metallic", .25), ue.MaterialProperty.MP_METALLIC)
        wpo = custom(body, "return N * smoothstep(.45,.85,N.z) * Snow * 2.0;", {"N": normal, "Snow": snow})
        output(wpo, ue.MaterialProperty.MP_WORLD_POSITION_OFFSET)
        finish(body)

    glass, new = asset("M_OK_Glass_Frost", "Materials", ue.Material, ue.MaterialFactoryNew())
    if new:
        glass.set_editor_property("blend_mode", ue.BlendMode.BLEND_TRANSLUCENT)
        frost = scalar(glass, "FrostAmount", .32)
        uv = expression(glass, ue.MaterialExpressionTextureCoordinate)
        mask = custom(glass, """
float edge = pow(saturate(abs(UV.x-.5)*2),5) + pow(saturate(abs(UV.y-.5)*2),5);
return saturate(edge * Frost + Frost*.1);
""", {"UV": uv, "Frost": frost}, ue.CustomMaterialOutputType.CMOT_FLOAT1)
        color = custom(glass, "return lerp(float3(.18,.25,.27),float3(.78,.85,.88),Mask);", {"Mask": mask})
        output(color, ue.MaterialProperty.MP_BASE_COLOR)
        output(custom(glass, "return .18+Mask*.5;", {"Mask": mask}, ue.CustomMaterialOutputType.CMOT_FLOAT1), ue.MaterialProperty.MP_OPACITY)
        output(custom(glass, "return .08+Mask*.6;", {"Mask": mask}, ue.CustomMaterialOutputType.CMOT_FLOAT1), ue.MaterialProperty.MP_ROUGHNESS)
        finish(glass)

    landscape, new = asset("M_OK_Landscape", "Materials", ue.Material, ue.MaterialFactoryNew())
    if new:
        layers = []
        for name, color in [("Paved_Roads_Interiors", (.19,.20,.20)),
                            ("Autumn_Mud", (.10,.065,.04)),
                            ("Deep_Winter_Snow", (.82,.87,.90))]:
            layers.append(ue.LayerBlendInput(layer_name=name, blend_type=ue.LandscapeLayerBlendType.LB_WEIGHT_BLEND,
                                            const_layer_input=ue.Vector(*color), preview_weight=1/3))
        blend = expression(landscape, ue.MaterialExpressionLandscapeLayerBlend, layers=layers)
        output(blend, ue.MaterialProperty.MP_BASE_COLOR)
        output(scalar(landscape, "Roughness", .8), ue.MaterialProperty.MP_ROUGHNESS)
        finish(landscape)

    stamp, new = asset("M_OK_FootprintStamp", "Materials", ue.Material, ue.MaterialFactoryNew())
    if new:
        stamp.set_editor_property("shading_model", ue.MaterialShadingModel.MSM_UNLIT)
        stamp.set_editor_property("blend_mode", ue.BlendMode.BLEND_TRANSLUCENT)
        uv = expression(stamp, ue.MaterialExpressionTextureCoordinate)
        mask = custom(stamp, "float2 p=(UV-.5)*2; return 1-smoothstep(.65,1,dot(p,p));",
                      {"UV": uv}, ue.CustomMaterialOutputType.CMOT_FLOAT1)
        output(mask, ue.MaterialProperty.MP_OPACITY)
        output(scalar(stamp, "Depth", 1), ue.MaterialProperty.MP_EMISSIVE_COLOR)
        finish(stamp)

    concrete, new = asset("M_OK_Concrete", "Materials", ue.Material, ue.MaterialFactoryNew())
    if new:
        p = expression(concrete, ue.MaterialExpressionWorldPosition)
        color = custom(concrete, "float g=.5+.5*sin(P.x*3)*sin(P.y*3)*sin(P.z*3); return lerp(float3(.22,.23,.22),float3(.38,.39,.37),g);", {"P": p})
        output(color, ue.MaterialProperty.MP_BASE_COLOR)
        output(scalar(concrete, "Roughness", .9), ue.MaterialProperty.MP_ROUGHNESS)
        finish(concrete)
    return body, glass, stamp, concrete


def blueprint(name, parent, defaults=None):
    factory = ue.BlueprintFactory()
    factory.set_editor_property("parent_class", parent)
    bp, fresh = asset(name, "Blueprints", ue.Blueprint, factory)
    if fresh:
        ue.BlueprintEditorLibrary.compile_blueprint(bp)
        cls = ue.EditorAssetLibrary.load_blueprint_class(ROOT + "/Blueprints/" + name)
        cdo = ue.get_default_object(cls)
        for key, value in (defaults or {}).items():
            cdo.set_editor_property(key, value)
        save(bp)
    return bp


def main():
    surface, fresh = data_asset("DA_OK_Terrain", ue.OKTerrainSurfaceDataAsset)
    entries = list(surface.get_editor_property("surfaces"))
    for index, (name, friction, surface_type) in enumerate([
        ("Paved", 1.0, ue.PhysicalSurface.SURFACE_TYPE1),
        ("Mud", .6, ue.PhysicalSurface.SURFACE_TYPE2),
        ("Snow", .3, ue.PhysicalSurface.SURFACE_TYPE3)
    ]):
        physical, new = asset("PM_OK_" + name, "Terrain", ue.PhysicalMaterial, ue.PhysicalMaterialFactoryNew())
        if new:
            physical.set_editor_property("friction", friction)
            physical.set_editor_property("surface_type", surface_type)
            save(physical)
        info, new = asset("LI_OK_" + name, "Terrain", ue.LandscapeLayerInfoObject, ue.LandscapeLayerInfoObjectFactory())
        if new:
            info.set_editor_property("layer_name", entries[index].get_editor_property("landscape_layer_name"))
            info.set_editor_property("phys_material", physical)
            info.set_editor_property("no_weight_blend", False)
            save(info)
        if fresh:
            entries[index].set_editor_property("physical_material", physical)
    if fresh:
        surface.set_editor_property("surfaces", entries)
        save(surface)
    rules, _ = data_asset("DA_OK_PCG_Rules", ue.OKPCGScatterRuleSet)
    save(rules)
    body, glass, stamp, concrete = make_materials()
    blueprint("BP_OK_Drevenica", ue.OKModularBuildingTemplateActor)
    blueprint("BP_OK_Stonework", ue.OKModularBuildingTemplateActor,
              {"style": ue.OKModularBuildingStyle.CARPATHIAN_STONEWORK})
    blueprint("BP_OK_ConcretePlatform", ue.OKConcreteTilePlatformActor, {"concrete_material": concrete})
    for name, kind in [("Kubelwagen_Typ82", ue.OKHistoricalVehicleType.KUBELWAGEN_TYP82),
                       ("OpelBlitz_3T", ue.OKHistoricalVehicleType.OPEL_BLITZ3_T),
                       ("Tatra_T77A_1938", ue.OKHistoricalVehicleType.TATRA_T77_A1938)]:
        blueprint("BP_OK_" + name, ue.OKHistoricalVehicleActor,
                  {"vehicle_type": kind, "body_material": body, "glass_material": glass})
    blueprint("BP_OK_EnemyController", ue.OKEnemyAIController)
    blueprint("BP_OK_TacticalCharacter", ue.OKTacticalCharacter)
    blueprint("BP_OK_Viaduct", ue.OKViaductActor)
    target, fresh = asset("RT_OK_Footprints", "Terrain", ue.TextureRenderTarget2D, ue.TextureRenderTargetFactoryNew())
    if fresh:
        target.set_editor_property("size_x", 2048)
        target.set_editor_property("size_y", 2048)
        target.set_editor_property("render_target_format", ue.TextureRenderTargetFormat.RTF_RGBA8)
        save(target)
    blueprint("BP_OK_FootprintField", ue.OKFootprintFieldActor, {"depth_target": target, "stamp_material": stamp})
    for index, name in enumerate(["Beech", "Limestone"]):
        graph, fresh = asset("PCG_OK_" + name, "PCG", ue.PCGGraph, ue.PCGGraphFactory())
        if fresh:
            sampler, settings = graph.add_node_of_type(ue.PCGSurfaceSamplerSettings)
            settings.set_editor_property("points_per_squared_meter", [0.062, 0.009][index])
            rule, settings = graph.add_node_of_type(ue.PCGBlueprintSettings)
            element = settings.set_blueprint_element_type(ue.OKPCGScatterElement)
            element.set_editor_property("rule_set", rules)
            element.set_editor_property("rule_index", index)
            graph.add_edge(graph.get_input_node(), "In", sampler, "Surface")
            graph.add_edge(sampler, "Out", rule, "In")
            graph.add_edge(rule, "Out", graph.get_output_node(), "Out")
            save(graph)
    ue.log("KOPANICE_BOOTSTRAP_OK: assets created; mesh selection and landscape setup: Docs/Native_Authoring.md")


if __name__ == "__main__":
    main()
