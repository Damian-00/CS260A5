project(asteroids_engine)

############################
# Engine

# Source
add_library(asteroids_engine
  # Engine
  mesh.cpp
  mesh.hpp
  opengl.hpp
  shader.hpp
  shader.cpp
  texture.cpp
  texture.hpp
  window.cpp
  window.hpp
  font.hpp
  font.cpp
)

target_link_libraries(asteroids_engine PRIVATE 
  glad::glad
  glfw
)