
#####################
# Build for Application
#####################

project('Qt', ['cpp', 'c'],  # Allow compiling for both c++ and c files
        default_options : ['cpp_std=c++17'])

cmake = import('cmake')

# Dependencies for the gui
qt6_core = dependency('Qt6Core', required : true)
qt6_gui = dependency('Qt6Gui', required : true)
qt6_widgets = dependency('Qt6Widgets', required : true)
qt6_qml = dependency('Qt6Qml', required : true)
qt6_quick = dependency('Qt6Quick', required : true)
qt6_network = dependency('Qt6Network', required : true)
qt6_charts = dependency('Qt6Charts', required : true)
qt6_positioning = dependency('Qt6Positioning', required : true)
qt6_test = dependency('Qt6Test', required : true)

qt6_found = qt6_core.found() and qt6_gui.found() and qt6_widgets.found() and qt6_qml.found() and qt6_quick.found() and qt6_charts.found() and qt6_network.found() and qt6_positioning.found() and qt6_test.found()

if not qt6_found
  message('Did not find qt6 dependencies')
  qt6_dep = disabler()
else
  qt6_dep = [qt6_core, qt6_gui, qt6_widgets, qt6_qml, qt6_quick, qt6_network, qt6_charts, qt6_positioning, qt6_test]
endif

assimp_dep = dependency('assimp')

# Getting eigen from wrap (but first tries locally)
if get_option('msys2')
  eigen_dep = dependency('eigen3')
else
  eigen_dep = dependency('eigen', fallback : ['eigen', 'eigen_dep'])
endif

# Freetype dependency for text
freetype_dep = subproject('freetype').get_variable('freetype_dep')

cmake = import('cmake')

# Get Constrained Delaunay Triangulation library
cdt_dep = cmake.subproject('cdt')
inc_cdt = include_directories('subprojects/CDT/CDT/include')

deps_common = [
  qt6_dep,
  assimp_dep,
  eigen_dep,
  freetype_dep
  ]

inc_fbo = [include_directories('fbo')]   # fbo

inc_ext = [include_directories('ext_inc')]
inc_general = [include_directories('general_inc')]

## Header to moc
headers_to_moc_fbo = ['fbo/myframebufferobject.h',
                      'fbo/chartwrapper.h']

## Resources
qresources_fbo = ['fbo/fbo.qrc']

## Sources
sources_fbo = ['fbo/myframebufferobject.cpp', 'fbo/chartwrapper.cpp']
sources_general = ['ext_inc/stb_image.cpp']

### BUILDING BINARIES

# Preprocess qt header and qml files to handle Qt's C++ extension
# using the meta-object compiler (moc) 

## FBO binary
if get_option('fbo')
qt6_tools = import('qt6')
processed_moc_files_fbo = qt6_tools.preprocess(
    moc_headers: headers_to_moc_fbo,
    qresources: qresources_fbo,
    dependencies: qt6_core
)

# Now boost and filesystem magic for our different compilers:
if get_option('msys2')
  # This case is when we're using msys2 to build on windows:
  boost_dep = dependency('boost', modules : ['thread', 'filesystem', 'coroutine', 'context'])
  link_args = ['-Wl,-subsystem,windows','-lws2_32']
else
  # regular g++ linux:
  boost_dep = dependency('boost', modules : ['thread', 'system', 'coroutine', 'context'])
  link_args = ['-lstdc++fs', '-lpng']
endif

deps_common += boost_dep

application_fbo = executable('application',
                        sources: ['fbo/main.cpp'] + sources_fbo + processed_moc_files_fbo + sources_general,
                        dependencies: deps_common,
                        include_directories:  inc_fbo + inc_ext + inc_general + inc_cdt,
                        link_args: link_args)
endif
