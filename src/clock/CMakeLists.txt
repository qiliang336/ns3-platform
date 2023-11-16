build_lib(
  LIBNAME clock
  SOURCE_FILES
    model/clock-model.cc
    model/local-clock.cc
    model/localtime-simulator-impl.cc
    model/perfect-clock-model-impl.cc
    helper/clock-helper.cc
  HEADER_FILES
    model/clock-model.h
    model/local-clock.h
    model/localtime-simulator-impl.h
    model/perfect-clock-model-impl.h
    helper/clock-helper.h
  LIBRARIES_TO_LINK ${libcore} ${libnetwork} ${libpropagation} 
                    ${libspectrum} ${libmobility} ${libenergy}
  TEST_SOURCES
    test/clock-test.cc
)
