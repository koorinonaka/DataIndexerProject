import unreal
EAL = unreal.EditorAssetLibrary
SRC_REPO = '/Game/GameData/DI_ItemRarity'
REPO = '/Game/__Verify/DI_VItemRarity'
BP = '/Game/__Verify/BP_VSelect'
if not EAL.does_asset_exist(REPO):
    EAL.duplicate_asset(SRC_REPO, REPO)
if not EAL.does_asset_exist(BP):
    EAL.duplicate_asset('/Game/UI/VM_InventoryRowCard', BP)
repo = unreal.load_asset(REPO)
bp = unreal.load_asset(BP)
src = unreal.load_asset(SRC_REPO)
n = 0
for node in unreal.ObjectIterator(unreal.K2Node):
    if node.get_path_name().startswith(BP + '.') and node.get_class().get_name() == 'K2Node_DataIndexerSelectPrimaryKey':
        cur = node.get_editor_property('Repository')
        print('DIAG12a6 node', node.get_path_name(), cur.get_path_name() if cur else None)
        if cur == src:
            node.set_editor_property('Repository', repo)
            n += 1
print('DIAG12a6 retargeted', n)
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
print('DIAG12a6 saved', EAL.save_asset(BP, only_if_is_dirty=False), EAL.save_asset(REPO, only_if_is_dirty=False))
