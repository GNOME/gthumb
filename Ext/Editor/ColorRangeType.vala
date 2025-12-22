public enum Gth.ColorRangeType {
	HUE,
	BRIGHTNESS,
	SATURATION,
	RED,
	GREEN,
	BLUE,
	ALPHA,
	COLOR,
	PREVIEW,
	CYAN_RED,
	MAGENTA_GREEN,
	YELLOW_BLUE,
	BLACK_WHITE,
	WHITE_BLACK,
	GRAY_WHITE;

	public bool is_rgb () {
		return (this == RED) || (this == GREEN) || (this == BLUE);
	}

	public bool is_hsv () {
		return (this == HUE) || (this == SATURATION) || (this == BRIGHTNESS);
	}
}
