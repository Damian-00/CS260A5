#include <iostream>
#include "opengl.hpp"
#include "window.hpp"
namespace {
    /**
	 * OpenGL callback for debugging
	 * @param source
	 * @param type
	 * @param id
	 * @param severity
	 * @param length
	 * @param message
	 * @param userParam
	 */
    void APIENTRY openglCallbackFunction(GLenum        source,
                                         GLenum        type,
                                         GLuint        id,
                                         GLenum        severity,
                                         GLsizei       length,
                                         const GLchar* message,
                                         const void*   userParam)
    {
        if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) {
            return;
        }
        std::cout << "{\n";
        std::cout << "\tmessage: " << message << "\n";
        std::cout << "\ttype: ";
        switch (type) {
            case GL_DEBUG_TYPE_ERROR:
                std::cout << "ERROR";
                break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
                std::cout << "DEPRECATED_BEHAVIOR";
                break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
                std::cout << "UNDEFINED_BEHAVIOR";
                break;
            case GL_DEBUG_TYPE_PORTABILITY:
                std::cout << "PORTABILITY";
                break;
            case GL_DEBUG_TYPE_PERFORMANCE:
                std::cout << "PERFORMANCE";
                break;
            case GL_DEBUG_TYPE_OTHER:
                std::cout << "OTHER";
                break;
            default: break;
        }
        std::cout << "\n";

        std::cout << "\tid: " << id << "\n";
        std::cout << "\tseverity: ";
        switch (severity) {
            case GL_DEBUG_SEVERITY_LOW:
                std::cout << "LOW";
                break;
            case GL_DEBUG_SEVERITY_MEDIUM:
                std::cout << "MEDIUM";
                break;
            case GL_DEBUG_SEVERITY_HIGH:
                std::cout << "HIGH";
                break;
            case GL_DEBUG_SEVERITY_NOTIFICATION:
                std::cout << "NOTIFICATION";
                break;
            default: break;
        }
        std::cout << "\n}\n";
        assert(type != GL_DEBUG_TYPE_ERROR);
    }
}
namespace engine {
    /**
     * @brief 
     * 
     * @param w 
     * @param h 
     * @param window_name 
     * @return true 
     * @return false 
     */
    bool window::create(int w, int h, const char* window_name)
    {
        // Initialize
        if (!glfwInit()) {
            std::cerr << "Could not initialize glfw" << std::endl;
            return false;
        }

        // Window and context
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        m_window = glfwCreateWindow(w, h, window_name, nullptr, nullptr);
        if (!m_window) {
            std::cerr << "Could not create window" << std::endl;
            return false;
        }
        glfwMakeContextCurrent(m_window);

        // Vsync
        //glfwSwapInterval(0);

        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            std::cerr << "Could not load GLAD" << std::endl;
            return false;
        }

        if (!GLAD_GL_VERSION_4_4) {
            std::cerr << "Incorrect OpenGL version" << std::endl;
            return false;
        }
        
        { // Debug
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(openglCallbackFunction, nullptr);
            GLuint unusedIds = 0;
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, &unusedIds, GL_TRUE);
        }
        return true;
    }

    /**
     * @brief 
     * 
     * @return true 
     * @return false 
     */
    bool window::update()
    {
        glfwPollEvents();
        // Retrieve window size
        glfwGetFramebufferSize(m_window, &m_size.x, &m_size.y);
        return !glfwWindowShouldClose(m_window);
    }

    /**
     * @brief 
     * 
     */
    void window::swap_buffers()
    {
        glfwSwapBuffers(m_window);
    }

    /**
     * @brief 
     * 
     */
    void window::destroy()
    {
        if (m_window) {
            glfwDestroyWindow(m_window);
            m_window = nullptr;
        }
        glfwTerminate();
    }
}