cmake.exe -G"MinGW Makefiles" .. ^
    -DTARGET_TYPE=OTHER -DCLIENT_ONLY=ON ^
    -DOPENSSL_ROOT_DIR:PATH=C:/OpenSSL-Win64 ^
    -DCMAKE_INSTALL_PREFIX:PATH=.. ^
    -DCMAKE_BUILD_TYPE=Release ^
    -DSWIG_EXECUTABLE:FILEPATH=%HOMEPATH%/Documents/swigwin-3.0.0/swig.exe ^
    -DBOOST_ROOT:PATH=C:/Boost