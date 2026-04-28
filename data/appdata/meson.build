metainfo_i18n = i18n.merge_file(
  input: 'org.gnome.gthumb.metainfo.xml.in',
  output: 'org.gnome.gthumb.metainfo.xml',
  type: 'xml',
  po_dir: po_dir,
  install: true,
  install_dir: join_paths(datadir, 'metainfo'),
)

appstreamcli = find_program('appstreamcli', required: false)

if appstreamcli.found()
  test('as-validate-metainfo',
    appstreamcli,
    args: [
      'validate',
      '--pedantic',
      '--no-net',
      metainfo_i18n
    ]
  )
endif
