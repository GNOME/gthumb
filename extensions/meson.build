extension_data = configuration_data()
extension_data.set('GTHUMB_VERSION', meson.project_version())
extension_data.set('GTHUMB_API_VERSION', api_version)

extension_common_deps = [ common_deps, gthumb_h_dep ]

subdir('file_viewer')
subdir('catalogs')
subdir('edit_metadata')
subdir('exiv2_tools')
subdir('image_rotation')

# after exiv2
subdir('comments')

# jpeg tools
subdir('jpeg_utils')
subdir('cairo_io')
subdir('image_viewer')
subdir('importer')

# list tools
subdir('list_tools')
subdir('change_date')
subdir('convert_format')
subdir('rename_series')
subdir('resize_images')

# importers
subdir('photo_importer')

# exporters
subdir('export_tools')
if use_libbrasero
  subdir('burn_disc')
endif
subdir('contact_sheet')
if with_webalbums
  subdir('webalbums')
endif

# file tools
subdir('file_tools')
subdir('red_eye_removal')

# other tools
subdir('bookmarks')
subdir('file_manager')
if use_gstreamer
  subdir('gstreamer_utils')
  subdir('gstreamer_tools')
endif
subdir('image_print')
if use_libchamplain
  subdir('map_view')
endif
if use_libraw
  subdir('raw_files')
endif
subdir('search')
subdir('selections')
subdir('slideshow')
subdir('desktop_background')
subdir('find_duplicates')
subdir('terminal')
subdir('open_map')
