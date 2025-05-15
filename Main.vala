int main (string[] args) {
	Gtk.init ();
	var gtk_settings = Gtk.Settings.get_default ();
	gtk_settings.gtk_application_prefer_dark_theme = true;
	//Adw.init ();
	return 0;
}
