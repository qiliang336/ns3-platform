build_lib(
  LIBNAME tsnwifi
  SOURCE_FILES
    model/Sta-priority.cc
    model/switch-trace-application.cc
    helper/wifi-setup.cc

    helper/ftm_sync.cc
    model/send-action-frame-application.cc
    model/sleep-cycle-application.cc
  HEADER_FILES
    model/Sta-priority.h
    model/switch-trace-application.h
    helper/wifi-setup.h
    
    helper/ftm_sync.h
    model/send-action-frame-application.h
    model/sleep-cycle-application.h

  LIBRARIES_TO_LINK ${libnetwork} ${libpropagation} ${libenergy}
                    ${libspectrum} ${libcore} ${libmobility}
                    ${libclock}
)
