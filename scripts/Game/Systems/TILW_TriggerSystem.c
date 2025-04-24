class TILW_TriggerSystem : GameSystem
{
	static TILW_TriggerSystem GetInstance()
	{
		World world = GetGame().GetWorld();
		if (!world)
			return null;
		return TILW_TriggerSystem.Cast(world.FindSystem(TILW_TriggerSystem));
	}
	
	override void OnInit()
	{
		super.OnInit();
		if (m_triggers.IsEmpty())
			Enable(false);
	}
	
	override static void InitInfo(WorldSystemInfo outInfo)
	{
		super.InitInfo(outInfo);
		outInfo
			.SetAbstract(false)
			.SetUnique(true)
			.SetLocation(ESystemLocation.Server)
			.AddPoint(ESystemPoint.Frame);
	}
	
	// Triggers and characters
	
	protected ref array<TILW_BaseTriggerEntity> m_triggers = {};
	protected ref array<SCR_ChimeraCharacter> m_characters = {};
	
	void InsertTrigger(TILW_BaseTriggerEntity trigger)
	{
		if (!trigger)
			return;
		m_triggers.Insert(trigger);
		
		if (!IsEnabled())
			Enable(true);
	}
	void RemoveTrigger(TILW_BaseTriggerEntity trigger)
	{
		m_triggers.RemoveItem(trigger);
		
		if (m_triggers.IsEmpty())
			Enable(false);
	}
	
	void InsertCharacter(SCR_ChimeraCharacter character)
	{
		if (!character)
			return;
		m_characters.Insert(character);
	}
	void RemoveCharacter(SCR_ChimeraCharacter character)
	{
		int index = m_characters.Find(character);
		if (index == -1)
			return;
		
		m_characters.Remove(index);
		
		if (m_currentIndex > 0 && index < m_currentIndex && m_characters.Count() > index)
			ProcessCharacter(m_characters[index]); // Process swapback character
	}
	
	// OnUpdate
	
	protected int m_lastUpdate = 0;
	protected int m_updateFrequency = 1000;
	
	override protected void OnUpdate(ESystemPoint point)
	{
		super.OnUpdate(point);
		if (m_currentIndex > 0 || System.GetTickCount() - m_lastUpdate >= m_updateFrequency)
			ProcessCharacters();
	}
	
	protected int m_currentIndex = 0;
	protected int m_maxCharactersPerFrame = 50;
	
	// Why is this being done here and not directly on the triggers?
	protected void ProcessCharacters()
	{
		int count = m_characters.Count();
		int processedThisFrame = 0;
		
		while (m_currentIndex < count)
		{
			// Make sure to not process too much this frame
			if (processedThisFrame >= m_maxCharactersPerFrame)
				return;
			ProcessCharacter(m_characters[m_currentIndex]);
			
			m_currentIndex += 1;
			processedThisFrame += 1;
		}
		
		PostCharacterProcessing();
	}
	
	protected void ProcessCharacter(SCR_ChimeraCharacter cc)
	{
		foreach (TILW_BaseTriggerEntity t : m_triggers)
			t.ProcessCharacter(m_characters[m_currentIndex]);
	}
	
	protected void PostCharacterProcessing()
	{
		foreach (TILW_BaseTriggerEntity t : m_triggers)
			t.Eval();
		m_lastUpdate = System.GetTickCount();
		m_currentIndex = 0;
	}
	
	// Polyline WB visualiuation
	
}