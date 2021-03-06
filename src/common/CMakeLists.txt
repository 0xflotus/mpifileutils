# todo re-asses if all of these must be *installed*
LIST(APPEND libmfu_install_headers
  mfu.h
  mfu_bz2.h
  mfu_flist.h
  mfu_flist_internal.h
  mfu_io.h
  mfu_param_path.h
  mfu_path.h
  mfu_pred.h
  mfu_util.h
  )
INSTALL(FILES ${libmfu_install_headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# common library
LIST(APPEND libmfu_srcs
  mfu_compress_bz2_libcircle.c
  mfu_decompress_bz2_libcircle.c
  mfu_flist.c
  mfu_flist_chunk.c
  mfu_flist_copy.c
  mfu_flist_io.c
  mfu_flist_chmod.c
  mfu_flist_create.c
  mfu_flist_remove.c
  mfu_flist_sort.c
  mfu_flist_usrgrp.c
  mfu_flist_walk.c
  mfu_io.c
  mfu_param_path.c
  mfu_path.c
  mfu_pred.c
  mfu_util.c
  strmap.c
  )

ADD_LIBRARY(mfu_o OBJECT ${libmfu_srcs})
SET_TARGET_PROPERTIES(mfu_o PROPERTIES C_STANDARD 99)

ADD_LIBRARY(mfu SHARED $<TARGET_OBJECTS:mfu_o>)
TARGET_LINK_LIBRARIES(mfu ${MFU_EXTERNAL_LIBS})
SET_TARGET_PROPERTIES(mfu PROPERTIES OUTPUT_NAME mfu CLEAN_DIRECT_OUTPUT 1)
INSTALL(TARGETS mfu DESTINATION ${CMAKE_INSTALL_LIBDIR})

ADD_LIBRARY(mfu-static STATIC $<TARGET_OBJECTS:mfu_o>)
TARGET_LINK_LIBRARIES(mfu-static ${MFU_EXTERNAL_LIBS})
SET_TARGET_PROPERTIES(mfu-static PROPERTIES OUTPUT_NAME mfu CLEAN_DIRECT_OUTPUT 1)
INSTALL(TARGETS mfu-static DESTINATION ${CMAKE_INSTALL_LIBDIR})
