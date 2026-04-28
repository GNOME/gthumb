config = configuration_data()
config.set_quoted('GETTEXT_PACKAGE', gettext_package)
config.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
config.set_quoted('DOMAIN', meson.project_name ())
config.set_quoted('G_LOG_DOMAIN', log_domain)
config.set_quoted('APP_NAME', 'Thumbnails')
app_version = meson.project_version()
if get_option('developer-mode') and get_option('flatpak-build')
  app_version = app_version + '.flatpak'
endif
config.set_quoted('APP_VERSION', app_version)
config.set_quoted('PREFIX', get_option('prefix'))
config.set('DEVELOPER_MODE', get_option('developer-mode') ? 'true' : 'false')
if get_option('developer-mode')
  add_project_arguments([
    '-D', 'DEVELOPER_MODE',
    '-D', 'RUN_IN_PLACE',
  ], language: 'vala')
  config.set_quoted('PRIVEXECDIR', join_paths(meson.project_build_root() + '/src'))
  config.set_quoted('APP_ID', 'org.gnome.gthumb-devel')
  exec_name = 'gthumb-devel'
else
  config.set_quoted('PRIVEXECDIR', privexecdir)
  config.set_quoted('APP_ID', 'org.gnome.gthumb')
  exec_name = 'gthumb'
endif
config.set('FLATPAK', get_option('flatpak-build') ? 'true' : 'false')
if have_progressive_jpeg
  config.set('HAVE_PROGRESSIVE_JPEG', 1)
endif
if use_colord
  config.set('HAVE_COLORD', 1)
endif
if use_libwebp
  config.set('HAVE_LIBWEBP', 1)
  add_project_arguments(['-D', 'HAVE_LIBWEBP'], language: 'vala')
endif
if use_librsvg
  config.set('HAVE_LIBRSVG', 1)
  add_project_arguments(['-D', 'HAVE_LIBRSVG'], language: 'vala')
endif
if use_libheif
  config.set('HAVE_LIBHEIF', 1)
  add_project_arguments(['-D', 'HAVE_LIBHEIF'], language: 'vala')
endif
if use_libjxl
  config.set('HAVE_LIBJXL', 1)
  add_project_arguments(['-D', 'HAVE_LIBJXL'], language: 'vala')
endif
if use_libtiff
  config.set('HAVE_LIBTIFF', 1)
  add_project_arguments(['-D', 'HAVE_LIBTIFF'], language: 'vala')
endif
if use_libgif
  config.set('HAVE_LIBGIF', 1)
  add_project_arguments(['-D', 'HAVE_LIBGIF'], language: 'vala')
endif
if use_libraw
  config.set('HAVE_LIBRAW', 1)
  add_project_arguments(['-D', 'HAVE_LIBRAW'], language: 'vala')
endif

config_file = configure_file(
  output : 'config.h',
  configuration : config
)

resources_files = gnome.compile_resources('resources',
  'App.gresource.xml',
  c_name: 'resources',
  source_dir: [
    '../data/icons/private/',
    '../data/icons/private/actions',
    '../data/icons/',
    '../data/css',
    '../data/ui',
  ]
)

lib_files = files(
  'lib/exiv2-utils.cpp',
  'lib/gstreamer-utils.c',
  'lib/gth-color-manager.c',
  'lib/gth-curve.c',
  'lib/gth-histogram.c',
  'lib/gth-icc-profile.c',
  'lib/gth-image-blur.c',
  'lib/gth-image-dither.c',
  'lib/gth-image.c',
  'lib/gth-image-cut.c',
  'lib/gth-image-edit.c',
  'lib/gth-image-histogram.c',
  'lib/gth-image-resize.c',
  'lib/gth-image-rotate.c',
  'lib/gth-image-transform.c',
  'lib/gth-metadata.c',
  'lib/gth-option.c',
  'lib/gth-point.c',
  'lib/gth-points.c',
  'lib/gth-string-list.c',
  'lib/io/image-info.c',
  'lib/io/load-jpeg.c',
  'lib/io/load-png.c',
  'lib/io/save-jpeg.c',
  'lib/io/save-png.c',
  'lib/io/svg-info.c',
  'lib/jpeg/jmemorydest.c',
  'lib/jpeg/jmemorysrc.c',
  'lib/jpeg/jpeg-info.c',
  'lib/pixel.c',
  'lib/util.c',
  'lib/zlib-utils.c',
)
if use_libwebp
  lib_files += files('lib/io/load-webp.c', 'lib/io/save-webp.c')
endif
if use_librsvg
  lib_files += files('lib/io/load-svg.c')
endif
if use_libjxl
  lib_files += files('lib/io/load-jxl.c', 'lib/io/save-jxl.c')
endif
if use_libtiff
  lib_files += files('lib/io/load-tiff.c', 'lib/io/save-tiff.c')
endif
if use_libheif
  lib_files += files('lib/io/load-heif.c', 'lib/io/save-avif.c')
endif
if use_libgif
  lib_files += files('lib/io/load-gif.c')
endif
if use_libraw
  lib_files += files('lib/io/load-raw.cpp')
endif

