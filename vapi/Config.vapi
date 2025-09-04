[CCode (cheader_filename = "config.h", cprefix = "")]
class Config {
	public const string APP;
	public const string DOMAIN;
	public const string GETTEXT_PACKAGE;
	public const string G_LOG_DOMAIN;
	public const string LOCALEDIR;
	public const string VERSION;
	public const string PRIVEXECDIR;

	[CCode (cheader_filename = "version.h")]
	public const string APP_VERSION;
}
