cmake_minimum_required(VERSION 3.8)
project(cs6210Project3)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        external/protos/store.proto
        external/protos/vendor.proto
        src/Makefile
        src/store.cc
        src/threadpool.h
        src/vendor_addresses.txt
        src/store.grpc.pb.h
        src/store.grpc.pb.cc
        src/store.pb.cc
        src/store.pb.h
        src/vendor.grpc.pb.cc
        src/vendor.grpc.pb.h
        src/vendor.pb.cc
        src/vendor.pb.h
        src/ThreadPool.h
        test/client.cc
        test/Makefile
        test/product_queries_util.h
        test/product_query_list.txt
        test/run_tests.cc
        test/run_vendors.cc
        test/vendor.cc)

add_executable(cs6210Project3 ${SOURCE_FILES})