[ComponentEditorProps(category: "GC/Component", description: "I protect")]
class PK_EntityProtectorComponentClass : ScriptComponentClass
{
}

class PK_EntityProtectorComponent : ScriptComponent
{
	[Attribute("1", UIWidgets.Auto, "Keep Entity after Prefab Deleter", category: "Prefab Deleter")]
	protected bool m_keepEntityAfterDelete;
}
