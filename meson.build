
#####################
# Build for Application
#####################

project('Qt', ['cpp', 'c'],  # Allow compiling for both c++ and c files
        default_options : ['cpp_std=c++17'])

# Dependencies for the gui
qt5_bin = import('qt5')
qt5_dep = dependency('qt5', 
                     modules: ['Core', 'Gui', 'Qml', 'Widgets',
                               'Quick', 'Location', 'Network', 'Charts',
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

cmake = import('cmake')

# Get Constrained Delaunay Triangulation library
cdt_dep = cmake.subproject('cdt')
inc_cdt = include_directories('subprojects/CDT/CDT/include')

deps_common = [
  qt5_dep,
  assimp_dep,
  eigen_dep,
  freetype_dep
  ]

inc_common = [include_directories('lib')] 
inc_underlay = [include_directories('underlay')]   # underlay
inc_fbo = [include_directories('fbo')]   # fbo
inc_render = [include_directories('rendercontrol')]   # render-control

inc_ext = [include_directories('ext_inc')]
inc_general = [include_directories('general_inc')]

## Header to moc
headers_to_moc_common = ['lib/camera.h',
                         'lib/meshrenderer.h']  
headers_to_moc_underlay = ['underlay/myquickview.h']
headers_to_moc_fbo = ['fbo/myframebufferobject.h',
                      'fbo/chartwrapper.h']

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

sources_underlay = ['underlay/myquickview.cpp']
sources_fbo = ['fbo/myframebufferobject.cpp', 'fbo/chartwrapper.cpp']
sources_render = ['rendercontrol/renderwindow.cpp']
sources_general = ['ext_inc/stb_image.cpp']

### BUILDING BINARIES

# Preprocess qt header and qml files to handle Qt's C++ extension
# using the meta-object compiler (moc) 

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
                        include_directories: inc_common + inc_fbo + inc_ext + inc_general + inc_cdt, #+ inc_delaunay + inc_earcut + inc_poly2tri,
                        link_args: ['-lstdc++fs'])
endif