class RainbowColor
{
private:
	float v;

public:
	Color GetColor()
	{
		if (v >= 1) v = 0; else v = v + (Options::Misc::SpeedControl / 1000.f)/*0.001f*/;
		return Color().FromHSB(v, 1, 1);
	}
};