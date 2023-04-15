#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>
#include <rg/Error.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void mouse_callback(GLFWwindow *window, double xpos, double ypos);
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

unsigned int loadTexture(const char *path);
unsigned int loadCubemap(vector<std::string> faces);
void setUpShader(Shader shader,glm::vec3 position,glm::vec3 specular,glm::vec3 diffuse,glm::vec3 ambient,float constant,float linear,float quadratic,glm::mat4 projection,glm::mat4 view,glm::vec3 camPosition,bool point_spot,float cutOff,float outerCutoff,glm::vec3 direction);


// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
bool hdr = false;
bool bloom = false;
float exposure = 1.0f;
bool inCube = false;
glm::vec3 cubePosition;

// camera
Camera camera (glm::vec3(0.0f, 0.0f, 3.0f));
float lastX = (float)SCR_WIDTH / 2.0f;
float lastY = (float)SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool CameraMouseMovementUpdateEnabled = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

glm::vec3 dif;
glm::vec3 spec;

glm::vec3 ambientSpot=glm::vec3(0.0f);
glm::vec3 diffuseSpot=dif;
glm::vec3 specularSpot=spec;

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};
struct SpotLight {
    glm::vec3 position;
    glm::vec3 direction;
    float cutOff;
    float outerCutOff;

    float constant;
    float linear;
    float quadratic;

    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;
};

int main() {
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    stbi_set_flip_vertically_on_load(false);

    // configure global opengl state
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // build and compile shaders

    Shader shader("resources/shaders/cubemaps.vs", "resources/shaders/cubemaps.fs");
    Shader skyboxShader("resources/shaders/skybox.vs","resources/shaders/skybox.fs");
    Shader ourShader("resources/shaders/2.model_lighting.vs", "resources/shaders/2.model_lighting.fs");
    Shader floorShader("resources/shaders/floor.vs", "resources/shaders/floor.fs");
    Shader hdrShader("resources/shaders/hdr.vs", "resources/shaders/hdr.fs");
    Shader blurShader("resources/shaders/blur.vs", "resources/shaders/blur.fs");
    Shader blendingShader("resources/shaders/blending.vs", "resources/shaders/blending.fs");
    Shader advShader("resources/shaders/advanced.vs", "resources/shaders/advanced.fs");

    //----Floor-------------------
    float Floor_vertices[] = {
            // positions                        // normals                      // texture coords
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  4.0f,
            0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  4.0f,  4.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  4.0f,  0.0f,
            0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  4.0f,  0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  4.0f,
    };

    //----cube inside the skybox
    float cubeVertices[] = {
            // positions                       // texture Coords
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 0.0f,

            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,

            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,

            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,
            0.5f, -0.5f, -0.5f,  1.0f, 1.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            0.5f, -0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,  0.0f, 1.0f,

            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f,
            0.5f,  0.5f, -0.5f,  1.0f, 1.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            0.5f,  0.5f,  0.5f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,  0.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,  0.0f, 1.0f
    };
    float skyboxVertices[] = {
            // positions
            -1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,

            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,

            -1.0f,  1.0f, -1.0f,
            1.0f,  1.0f, -1.0f,
            1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,

            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f, -1.0f,
            1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            1.0f, -1.0f,  1.0f
    };

    // floor VAO
    unsigned int floorVBO, floorVAO;
    glGenVertexArrays(1, &floorVAO);
    glGenBuffers(1, &floorVBO);
    glBindVertexArray(floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Floor_vertices), Floor_vertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));


    // cube VAO
    unsigned int cubeVAO, cubeVBO;
    glGenVertexArrays(1, &cubeVAO);
    glGenBuffers(1, &cubeVBO);
    glBindVertexArray(cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), &cubeVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));


    //--------Hdr framebuffer -----
    unsigned int hdrFBO;
    glGenFramebuffers(1, &hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER,hdrFBO);
    unsigned int colorBuffers[2];
    glGenTextures(2, colorBuffers);
    for(int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, colorBuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER,GL_COLOR_ATTACHMENT0+i,GL_TEXTURE_2D,colorBuffers[i],0);
    }
    //----depth buffer-renderbuffer------
    unsigned int rboDepth;
    glGenRenderbuffers(1, &rboDepth);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth);

    unsigned int attachment[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachment);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "Framebuffer not complete!" << std::endl;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //------ping pong---------

    unsigned int pingpongFBO[2];
    unsigned int pingpongColorbuffers[2];
    glGenFramebuffers(2, pingpongFBO);
    glGenTextures(2, pingpongColorbuffers);
    for (unsigned int i = 0; i < 2; ++i){
        glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[i]);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, nullptr); //bilo NULL umesto nullptr
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingpongColorbuffers[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Framebuffer not complete!" << std::endl;
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //-----quad------
    float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
            1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
    };

    unsigned int quadVAO,quadVBO;

    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // skybox VAO- moved to be last
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    unsigned int cubeTexture = loadTexture(FileSystem::getPath("resources/textures/pexels-photo-989941.jpeg").c_str());
    unsigned int floorTexture = loadTexture(FileSystem::getPath("resources/textures/dirttexture.jpg").c_str());


    vector<std::string> faces {
        FileSystem::getPath("resources/textures/skybox/right.jpg"),
        FileSystem::getPath("resources/textures/skybox/left.jpg"),
        FileSystem::getPath("resources/textures/skybox/top.jpg"),
        FileSystem::getPath("resources/textures/skybox/bottom.jpg"),
        FileSystem::getPath("resources/textures/skybox/front.jpg"),
        FileSystem::getPath("resources/textures/skybox/back.jpg")
    };
