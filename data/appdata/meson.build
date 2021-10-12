# Required version 0.14.6 or newer.
ascli = find_program('appstreamcli')

# NOTE: Release notes are not translated on purpose.
metainfo_with_relinfo = custom_target('gen-output',
  input : ['../../NEWS', 'org.gnome.gThumb.appdata.xml.in'],
  output : ['nol10n_withrelinfo_org.gnome.gThumb.appdata.xml'],
  command : [ascli, 'news-to-metainfo', '--limit=6', '@INPUT0@', '@INPUT1@', '@OUTPUT@']
)

metainfo_i18n = i18n.merge_file(
  input : metainfo_with_relinfo,
  output : 'org.gnome.gThumb.appdata.xml',
  type : 'xml',
  po_dir : po_dir,
  install : true,
  install_dir : join_paths(datadir, 'metainfo')
)

test('as-validate-metainfo',
  ascli,
  args: ['validate',
         '--pedantic',
         '--no-net',
         metainfo_i18n]
)
