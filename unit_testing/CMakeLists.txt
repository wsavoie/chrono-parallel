#--------------------------------------------------------------
# Additional include paths (for ChronoEngine_Utils)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}
                    ${CMAKE_BINARY_DIR}
)

# Libraries
SET(LIBRARIES
    ${CHRONO_LIB}
    ChronoEngine_Parallel
    ChronoEngine_Utils
)

#--------------------------------------------------------------
# Always use full RPATH (differentiating between the build and install trees)

# use, i.e. don't skip the full RPATH for the build tree
SET(CMAKE_SKIP_BUILD_RPATH  FALSE)

# when building, don't use the install RPATH already
# (but later on when installing)
SET(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE) 

SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

# add the automatically determined parts of the RPATH
# which point to directories outside the build tree to the install RPATH
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# the RPATH to be used when installing, but only if it's not a system directory
LIST(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
IF("${isSystemDir}" STREQUAL "-1")
   SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
ENDIF("${isSystemDir}" STREQUAL "-1")

#--------------------------------------------------------------

IF("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
ENDIF()

#IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#	SET (CH_LINKERFLAG_EXE  "${CH_LINKERFLAG_EXE} -framework IOKit -framework Cocoa -framework OpenGL")
#ENDIF()

#--------------------------------------------------------------
# List of all executables

SET(TESTS
    test_real3
    test_real4
    test_mat33
    test_gravity
    test_rhs
    test_r
    test_apgd
    test_shur_performance
    test_shafts
    
)

MESSAGE(STATUS "Unit test programs...")

FOREACH(PROGRAM ${TESTS})
    MESSAGE(STATUS "...add ${PROGRAM}")

    ADD_EXECUTABLE(${PROGRAM}  "${PROGRAM}.cpp")
    SOURCE_GROUP(""  FILES "${PROGRAM}.cpp")

    SET_TARGET_PROPERTIES(${PROGRAM} PROPERTIES
        FOLDER demos
        COMPILE_FLAGS "${CH_BUILDFLAGS}"
        LINK_FLAGS "${CH_LINKERFLAG_EXE}"
    )

    TARGET_LINK_LIBRARIES(${PROGRAM} ${LIBRARIES})
    ADD_DEPENDENCIES(${PROGRAM} ${LIBRARIES})

    INSTALL(TARGETS ${PROGRAM} DESTINATION bin)
    ADD_TEST(${PROGRAM} ${PROJECT_BINARY_DIR}/bin/${PROGRAM})

ENDFOREACH(PROGRAM)

 #CUDA_ADD_EXECUTABLE(test_cusparse  test_cusparse.cu test_matvec.cpp)
 #SOURCE_GROUP(""  FILES "test_cusparse.cu test_matvec.cpp")
 #SET_TARGET_PROPERTIES(test_cusparse PROPERTIES
 #       FOLDER demos
 #       COMPILE_FLAGS "${CH_BUILDFLAGS}"
 #       LINK_FLAGS "${CH_LINKERFLAG_EXE}"
 #)
 #   TARGET_LINK_LIBRARIES(test_cusparse ${LIBRARIES} ${CUDA_cusparse_LIBRARY})
 #   ADD_DEPENDENCIES(test_cusparse ${LIBRARIES})
 #   INSTALL(TARGETS test_cusparse DESTINATION bin)
 
#--------------------------------------------------------------
# Executables that use Bullet

SET(TESTS_B
    test_mpr
)

# Because Bullet classes are not DLL exported, the sources must be explicitly
# included.
IF (MSVC)
  SET(BULLET_FILES
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btCollisionShape.h
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btCollisionShape.cpp
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btConvexInternalShape.h
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btConvexInternalShape.cpp
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btConvexShape.h
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btConvexShape.cpp
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btPolyhedralConvexShape.h
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btPolyhedralConvexShape.cpp
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btBoxShape.h
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btBoxShape.cpp
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btConeShape.h
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btConeShape.cpp
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btCylinderShape.h
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btCylinderShape.cpp
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btSphereShape.h
        ${CH_CHRONO_SDKDIR}/collision/bullet/BulletCollision/CollisionShapes/btSphereShape.cpp
        ${CH_CHRONO_SDKDIR}/collision/bullet/LinearMath/btAlignedAllocator.h
        ${CH_CHRONO_SDKDIR}/collision/bullet/LinearMath/btAlignedAllocator.cpp
  )
  SOURCE_GROUP(bullet FILES ${BULLET_FILES})
ELSE()
    SET(BULLET_FILES "")
ENDIF()

FOREACH(PROGRAM ${TESTS_B})
    MESSAGE(STATUS "...add ${PROGRAM}")

    ADD_EXECUTABLE(${PROGRAM}  ${PROGRAM}.cpp ${BULLET_FILES})
    SOURCE_GROUP(""  FILES "${PROGRAM}.cpp")

    SET_TARGET_PROPERTIES(${PROGRAM} PROPERTIES
        FOLDER demos
        COMPILE_FLAGS "${CH_BUILDFLAGS}"
        LINK_FLAGS "${CH_LINKERFLAG_EXE}"
    )

    TARGET_LINK_LIBRARIES(${PROGRAM} ${LIBRARIES})
    ADD_DEPENDENCIES(${PROGRAM} ${LIBRARIES})

    INSTALL(TARGETS ${PROGRAM} DESTINATION bin)
    ADD_TEST(${PROGRAM} ${PROJECT_BINARY_DIR}/bin/${PROGRAM})

ENDFOREACH(PROGRAM)


#--------------------------------------------------------------
# Executables that use OpenGL

IF (ENABLE_OPENGL)
    INCLUDE_DIRECTORIES(
        ${GLM_INCLUDE_DIRS}
        ${GLEW_INCLUDE_DIR}
        ${GLFW_INCLUDE_DIR}
    )

    SET(LIBRARIES ${LIBRARIES} ChronoEngine_OpenGL)
ENDIF()

SET(TESTS_G
    test_linactuator
    test_bodyauxref
    test_joints_dvi
    test_narrowphase
    test_jacobians
    
)

FOREACH(PROGRAM ${TESTS_G})
    MESSAGE(STATUS "...add ${PROGRAM}")

    ADD_EXECUTABLE(${PROGRAM}  "${PROGRAM}.cpp")
    SOURCE_GROUP(""  FILES "${PROGRAM}.cpp")

    SET_TARGET_PROPERTIES(${PROGRAM} PROPERTIES
        FOLDER demos
        COMPILE_FLAGS "${CH_BUILDFLAGS}"
        LINK_FLAGS "${CH_LINKERFLAG_EXE}"
    )

    TARGET_LINK_LIBRARIES(${PROGRAM} ${LIBRARIES})
    ADD_DEPENDENCIES(${PROGRAM} ${LIBRARIES})

    INSTALL(TARGETS ${PROGRAM} DESTINATION bin)
    ADD_TEST(${PROGRAM} ${PROJECT_BINARY_DIR}/bin/${PROGRAM})

ENDFOREACH(PROGRAM)

