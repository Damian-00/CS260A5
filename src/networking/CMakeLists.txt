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
  protocol.hpp
  protocol.cpp


)

# Winsock (Windows only)
if (WIN32)
    target_link_libraries(asteroids_networking PRIVATE
    ws2_32.lib
    )

    target_link_libraries(asteroids_networking PRIVATE 
    glm::glm
    )
endif ()