//todo: new skybox images
    /*vector<std::string> faces {
            FileSystem::getPath("resources/textures/envmap_interstellar1/interstellar_right1.tga"),
            FileSystem::getPath("resources/textures/envmap_interstellar1/interstellar_left1.tga"),
            FileSystem::getPath("resources/textures/envmap_interstellar1/interstellar_top.tga"),
            FileSystem::getPath("resources/textures/envmap_interstellar1/interstellar_bottom.tga"),
            FileSystem::getPath("resources/textures/envmap_interstellar1/interstellar_front1.tga"),
            FileSystem::getPath("resources/textures/envmap_interstellar1/interstellar_back1.tga"),
    }; */

    unsigned int cubemapTexture = loadCubemap(faces);

    // shader configuration

    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);

    floorShader.use();
    floorShader.setInt("material.floorTextured", 0);

    blendingShader.use();
    blendingShader.setInt("texture1", 0);

    shader.use();
    shader.setInt("texture1", 0);

/*
    skyboxShader.use();
    skyboxShader.setInt("skybox", 0);
*/
    blurShader.use();
    blurShader.setInt("image", 0);

    hdrShader.use();
    hdrShader.setInt("hdrBuffer", 0);
    hdrShader.setInt("bloomBlur", 1);




    // load models
    Model ourModel("resources/objects/backpack/backpack.obj");
    Model destroyedBuildingModel("resources/objects/BuildingRADI/Building01.obj");
    Model skyscraperModel("resources/objects/zgradaRADI/RuinedCity_pack.obj");
    Model ruinedBuildingModel("resources/objects/Post_Apocalyptic_BuildingRADI/Post_Apocalyptic_Building.obj");

    ourModel.SetShaderTextureNamePrefix("material.");
    destroyedBuildingModel.SetShaderTextureNamePrefix("material.");
    ruinedBuildingModel.SetShaderTextureNamePrefix("material.");
    skyscraperModel.SetShaderTextureNamePrefix("material.");

    // light init
    PointLight pointLight;
    pointLight.position = glm::vec3(4.0f, 4.0, 0.0);
    pointLight.ambient = glm::vec3(0.8);
    pointLight.diffuse = glm::vec3(1.0);
    pointLight.specular = glm::vec3(0.1);

    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.032f;

    SpotLight spotLight;
    spotLight.position=glm::vec3(0.0f);
    spotLight.direction=glm::vec3(0.0f);
    spotLight.cutOff=glm::cos(glm::radians(12.5f));
    spotLight.outerCutOff=glm::cos(glm::radians(15.0f));
    spotLight.constant=1.0f;
    spotLight.linear=0.09f;
    spotLight.quadratic=0.032;
    spotLight.ambient=ambientSpot;
    spotLight.diffuse=diffuseSpot;
    spotLight.specular=specularSpot;


    // render loop

    while (!glfwWindowShouldClose(window)) {

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        //glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glBindFramebuffer(GL_FRAMEBUFFER,hdrFBO);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthMask(GL_TRUE);

        // draw scene as normal
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        glm::mat4 model = glm::mat4(1.0f);

        shader.use();
        //shader.setMat4("model", model); //nisam sigurna da li treba ovde ovo-nista se ne menja
        shader.setMat4("view", view);
        shader.setMat4("projection", projection);

        spotLight.position=camera.Position;
        spotLight.direction=camera.Front;
        spotLight.ambient=ambientSpot;
        spotLight.diffuse=dif;
        spotLight.specular=spec;


        // setup shaders
        setUpShader(ourShader,pointLight.position,pointLight.specular,pointLight.diffuse,pointLight.ambient,pointLight.constant,pointLight.linear,pointLight.quadratic,projection,view,camera.Position,true,spotLight.cutOff,spotLight.outerCutOff,spotLight.direction);
        setUpShader(advShader,pointLight.position,pointLight.specular,pointLight.diffuse,pointLight.ambient,pointLight.constant,pointLight.linear,pointLight.quadratic,projection,view,camera.Position,true,spotLight.cutOff,spotLight.outerCutOff,spotLight.direction);

        /*
        ourShader.use();
        ourShader.setVec3("pointLight.position", pointLight.position);
        ourShader.setVec3("pointLight.ambient", pointLight.ambient);
        ourShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        ourShader.setVec3("pointLight.specular", pointLight.specular);
        ourShader.setFloat("pointLight.constant", pointLight.constant);
        ourShader.setFloat("pointLight.linear", pointLight.linear);
        ourShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        ourShader.setFloat("material.shininess", 32.0f);
        ourShader.setVec3("viewPosition", camera.Position);
        ourShader.setMat4("projection", projection);
        ourShader.setMat4("view", view);
*/


        // render models

        //-----destroyedBuildingModel----
        ourShader.use();
        //advShader.use();
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.25));
        model = glm::translate(model,glm::vec3(22.0f, -2.3, -30.0));
        shader.setMat4("model",model);
        shader.setMat4("projection",projection);
        shader.setMat4("view",view);
        ourShader.setMat4("model", model);
        //advShader.setMat4("model", model);
        destroyedBuildingModel.Draw(ourShader);
        //destroyedBuildingModel.Draw(advShader);

        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.25));
        model = glm::translate(model,glm::vec3(7.0f, -2.3, -30.0));
        ourShader.setMat4("model", model);
        destroyedBuildingModel.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.25));
        model = glm::translate(model,glm::vec3(-7.0f, -2.3, -30.0));
        ourShader.setMat4("model", model);
        destroyedBuildingModel.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.25));
        model = glm::translate(model,glm::vec3(-22.0f, -2.3, -30.0));
        ourShader.setMat4("model", model);
        destroyedBuildingModel.Draw(ourShader);


        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.25));
        model = glm::translate(model,glm::vec3(-22.0f, -2.3, -15.0));
        ourShader.setMat4("model", model);
        destroyedBuildingModel.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.25));
        model = glm::translate(model,glm::vec3(-22.0f, -2.3, 0.0));
        ourShader.setMat4("model", model);
        destroyedBuildingModel.Draw(ourShader);

        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.25));
        model = glm::translate(model,glm::vec3(-22.0f, -2.3, 15.0));
        ourShader.setMat4("model", model);
        destroyedBuildingModel.Draw(ourShader);


