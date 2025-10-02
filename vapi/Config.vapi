[CCode (cheader_filename = "config.h", cprefix = "")]
class Config {
	public const string APP_NAME;
	public const string APP_ID;
	public const string DOMAIN;
	public const string GETTEXT_PACKAGE;
	public const string G_LOG_DOMAIN;
	public const string LOCALEDIR;
	public const string VERSION;
	public const string PRIVEXECDIR;
	public const bool DEVELOPER_MODE;
	public const bool FLATPAK;

	[CCode (cheader_filename = "version.h")]
	public const string APP_VERSION;
}
