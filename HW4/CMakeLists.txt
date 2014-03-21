CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
PROJECT (StreamServer)

SET (CMAKE_BUILD_TYPE DEBUG)

FIND_PACKAGE (Threads REQUIRED)
FIND_PACKAGE (Qt4 REQUIRED)

SET (CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -pthread -msse")
SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread -msse")

FIND_LIBRARY (ZLOG NAMES zlog)
#INCLUDE_DIRECTORIES (${ZLOG_INCLUDE_DIRS})
#LINK_DIRECTORIES (${ZLOG_LIBRARY_DIRS})

#INCLUDE_DIRECTORIES ("/usr/src/linux-source-3.2.0/arch/x86/include/")
#INCLUDE_DIRECTORIES ("/usr/src/linux/include/")


ADD_LIBRARY (thread_pool thread_pool.c)
ADD_LIBRARY (ring_buffer ring_buffer.c)
ADD_LIBRARY (file_loader file_loader.c)
ADD_LIBRARY (thread_pool_cluster thread_pool_cluster.c)
TARGET_LINK_LIBRARIES (thread_pool_cluster thread_pool)

ADD_LIBRARY (simple_sync simple_sync.c)
ADD_EXECUTABLE (test_simple_sync test_simple_sync.c)
TARGET_LINK_LIBRARIES (test_simple_sync 
	simple_sync 
	${CMAKE_THREAD_LIBS_INIT} 
	zlog)

ADD_LIBRARY (server server.c)
ADD_LIBRARY (alt_server alt_server.c)
ADD_LIBRARY (client client.c)
ADD_LIBRARY (socket socket_manager.c)
ADD_LIBRARY (request request.c)
ADD_LIBRARY (response response.c)
ADD_LIBRARY (sortlist sortlist.c)

ADD_EXECUTABLE (streamserver mainserver.c)
TARGET_LINK_LIBRARIES (streamserver 
	server 
	socket 
	file_loader 
	${CMAKE_THREAD_LIBS_INIT} 
	thread_pool_cluster 
	ring_buffer 
	sortlist 
	zlog 
	request 
	response)

ADD_EXECUTABLE (altstreamserver alt_mainserver.c)
TARGET_LINK_LIBRARIES (altstreamserver
	alt_server 
	socket 
	file_loader 
	${CMAKE_THREAD_LIBS_INIT} 
	thread_pool_cluster 
	alt_ring_buffer 
	sortlist 
	simple_sync
	zlog 
	request 
	response)

ADD_EXECUTABLE (test_socket_manager test_socket_manager.c)
TARGET_LINK_LIBRARIES (test_socket_manager 
	server client 
	socket 
	file_loader
	${CMAKE_THREAD_LIBS_INIT} 
	thread_pool_cluster 
	ring_buffer 
	sortlist
	zlog 
	request response)

ADD_LIBRARY (mainwindow_wrapper mainwindow_wrapper.cpp)
#TARGET_LINK_LIBRARIES (client mainwindow_wrapper)

QT4_WRAP_CPP (STREAM_CLIENT_MOC mainwindow.h)
QT4_WRAP_UI (STREAM_CLINET_FORM mainwindow.ui)

INCLUDE(${QT_USE_FILE})
ADD_DEFINITIONS(${QT_DEFINITIONS})
INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
ADD_EXECUTABLE(streamclient main.cpp mainwindow.cpp ${STREAM_CLIENT_MOC} ${STREAM_CLINET_FORM})
TARGET_LINK_LIBRARIES(streamclient 
	${QT_LIBRARIES}
	client 
	socket 
	file_loader
	${CMAKE_THREAD_LIBS_INIT} 
	thread_pool_cluster 
	ring_buffer 
	sortlist
	zlog 
	request 
	response)


ADD_LIBRARY (alt_ring_buffer alt_ring_buffer.c)
ADD_EXECUTABLE (test_alt_ring_buffer test_alt_ring_buffer.c)
TARGET_LINK_LIBRARIES (test_alt_ring_buffer alt_ring_buffer simple_sync thread_pool zlog ${CMAKE_THREAD_LIBS_INIT})

ADD_EXECUTABLE (test_ring_buffer test_ring_buffer.c)
TARGET_LINK_LIBRARIES (test_ring_buffer ring_buffer thread_pool zlog ${CMAKE_THREAD_LIBS_INIT})

ADD_EXECUTABLE (test_thread_pool test_thread_pool.c)
TARGET_LINK_LIBRARIES (test_thread_pool thread_pool zlog ${CMAKE_THREAD_LIBS_INIT})

ADD_EXECUTABLE (test_file_loader test_file_loader.c)
TARGET_LINK_LIBRARIES (test_file_loader thread_pool file_loader zlog ${CMAKE_THREAD_LIBS_INIT})

ADD_EXECUTABLE (test_thread_pool_cluster test_thread_pool_cluster.c)
TARGET_LINK_LIBRARIES (test_thread_pool_cluster thread_pool thread_pool_cluster zlog ${CMAKE_THREAD_LIBS_INIT})
