#ifdef WORKBENCH
[WorkbenchToolAttribute(
    name: "Prefab Picker",
    wbModules: { "WorldEditor" },
    awesomeFontCode: 0xF002,
    description: "- Ctrl+Click prefab to add to prefab list\n- Ctrl+Shift+Click to remove\n- right click prefab list and select copy. This can now be pasted into other prefab arrays"
)]
class PK_PrefabPickerTool : WorldEditorTool
{
    [Attribute(desc: "List of prefab resources.")]
    protected ref array<ResourceName> m_prefabList;

    void PK_PrefabPickerTool()
    {
        m_prefabList = new array<ResourceName>();
    }

    override void OnMousePressEvent(float x, float y, WETMouseButtonFlag buttons)
    {
        if (buttons != WETMouseButtonFlag.LEFT)
            return;

        bool ctrl = GetModifierKeyState(ModifierKey.CONTROL);
        bool shift = GetModifierKeyState(ModifierKey.SHIFT);

        if (!ctrl)
            return; // Only act on Ctrl+Click

        IEntity entity;
        vector start, end, normal;
        m_API.TraceWorldPos(x, y, TraceFlags.ENTS | TraceFlags.WORLD, start, end, normal, entity);

        if (!entity)
            return;

        IEntitySource entitySource = m_API.EntityToSource(entity);
        if (!entitySource)
            return;

        BaseContainer ancestor = entitySource.GetAncestor();
        if (!ancestor)
            return;

        ResourceName prefabName = ancestor.GetResourceName();
        if (prefabName.IsEmpty())
            return;

        int idx = m_prefabList.Find(prefabName);

        if (shift)
        {
            // Ctrl+Shift+Click: Remove from list
            if (idx > -1)
            {
                m_prefabList.RemoveOrdered(idx);
                Print(string.Format("Removed %1 from prefab list.", prefabName), LogLevel.NORMAL);
            }
            else
            {
                Print(string.Format("%1 not found in prefab list.", prefabName), LogLevel.NORMAL);
            }
        }
        else
        {
            // Ctrl+Click: Add to list
            if (idx == -1)
            {
                m_prefabList.Insert(prefabName);
                Print(string.Format("Added %1 to prefab list.", prefabName), LogLevel.NORMAL);
            }
            else
            {
                Print(string.Format("%1 already in prefab list.", prefabName), LogLevel.NORMAL);
            }
        }

        WorldEditorTool.UpdatePropertyPanel();
    }
};
#endif
