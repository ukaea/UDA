module purge
module load GCC/12.2.0
module load libxml2/2.10.3-GCCcore-12.2.0
module load CapnProto/0.10.3-GCCcore-12.2.0
module load spdlog/1.11.0-GCCcore-12.2.0
module load Boost/1.81.0-GCCcore-12.2.0

cmake -Bbuild -H. -DENABLE_CAPNP=ON -DBUILD_SHARED_LIBS=ON -DCMAKE_INSTALL_PREFIX=install -DCMAKE_BUILD_TYPE=Debug
