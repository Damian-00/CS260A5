project(asteroids_networking)

############################
# Networking

# Source
add_library(asteroids_networking
  # Networking
  networking.hpp
  networking.cpp
  utils.hpp
  utils.cpp
  server.hpp
  server.cpp
  client.hpp
  client.cpp


)

# Winsock (Windows only)
if (WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE ws2_32.lib)
endif ()