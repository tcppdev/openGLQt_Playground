
#####################
# Build for Application
#####################

project('Qt', ['cpp', 'c'],  # Allow compiling for both c++ and c files
        default_options : ['cpp_std=c++17'])

cmake = import('cmake')

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
  qt5_dep,
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
processed_moc_files_fbo = qt5_bin.preprocess(
    moc_headers: headers_to_moc_fbo,
    qresources: qresources_fbo,
    dependencies: qt5_dep
)

# Now boost and filesystem magic for our different compilers:
if get_option('msys2')
  # This case is when we're using msys2 to build on windows:
  boost_dep = dependency('boost', modules : ['thread', 'filesystem', 'coroutine', 'context'])
  link_args = ['-Wl,-subsystem,windows','-lws2_32']
else
  # regular g++ linux:
  boost_dep = dependency('boost', modules : ['thread', 'system', 'coroutine', 'context'])
  link_args = ['-lstdc++fs']
endif

deps_common += boost_dep

application_fbo = executable('application',
                        sources: ['fbo/main.cpp'] + sources_fbo + processed_moc_files_fbo + sources_general,
                        dependencies: deps_common,
                        include_directories:  inc_fbo + inc_ext + inc_general + inc_cdt,
                        link_args: link_args)
endif