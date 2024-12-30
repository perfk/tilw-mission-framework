class TILW_AOLimit : SCR_InfoDisplay
{
	protected override event void OnStartDraw(IEntity owner)
	{
		super.OnStartDraw(owner);
		
		Print("TILW_AOLimitMenu.OnMenuOpsden"); 
	}
	
	void SetTime(float time)
	{
		TextWidget timer = TextWidget.Cast(m_wRoot.FindAnyWidget("Timer"));
		string timeFormat = time.ToString(-1,3);
	
		timer.SetText(timeFormat);
		
		if(time < 10)
		{
			timer.SetColor(Color.Red);
		}
		else
		{
			timer.SetColor(Color.White);
		}
	}
}