import unreal
for p in ['/Game/__Verify/DI_VItemRarity.DI_VItemRarity', '/Game/GameData/DI_ItemRarity.DI_ItemRarity']:
    print('DIAG12a6 before-load', p, unreal.find_object(None, p) is not None)
bp = unreal.load_asset('/Game/__Verify/BP_VSelect')
print('DIAG12a6 loaded', bp is not None)
