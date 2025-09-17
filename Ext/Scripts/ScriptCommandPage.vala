public class Gth.ScriptCommandPage : Gth.TemplatePage {
	construct {
		title = _("Command");
		allowed_codes = new GenericList<TemplateCodeInfo> ();
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.TEXT, _("Text")));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SPACE, _("Space")));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.QUOTED, _("Quoted text"), Script.Code.QUOTE, 1));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File URI"), Script.Code.URI));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File path"), Script.Code.PATH));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File name"), Script.Code.BASENAME));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File name, no extension"), Script.Code.BASENAME_NO_EXTENSION));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File extension"), Script.Code.EXTENSION));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("Folder path"), Script.Code.PARENT_PATH));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.DATE, _("Current date"), Script.Code.TIMESTAMP, 1));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.ASK_VALUE, _("Ask a value"), Script.Code.ASK_VALUE, 2));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.FILE_ATTRIBUTE, _("File attribute"), Script.Code.FILE_ATTRIBUTE, 1));
	}
}