vapi_files = files(
  'vapi/ColorManager.vapi',
  'vapi/Compat.vapi',
  'vapi/Config.vapi',
  'vapi/Curve.vapi',
  'vapi/Exiv2.vapi',
  'vapi/Histogram.vapi',
  'vapi/IccProfile.vapi',
  'vapi/IccTransform.vapi',
  'vapi/Image.vapi',
  'vapi/Lib.vapi',
  'vapi/Loaders.vapi',
  'vapi/Metadata.vapi',
  'vapi/Option.vapi',
  'vapi/Point.vapi',
  'vapi/Points.vapi',
  'vapi/Savers.vapi',
  'vapi/StringList.vapi',
  'vapi/Types.vapi',
  'vapi/Video.vapi',
  'vapi/Zip.vapi',
)

subdir('Ext')
subdir('Dialogs')

dependencies = [
  dependency('threads'),
  c_comp.find_library('m', required: false),
  vala_comp.find_library('posix'),
  dependency('glib-2.0', version: '>= @0@'.format(glib_version)),
  dependency('gobject-2.0'),
  dependency('gtk4', version: gtk_version),
  dependency('libadwaita-1', version: libadwaita_version),
  dependency('libpng', version: libpng_version),
  dependency('lcms2', version: lcms2_version),
  dependency('exiv2', version: exiv2_version),
  libjpeg_dep,
  gstreamer_dep_list,
  use_colord ? colord_dep : [],
  use_libwebp ? libwebp_dep : [],
  use_librsvg ? librsvg_dep : [],
  use_libheif ? libheif_dep : [],
  use_libjxl ? libjxl_deps : [],
  use_libtiff ? libtiff_deps : [],
  use_libgif ? libgif_dep : [],
  use_libraw? libraw_dep : [],
]

executable(exec_name,
  sources: [
    'ActionList.vala',
    'ActionPopover.vala',
    'Application.vala',
    'Browser.vala',
    'DateRow.vala',
    'DateTime.vala',
    'DefaultSorters.vala',
    'DefaultTests.vala',
    'DestinationRow.vala',
    'Dom.vala',
    'Editor.vala',
    'Events.vala',
    'Factory.vala',
    'FileData.vala',
    'FileGridItem.vala',
    'FileGrid.vala',
    'FilePropertyView.vala',
    'FileSet.vala',
    'FileSource.vala',
    'Files.vala',
    'FileViewer.vala',
    'FolderRow.vala',
    'FolderStatus.vala',
    'FolderTreeItem.vala',
    'FolderTree.vala',
    'GenericList.vala',
    'History.vala',
    'ImageEditor.vala',
    'ImageLoader.vala',
    'ImageOperation.vala',
    'ImageSaver.vala',
    'JobStatus.vala',
    'Job.vala',
    'Main.vala',
    'MainWindow.vala',
    'Migration.vala',
    'Misc.vala',
    'MonitorProfile.vala',
    'PointUtil.vala',
    'ProgressDialog.vala',
    'PropertySidebar.vala',
    'PropertyView.vala',
    'RatingEntry.vala',
    'RatingRow.vala',
    'RegisteredSignals.vala',
    'SelectionView.vala',
    'Settings.vala',
    'SidebarResizer.vala',
    'Status.vala',
    'Strings.vala',
    'TagEntry.vala',
    'TagsRow.vala',
    'ThumbLoader.vala',
    'Thumbnailer.vala',
    'Thumbnail.vala',
    'TimeRow.vala',
    'TimeSelector.vala',
    'TreeIterator.vala',
    'Types.vala',
    'UnknownViewer.vala',
    'UserDir.vala',
    'Util.vala',
    'ViewerStatus.vala',
    'Viewer.vala',
    'WindowState.vala',
    'Window.vala',
    bookmarks_files,
    catalogs_files,
    color_files,
    comments_files,
    config_file,
    desktop_background_files,
    dialogs_files,
    editor_files,
    exiv2_files,
    file_manager_files,
    filters_files,
    image_files,
    import_files,
    lib_files,
    metadata_files,
    preferences_files,
    preloader_files,
    print_files,
    rename_files,
    resources_files,
    savers_files,
    scripts_files,
    search_files,
    selections_files,
    shortcuts_files,
    slideshow_files,
    template_files,
    tools_files,
    vapi_files,
    video_files,
  ],
  dependencies: [
    dependencies,
    libportal_deps,
  ],
  install: true,
)

executable('video-thumbnailer',
  sources: [
    'Ext/Video/Util.vala',
    'Ext/Video/VideoThumbnailer.vala',
    'Files.vala',
    'Strings.vala',
    config_file,
    lib_files,
    vapi_files,
  ],
  dependencies: dependencies,
  install: true,
  install_dir: privexecdir,
)

if get_option('developer-mode')
  test('dom',
    executable('test-dom',
      sources: [
        'Dom.vala',
        'Files.vala',
        'Strings.vala',
        'Tests/TestDom.vala',
      ],
      dependencies: dependencies,
      vala_args: '--define=TEST_XML',
    )
  )

  test('util',
    executable('test-util',
      sources: [
        'Strings.vala',
        'Util.vala',
        'Tests/TestUtil.vala',
      ],
      dependencies: dependencies,
    )
  )

  test('strings',
    executable('test-strings',
      sources: [
        'Strings.vala',
        'Tests/TestStrings.vala',
      ],
      dependencies: dependencies,
    )
  )

  test('template',
    executable('test-template',
      sources: [
        'Strings.vala',
        'Ext/Template/Template.vala',
        'Tests/TestTemplate.vala',
      ],
      dependencies: dependencies,
    )
  )
endif