/*
        //-----skyscraperModel-------
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.05));
        model = glm::translate(model,glm::vec3(-80.0f, -9.5, 30.0));
        shader.setMat4("model",model);
        shader.setMat4("projection",projection);
        shader.setMat4("view",view);
        ourShader.setMat4("model", model);
        skyscraperModel.Draw(ourShader);


        //-----ruinedBuildingModel-----
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.15));
        model = glm::translate(model,glm::vec3(30.0f, -2.8, 10.0));
        shader.setMat4("model",model);
        shader.setMat4("projection",projection);
        shader.setMat4("view",view);
        ourShader.setMat4("model", model);
        ruinedBuildingModel.Draw(ourShader);

*/
        //----------floor-----------
        floorShader.use();
        model = glm::mat4(1.0f);

        model = glm::scale(model, glm::vec3(15.0));
        model = glm::translate(model,glm::vec3(0,0,0));
        floorShader.setMat4("model", model);
        floorShader.setMat4("projection", projection);
        floorShader.setMat4("view", view);

        floorShader.setVec3("light.position", pointLight.position);
        floorShader.setVec3("light.ambient", pointLight.ambient);
        floorShader.setVec3("light.diffuse", pointLight.diffuse);
        floorShader.setVec3("light.specular", pointLight.specular);
        floorShader.setVec3("material.specular", glm::vec3(0.1f));
        floorShader.setFloat("material.shininess", 10.0f);
        floorShader.setVec3("viewPos", camera.Position);

        glBindVertexArray(floorVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, floorTexture);
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(15.0));
        model = glm::translate(model,glm::vec3(0,0.465,-0.1333));
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glBindVertexArray(0);

        // --------inside cube----------
        blendingShader.use();
        //blendingShader.setInt("texture1", 0);
        blendingShader.setMat4("projection",projection);
        blendingShader.setMat4("view",view);
        blendingShader.setVec3("viewPosition", camera.Position);

        //cube is transparent once you are inside it

        glEnable(GL_CULL_FACE);
        for(int i = 0; i < 2; i++){
            if(i){
                glCullFace(GL_FRONT);
            }else {
                glCullFace(GL_BACK);
            }
            blendingShader.use();
            blendingShader.setInt("i", i);
            model = glm::mat4(1.0f);
            model = glm::scale(model, glm::vec3(0.2));
            // ovo dole?
            model = glm::translate(model,glm::vec3(-6.0,-2.0,-1.0));
            if(inCube){
                cubePosition = camera.Position;
                model = translate(model, camera.Position);
            }
            else {
                model = glm::translate(model, cubePosition);
            }

            blendingShader.setMat4("model", model);
            glBindVertexArray(cubeVAO);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cubeTexture);
            shader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, 36);
            glBindVertexArray(0);
        }
        glDisable(GL_CULL_FACE);

        /*
        model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(0.2));
        model = glm::translate(model,glm::vec3(-6.0,-2.0,-1.0));

        blendingShader.setMat4("model",model);
        glBindVertexArray(cubeVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, cubeTexture);
        shader.setMat4("model", model);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
*/

        // ------ draw skybox as last
        glDepthMask(GL_FALSE);
        glDepthFunc(GL_LEQUAL);
        skyboxShader.use();
        skyboxShader.setMat4("view",glm::mat4(glm::mat3(view)));
        skyboxShader.setMat4("projection", projection);
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);


        glBindFramebuffer(GL_FRAMEBUFFER,0);

        bool horizontal = true;
        bool first_iteration = true;
        blurShader.use();
        unsigned int amount = 10;
        for (unsigned int i = 0; i < amount; i++){
            glBindFramebuffer(GL_FRAMEBUFFER, pingpongFBO[horizontal]);
            blurShader.setInt("horizontal", horizontal);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, first_iteration ? colorBuffers[1] : pingpongColorbuffers[!horizontal]);
            glBindVertexArray(quadVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);
            horizontal = !horizontal;
            if (first_iteration)
                first_iteration = false;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        hdrShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,colorBuffers[0]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, pingpongColorbuffers[!horizontal]);
        hdrShader.setBool("bloom", bloom);
        hdrShader.setBool("hdr", hdr);
        hdrShader.setFloat("exposure", exposure);
        glBindVertexArray(quadVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // glfw: terminate, clearing all previously allocated GLFW resources.
    glDeleteVertexArrays(1, &skyboxVAO);
    glDeleteBuffers(1, &skyboxVAO);
    glDeleteVertexArrays(1, &floorVAO);
    glDeleteBuffers(1, &floorVAO);
    glDeleteVertexArrays(1, &cubeVAO);
    glDeleteBuffers(1, &cubeVAO);


    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if(glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS){
        camera.MovementSpeed=15.0f;
    }
    if(glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_RELEASE){
        camera.MovementSpeed=2.5f;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    if (CameraMouseMovementUpdateEnabled)
        camera.ProcessMouseMovement(xoffset, yoffset);

}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    camera.ProcessMouseScroll(yoffset);
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {

    if(key == GLFW_KEY_B && action == GLFW_PRESS){
        bloom=!bloom;
    }
    if(key == GLFW_KEY_H && action == GLFW_PRESS){
        hdr=!hdr;
    }
    if(key == GLFW_KEY_UP && action == GLFW_PRESS){
        exposure+=0.03;
    }
    if(key == GLFW_KEY_DOWN && action == GLFW_PRESS){
        if(exposure > 0.0f)
            exposure-=0.03;
        else
            exposure = 0.0f;
    }
    if(GLFW_KEY_ENTER && action == GLFW_PRESS){
        inCube = !inCube;
    }

    /*if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }*/
}

unsigned int loadCubemap(vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++){
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data){
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else{
            std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}
// utility function for loading a 2D texture from file
unsigned int loadTexture(char const * path){
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data){
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T,format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else{
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

void setUpShader(Shader shader,glm::vec3 position,glm::vec3 specular,glm::vec3 diffuse,glm::vec3 ambient,float constant,float linear,float quadratic,glm::mat4 projection,glm::mat4 view,glm::vec3 camPosition,bool point_spot,float cutOff,float outerCutoff,glm::vec3 direction){
    if(point_spot){
        shader.use();
        shader.setVec3("pointLight.position", position);
        shader.setVec3("pointLight.specular", specular);
        shader.setVec3("pointLight.diffuse", diffuse);
        shader.setVec3("pointLight.ambient", ambient);
        shader.setFloat("pointLight.constant", constant);
        shader.setFloat("pointLight.linear", linear);
        shader.setFloat("pointLight.quadratic", quadratic);
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setVec3("viewPosition", camPosition);
    }
    else{
        shader.use();
        shader.setVec3("spotLight.position", position);
        shader.setVec3("spotLight.direction",direction);
        shader.setVec3("spotLight.specular", specular);
        shader.setVec3("spotLight.diffuse", diffuse);
        shader.setVec3("spotLight.ambient", ambient);
        shader.setFloat("spotLight.constant", constant);
        shader.setFloat("spotLight.linear", linear);
        shader.setFloat("spotLight.quadratic", quadratic);
        shader.setFloat("cutOff",cutOff);
        shader.setFloat("outerCutOff",outerCutoff);

        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        shader.setVec3("viewPosition", camPosition);
    }

}