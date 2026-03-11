public class Gth.ScriptCommandPage : Gth.TemplatePage {
	construct {
		title = _("Command");
		allowed_codes = new GenericList<TemplateCodeInfo> ();
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.TEXT, _("Text")));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SPACE, _("Space")));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.QUOTED, _("Quoted Text"), ScriptTemplate.Code.QUOTE, 1));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File URI"), ScriptTemplate.Code.URI));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File Path"), ScriptTemplate.Code.PATH));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File Name"), ScriptTemplate.Code.BASENAME));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File Name, No Extension"), ScriptTemplate.Code.BASENAME_NO_EXTENSION));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File Extension"), ScriptTemplate.Code.EXTENSION));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("Folder Path"), ScriptTemplate.Code.PARENT_PATH));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.DATE, _("Current Date"), ScriptTemplate.Code.TIMESTAMP, 1));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.ASK_VALUE, _("Ask a Value"), ScriptTemplate.Code.ASK_VALUE, 2));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.FILE_ATTRIBUTE, _("File Attribute"), ScriptTemplate.Code.FILE_ATTRIBUTE, 1));

		get_preview_func = (text, flags, parent_code) => {
			var template = new ScriptTemplate (text, flags | TemplateFlags.PREVIEW);
			return template.eval (parent_code);
		};
	}
}
