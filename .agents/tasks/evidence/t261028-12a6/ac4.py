import unreal
bp = unreal.load_asset('/Game/__Verify/BP_VSelect')
other = unreal.load_asset('/Game/GameData/DI_ItemType')
n = 0
for node in unreal.ObjectIterator(unreal.K2Node):
    if node.get_path_name().startswith('/Game/__Verify/BP_VSelect.') and node.get_class().get_name() == 'K2Node_DataIndexerSelectPrimaryKey':
        cur = node.get_editor_property('Repository')
        if cur and cur.get_name() == 'DI_VItemRarity':
            node.set_editor_property('Repository', other)
            n += 1
print('DIAG12a6 ac4 retargeted', n)
print('DIAG12a6 ac4 compile start')
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
print('DIAG12a6 ac4 compile end')
