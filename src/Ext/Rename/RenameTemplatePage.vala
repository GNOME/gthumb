public class Gth.RenameTemplatePage : Gth.TemplatePage {
	construct {
		title = _("Template");
		allowed_codes = new GenericList<TemplateCodeInfo> ();
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.TEXT, _("Text")));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SPACE, _("Space")));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.ENUMERATOR, _("Index")));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File Name"), RenameTemplate.Code.BASENAME));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File Name, No Extension"), RenameTemplate.Code.BASENAME_NO_EXTENSION));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("File Extension"), RenameTemplate.Code.EXTENSION));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.SIMPLE, _("Original Index"), RenameTemplate.Code.ORIGINAL_ENUMERATOR));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.DATE, _("Created"), RenameTemplate.Code.CREATION_TIME, 1));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.DATE, C_("Time", "Modified"), RenameTemplate.Code.MODIFICATION_TIME, 1));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.DATE, _("Digitalized"), RenameTemplate.Code.DIGITALIZATION_TIME, 1));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.DATE, _("Current Date"), RenameTemplate.Code.TIMESTAMP, 1));
		allowed_codes.model.append (new TemplateCodeInfo (TemplateCodeType.FILE_ATTRIBUTE, _("File Attribute"), RenameTemplate.Code.FILE_ATTRIBUTE, 1));

		get_preview_func = (text, flags, parent_code) => {
			var template = new RenameTemplate (text, flags | TemplateFlags.PREVIEW);
			return template.get_preview (parent_code);
		};
	}
}
