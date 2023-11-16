build_lib_example(
  NAME two-clocks-simple
  SOURCE_FILES
    two-clocks-simple.cc
  HEADER_FILES
  LIBRARIES_TO_LINK ${libinternet} ${libmobility} ${libstats}
                    ${libapplications} ${libcore} ${libpoint-to-point}
                    ${libnetwork} ${libpropagation} ${libenergy}
                    ${libspectrum} 
)

