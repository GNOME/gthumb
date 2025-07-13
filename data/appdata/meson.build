# Required version 0.14.6 or newer.
ascli = find_program('appstreamcli')

# NOTE: Release notes are not translated on purpose.
# This target should not be run when building from tarball, see metainfo-releases-tarball.sh script.
metainfo_with_relinfo = custom_target('gen-output',
  input : ['../../NEWS', 'org.gnome.gThumb.metainfo.xml.in'],
  output : ['nol10n_withrelinfo_org.gnome.gThumb.metainfo.xml'],
  command : [ascli, 'news-to-metainfo', '--limit=6', '@INPUT0@', '@INPUT1@', '@OUTPUT@']
)

if get_option('tarball-appdata') == true
  metainfo_i18n = i18n.merge_file(
    input : 'org.gnome.gThumb.metainfo.xml.in',
    output : 'org.gnome.gThumb.metainfo.xml',
    type : 'xml',
    po_dir : po_dir,
    install : true,
    install_dir : join_paths(datadir, 'metainfo')
  )
else
  metainfo_i18n = i18n.merge_file(
    input : metainfo_with_relinfo,
    output : 'org.gnome.gThumb.metainfo.xml',
    type : 'xml',
    po_dir : po_dir,
    install : true,
    install_dir : join_paths(datadir, 'metainfo')
  )
endif

test('as-validate-metainfo',
  ascli,
  args: ['validate',
         '--pedantic',
         '--no-net',
         metainfo_i18n]
)

meson.add_dist_script('metainfo-releases-tarball.sh')
