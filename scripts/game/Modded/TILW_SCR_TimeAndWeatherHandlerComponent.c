modded class SCR_TimeAndWeatherHandlerComponent : SCR_BaseGameModeComponent
{
	[Attribute("0")]
	protected bool m_bUseSpecifiedDate;
	
	[Attribute("1", UIWidgets.Slider, "Specified day of month", "1 31 1")]
	protected int m_iSetDay;
	
	[Attribute("8", UIWidgets.Slider, "Specified month of year", "1 12 1")]
	protected int m_iSetMonth;
	
	[Attribute("1989", UIWidgets.Slider, "Specified year", "1901 2199 1")]
	protected int m_iSetYear;
	
	//------------------------------------------------------------------------------------------------
	override void OnWorldPostProcess(World world)
	{
		super.OnWorldPostProcess(world);

		if (!Replication.IsServer() || !GetGame().InPlayMode())
			return;

		if (s_Instance != this)
		{
			Print("Multiple instances of SCR_TimeAndWeatherHandlerComponent detected.", LogLevel.WARNING);
			return;
		}

		SetupDaytimeAndWeather(m_iStartingHours, m_iStartingMinutes);

		if (m_bUseSpecifiedDate)
			SetupDate(m_iSetDay, m_iSetMonth, m_iSetYear);
	}
	
	protected void SetupDate(int day, int month, int year)
	{
		ChimeraWorld world = ChimeraWorld.CastFrom(GetOwner().GetWorld());
		if (!world)
			return;
		
		TimeAndWeatherManagerEntity manager = world.GetTimeAndWeatherManager();
		manager.SetDate(year, month, day, true);
	}
}