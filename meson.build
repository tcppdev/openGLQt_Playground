
#####################
# Build for Application
#####################

project('Qt', ['cpp', 'c'],  # Allow compiling for both c++ and c files
        default_options : ['cpp_std=c++17'])

# Dependencies for the gui
qt5_bin = import('qt5')
qt5_dep = dependency('qt5', 
                     modules: ['Core', 'Gui', 'Qml', 'Widgets',
                               'Quick', 'Location', 'Network', 
                               'Positioning', 'Test', 'QuickTest'], 
                     required : false)

if not qt5_dep.found()
  message('Did not find qt5 dependency')
  qt5_dep = disabler()
endif

assimp_dep = dependency('assimp')

# Getting eigen from wrap (but first tries locally)
eigen_dep = dependency('eigen', fallback : ['eigen', 'eigen_dep'])

# Freetype dependency for text
freetype_dep = subproject('freetype').get_variable('freetype_dep')

deps_common = [
  qt5_dep,
  assimp_dep,
  eigen_dep,
  freetype_dep
  ]

### Includes:

inc_common = [include_directories('lib')] #, include_directories('glad'), 
              #include_directories('KHR')]   # common
inc_underlay = [include_directories('underlay')]   # underlay
inc_fbo = [include_directories('fbo')]   # fbo
inc_render = [include_directories('rendercontrol')]   # render-control

inc_ext = [include_directories('ext_inc')]
inc_general = [include_directories('general_inc')]

## Header to moc
headers_to_moc_common = ['lib/camera.h',
                         'lib/meshrenderer.h']  
headers_to_moc_underlay = ['underlay/myquickview.h']
headers_to_moc_fbo = ['fbo/myframebufferobject.h']

## Resources
qresources_common = ['lib/assets.qrc']
qresources_underlay = ['underlay/underlay.qrc']
qresources_fbo = ['fbo/fbo.qrc']
qresources_render = ['rendercontrol/rendercontrol.qrc']

## Sources
sources_common = [
  'lib/axisalignedboundingbox.cpp',
  'lib/camera.cpp',
  'lib/meshrenderer.cpp',
  'lib/objloader.cpp'] 
 # 'glad/glad.c'   # ADDED
 # ]

sources_underlay = ['underlay/myquickview.cpp']
sources_fbo = ['fbo/myframebufferobject.cpp']
sources_render = ['rendercontrol/renderwindow.cpp']
sources_general = ['ext_inc/stb_image.cpp']

### BUILDING BINARIES

# Preprocess qt header and qml files to handle Qt's C++ extension
# using the meta-object compiler (moc) 

## Underlay binary
if get_option('underlay')
processed_moc_files_underlay = qt5_bin.preprocess(
    moc_headers: headers_to_moc_common + headers_to_moc_underlay,
    qresources: qresources_common + qresources_underlay,
    dependencies: qt5_dep
)

application_underlay = executable('underlay',
                        sources: ['underlay/main.cpp'] + sources_common + sources_underlay + processed_moc_files_underlay,
                        dependencies: deps_common,
                        include_directories: inc_common + inc_underlay,
                        link_args: ['-lstdc++fs'])
endif

## FBO binary
if get_option('fbo')
processed_moc_files_fbo = qt5_bin.preprocess(
    moc_headers: headers_to_moc_common + headers_to_moc_fbo,
    qresources: qresources_common + qresources_fbo,
    dependencies: qt5_dep
)

application_fbo = executable('fbo',
                        sources: ['fbo/main.cpp'] + sources_common + sources_fbo + processed_moc_files_fbo + sources_general,
                        dependencies: deps_common,
                        include_directories: inc_common + inc_fbo + inc_ext + inc_general,
                        link_args: ['-lstdc++fs'])
endif

## Render control binary
if get_option('render')
  processed_moc_files_render = qt5_bin.preprocess(
      moc_headers: headers_to_moc_common,
      qresources: qresources_common + qresources_render,
      dependencies: qt5_dep
  )

  application_render = executable('render-control',
                          sources: ['rendercontrol/main.cpp'] + sources_common + sources_render + processed_moc_files_render,
                          dependencies: deps_common,
                          include_directories: inc_common + inc_render,
                          link_args: ['-lstdc++fs'])
endif