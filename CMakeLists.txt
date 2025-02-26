## automatically generated CMakeLists.txt file

# Declare the package
atlas_subdir( xflow )

# Declare external dependencies ... default here is to include ROOT
find_package( ROOT COMPONENTS MathCore RIO Core Tree Hist )

# Declare package as a library
# Note the convention that library names get "Lib" suffix
# Any package you depend on you should add
# to LINK_LIBRARIES line below (see the example)
atlas_add_library( xflowlib src/*.cxx
                   PUBLIC_HEADERS xflow
                   INCLUDE_DIRS ${ROOT_INCLUDE_DIRS}
                   LINK_LIBRARIES ${ROOT_LIBRARIES}
                                    queryosity::queryosity
                                    AthAnalysisBaseCompsLib
                                    CaloEvent xAODEventInfo xAODTracking xAODEventShape xAODCaloEvent CaloUtilsLib
)

# if you add athena components (tools, algorithms) to this package
# these lines are needed so you can configure them in joboptions
atlas_add_component( xflow src/components/*.cxx
                      NOCLIDDB
                      LINK_LIBRARIES queryosity::queryosity xflowlib 
)

# if you add an application (exe) to this package
# declare it like this (note convention that apps go in the util dir)
# atlas_add_executable( MyApp util/myApp.cxx
#                       LINK_LIBRARIES xflowlib
# )

# Install python modules, joboptions, and share content
#atlas_install_python_modules( python/*.py )
#atlas_install_joboptions( share/*.py )
#atlas_install_data( data/* )
atlas_install_scripts( scripts/* )
# You can access your data from code using path resolver, e.g.
# PathResolverFindCalibFile("xflow/file.txt")