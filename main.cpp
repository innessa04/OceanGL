#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <string>
#include <cmath>
#include <map>
#include <tuple>
#include <cstdlib>   
#include <ctime>     

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"

#include "shaderClass.h"
#include "Camera.h"

unsigned int createOceanMesh(int width, int depth, std::vector<float>& vertices, std::vector<unsigned int>& indices);
unsigned int createGroundMesh(int width, int depth, std::vector<float>& vertices, std::vector<unsigned int>& indices);
unsigned int loadTexture(const char* path);
unsigned int loadCubemap(std::vector<std::string> faces);
bool loadObj(const char* path, std::vector<float>& out_vertices, int& vertex_count, unsigned int& vao, unsigned int& vbo);
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback_wrapper(GLFWwindow* window, double, double);
const float WATER_SURFACE_Y = 0.0f;
const float MAX_FISH_HEIGHT = -0.5f;
const float MAX_BUBBLE_HEIGHT = WATER_SURFACE_Y - 0.1f;



float fishGlobalSpeed = 0.1f;   //szybkosc plywania rybek

const unsigned int SCR_WIDTH = 1280;
const unsigned int SCR_HEIGHT = 720;
float deltaTime = 0.0f;
float lastFrame = 0.0f;
glm::vec3 lightPos = glm::vec3(500.0f, 1000.0f, 300.0f);
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 0.95f);
Camera camera(SCR_WIDTH, SCR_HEIGHT, glm::vec3(0.0f, 8.0f, 15.0f));

//babelki
struct BubbleInstance {
    glm::vec3 position;
    float     speed;
    float     scale;
};

std::vector<BubbleInstance> bubbles;
std::vector<float> bubbleVerts;
unsigned int bubbleVAO, bubbleVBO;
int bubbleCount = 0;


//roslinki
struct PlantInstance {
    glm::vec3 position;
    float     scale;
    float     yaw;
};

struct PlantType {
    unsigned int vao;
    int vertexCount;
    float scaleMin, scaleMax;
    glm::vec3 color;
};

//struktury ryb
struct FishInstance {
    glm::vec3 position;
    glm::vec3 velocity;
    float     yaw;
};

struct FishType {
    unsigned int vao;
    int          vertexCount;
    unsigned int texture;
    float        speed;   //predkosc
    float        scale;   //rozmiar
    float        yawOffset;
};

//parametry lawic
constexpr int   GROUPS_PER_TYPE = 60;   //liczba lawic na gatunek
constexpr int   GROUP_MIN = 8;
constexpr int   GROUP_MAX = 14;
constexpr float SPAWN_RADIUS_XZ = 15.0f;  //obszar X‑Z przed kamerą
constexpr float SPAWN_Z_OFFSET = 5.0f;   //startowe (kamera.z − offset)
constexpr float DESPAWN_Z = -120.0f; //po przekroczeniu – respawn

//filtry post-processingu
unsigned int framebuffer;
unsigned int textureColorbuffer;
unsigned int rbo; 

//wierzchołki kwadratu na cały ekran dla post-processingu
float quadVertices[] = {
    // positions   // texCoords
    -1.0f,  1.0f,  0.0f, 1.0f,
    -1.0f, -1.0f,  0.0f, 0.0f,
     1.0f, -1.0f,  1.0f, 0.0f,

    -1.0f,  1.0f,  0.0f, 1.0f,
     1.0f, -1.0f,  1.0f, 0.0f,
     1.0f,  1.0f,  1.0f, 1.0f
};
unsigned int quadVAO, quadVBO;


//main
int main()
{
    //init
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OceanGL", NULL, NULL);
    if (window == NULL) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback_wrapper);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    std::cout << "INFO: OpenGL Version: " << glGetString(GL_VERSION) << std::endl;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    //shadery
    std::cout << "INFO: Ladowanie shaderow..." << std::endl;
    Shader oceanShader("ocean.vert", "ocean.frag");
    Shader fishShader("fish.vert", "fish.frag");
    Shader plantShader("plant.vert", "plant.frag");
    Shader skyboxShader("skybox.vert", "skybox.frag");
    Shader groundShader("ground.vert", "ground.frag");
    Shader bubbleShader("buble.vert", "buble.frag");
    Shader postProcessShader("postprocess.vert", "postprocess.frag");


    //ocean & dno
    std::vector<float> oceanVertices;   std::vector<unsigned int> oceanIndices;
    unsigned int oceanVAO = createOceanMesh(150, 150, oceanVertices, oceanIndices);
    int oceanIndexCount = static_cast<int>(oceanIndices.size());

    std::vector<float> groundVertices;  std::vector<unsigned int> groundIndices;
    unsigned int groundVAO = createGroundMesh(150, 150, groundVertices, groundIndices);
    int groundIndexCount = static_cast<int>(groundIndices.size());

    //skybox
    float skyboxVertices[] = {
         -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f,
         -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
          1.0f, -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,
         -1.0f, -1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f, -1.0f,  1.0f,
         -1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f, -1.0f,
         -1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f, -1.0f,  1.0f, -1.0f, -1.0f, -1.0f, -1.0f,  1.0f,  1.0f, -1.0f,  1.0f
    };
    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO); glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO); glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glBindVertexArray(0);

    std::vector<std::string> faces = { "px.png", "nx.png", "py.png", "ny.png", "pz.png", "nz.png" };
    unsigned int cubemapTexture = loadCubemap(faces);

    //tex ryb i piasku
    unsigned int fishTexture = loadTexture("fish_texture.png");
    unsigned int fishTexture1 = loadTexture("blazenek.png");
    unsigned int fishTexture2 = loadTexture("ladnakolorowa.png");
    unsigned int groundTexture = loadTexture("tex-4k.jpg");

    //modele rybek
    std::vector<float> fishVertices;
    unsigned int fishVAO = 0, fishVBO = 0; int fishVertexCount = 0;
    loadObj("C:/Users/kiqar/Desktop/fish.obj", fishVertices, fishVertexCount, fishVAO, fishVBO);

    std::vector<float> fish2Vertices, fish3Vertices;
    unsigned int fish2VAO = 0, fish2VBO = 0; int fish2VertexCount = 0;
    unsigned int fish3VAO = 0, fish3VBO = 0; int fish3VertexCount = 0;
    loadObj("C:/Users/kiqar/source/repos/terazzadziala/terazzadziala/wrednerybsko.obj", fish2Vertices, fish2VertexCount, fish2VAO, fish2VBO);
    loadObj("C:/Users/kiqar/Desktop/ladnekolorowe.obj", fish3Vertices, fish3VertexCount, fish3VAO, fish3VBO);

    //modele roslinek
    std::vector<float> coralVertices, pinkVertices, redVertices, starVertices;
    unsigned int coralVAO, pinkVAO, redVAO, starVAO;
    unsigned int coralVBO, pinkVBO, redVBO, starVBO;
    int coralCount = 0, pinkCount = 0, redCount = 0, starCount = 0;

    loadObj("C:/Users/kiqar/source/repos/terazzadziala/terazzadziala/coral2.obj", coralVertices, coralCount, coralVAO, coralVBO);
    loadObj("C:/Users/kiqar/source/repos/terazzadziala/terazzadziala/pinkcoral.obj", pinkVertices, pinkCount, pinkVAO, pinkVBO);
    loadObj("C:/Users/kiqar/source/repos/terazzadziala/terazzadziala/redcoral.obj", redVertices, redCount, redVAO, redVBO);
    loadObj("C:/Users/kiqar/source/repos/terazzadziala/terazzadziala/starfish.obj", starVertices, starCount, starVAO, starVBO);

    //babelki i generowanie ich
    loadObj("C:/Users/kiqar/source/repos/terazzadziala/terazzadziala/bubbles.obj", bubbleVerts, bubbleCount, bubbleVAO, bubbleVBO);
    for (int i = 0; i < 10; ++i) {
        float x = (rand() / (float)RAND_MAX - 0.5f) * 300.0f;
        float z = (rand() / (float)RAND_MAX - 0.5f) * 300.0f;
        float y = -10.0f - ((rand() / (float)RAND_MAX) * 4.0f);
        float speed = 0.01f + ((rand() / (float)RAND_MAX) * 0.1f);
        float scale = 0.0001f + ((rand() / (float)RAND_MAX) * 0.001f);
        bubbles.push_back({ glm::vec3(x, y, z), speed, scale });
    }

    std::vector<PlantType> plantTypes = {
    { coralVAO, coralCount, 0.03f, 0.05f, glm::vec3(0.0f, 0.128f, 0.0f) },
    { pinkVAO,  pinkCount,  0.1f, 0.4f, glm::vec3(1.0f, 0.5f, 0.8f) },
    { redVAO,   redCount,   0.03f, 0.06f, glm::vec3(0.9f, 0.1f, 0.1f) },
    { starVAO,  starCount,  0.2f, 0.4f, glm::vec3(0.3f, 0.6f, 1.0f) }
    };

    std::vector<std::vector<PlantInstance>> plantInstances(plantTypes.size());
    srand(static_cast<unsigned int>(time(nullptr)));

    for (size_t t = 0; t < plantTypes.size(); ++t) {
        for (int i = 0; i < 400; ++i) {
            float x = (rand() / (float)RAND_MAX - 0.5f) * 300.0f;
            float z = (rand() / (float)RAND_MAX - 0.5f) * 300.0f;
            float y = -10.0f;
            float scale = plantTypes[t].scaleMin + (rand() / (float)RAND_MAX) * (plantTypes[t].scaleMax - plantTypes[t].scaleMin);
            float yaw = (rand() / (float)RAND_MAX) * 6.28318f;
            plantInstances[t].push_back({ glm::vec3(x, y, z), scale, yaw });
        }
    }

    std::vector<FishType> fishTypes = {
        { fishVAO,  fishVertexCount, fishTexture,  0.33f, 0.30f,  glm::radians(180.0f) },
        { fish2VAO, fish2VertexCount, fishTexture1, 0.76f, 0.85f,  glm::radians(90.0f) },
        { fish3VAO, fish3VertexCount, fishTexture2, 0.20f, 0.20f,  glm::radians(-90.0f) }
    };

    srand(static_cast<unsigned int>(time(nullptr)));
    std::map<int, std::vector<FishInstance>> fishInstances;

    for (size_t t = 0; t < fishTypes.size(); ++t) {
        std::vector<FishInstance> list;
        for (int g = 0; g < GROUPS_PER_TYPE; ++g) {
            int groupSize = GROUP_MIN + rand() % (GROUP_MAX - GROUP_MIN + 1);
            float spawnX = camera.Position.x + (rand() / (float)RAND_MAX - 0.5f) * 100.0f;
            float spawnZ = camera.Position.z + (rand() / (float)RAND_MAX - 0.5f) * 100.0f;
            float spawnY = -9.0f + ((rand() / (float)RAND_MAX) * 6.0f);
            glm::vec3 center(spawnX, spawnY, spawnZ);
            glm::vec3 dir((rand() / (float)RAND_MAX - 0.5f) * 0.4f, (rand() / (float)RAND_MAX - 0.5f) * 0.2f, -0.6f + (rand() / (float)RAND_MAX) * 1.2f);
            dir = glm::normalize(dir);
            float yawBase = std::atan2(dir.x, dir.z);
            for (int i = 0; i < groupSize; ++i) {
                glm::vec3 offset((rand() / (float)RAND_MAX - 0.5f) * 6.0f, (rand() / (float)RAND_MAX - 0.5f) * 2.0f, (rand() / (float)RAND_MAX - 0.5f) * 6.0f);
                glm::vec3 pos = center + offset;
                pos.y = std::min(pos.y, MAX_FISH_HEIGHT);
                list.push_back({ pos, dir, yawBase });
            }
        }
        fishInstances[static_cast<int>(t)] = std::move(list);
    }

	//konfiguracja FBO i tekstury dla post-processingu
    glGenFramebuffers(1, &framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);

    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cerr << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //konfiguracja VAO/VBO dla kwadratu post-processingu 
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    std::cout << "INFO: Inicjalizacja zakonczona. Wchodze do glownej petli..." << std::endl;

    //glowna petla
    while (!glfwWindowShouldClose(window)) {

        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        camera.Inputs(window, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        //rendere sceny do FBO
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glEnable(GL_DEPTH_TEST);
        glClearColor(0.1f, 0.2f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getViewMatrix();
        glm::mat4 projection = camera.getProjectionMatrix();

        //skybox
        glDepthMask(GL_FALSE);
        skyboxShader.Activate();
        skyboxShader.setMat4("view", glm::mat4(glm::mat3(view)));
        skyboxShader.setMat4("projection", projection);
        skyboxShader.setInt("skybox", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glBindVertexArray(skyboxVAO);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthMask(GL_TRUE);

        //piasek
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, groundTexture);
        groundShader.use();
        groundShader.setInt("texture_diffuse1", 0);
        groundShader.setFloat("texScale", 1.0f);
        groundShader.setVec3("fogColor", glm::vec3(0.0, 0.0, 0.0));
        groundShader.setFloat("fogDensity", 0.0f);
        groundShader.setFloat("ambientStrength", 1.0f);
        glm::mat4 groundModel = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -10.0f, 0.0f));
        groundShader.setMat4("model", groundModel);
        groundShader.setMat4("view", view);
        groundShader.setMat4("projection", projection);
        groundShader.setVec3("lightPos", lightPos);
        groundShader.setVec3("lightColor", lightColor);
        groundShader.setVec3("viewPos", camera.Position);
        glBindVertexArray(groundVAO);
        glDrawElements(GL_TRIANGLES, groundIndexCount, GL_UNSIGNED_INT, 0);

        //blending przed renderowaniem oceanu i babelkow
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //ocean
        oceanShader.Activate();
        oceanShader.setMat4("projection", projection);
        oceanShader.setMat4("view", view);
        oceanShader.setMat4("model", glm::mat4(1.0f));
        oceanShader.setFloat("time", currentFrame);
        oceanShader.setVec3("viewPos", camera.Position);
        oceanShader.setVec3("lightPos", lightPos);
        oceanShader.setVec3("lightColor", lightColor); 
        oceanShader.setInt("skybox", 0); 
        glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glBindVertexArray(oceanVAO);
        glDrawElements(GL_TRIANGLES, oceanIndexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);


        //render babelkow
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture); 
        bubbleShader.use();
        bubbleShader.setMat4("view", view);
        bubbleShader.setMat4("projection", projection);
        bubbleShader.setVec3("bubbleColor", glm::vec3(0.8f, 0.9f, 1.0f));

        glBindVertexArray(bubbleVAO);
        for (BubbleInstance& b : bubbles) {
            b.position.y += b.speed * deltaTime * 60.0f;
            if (b.position.y > MAX_BUBBLE_HEIGHT) {
                b.position = glm::vec3((rand() / (float)RAND_MAX - 0.5f) * 300.0f, -10.0f - ((rand() / (float)RAND_MAX) * 10.0f), (rand() / (float)RAND_MAX - 0.5f) * 300.0f);
                b.speed = 0.1f + ((rand() / (float)RAND_MAX) * 0.1f);
                b.scale = 0.0001f + ((rand() / (float)RAND_MAX) * 0.1f);
            }
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, glm::vec3(0.0f, -1.0f, 0.0f));
            model = glm::scale(model, glm::vec3(b.scale));
            model = glm::rotate(model, glm::radians(180.0f), glm::vec3(1, 0, 0));
            model = glm::translate(model, b.position);
            bubbleShader.setMat4("model", model);
            glDrawArrays(GL_TRIANGLES, 0, bubbleCount);
        }

        //render roslin 
        for (size_t t = 0; t < plantTypes.size(); ++t) {
            const PlantType& type = plantTypes[t];
            glBindVertexArray(type.vao);
            plantShader.use();
            plantShader.setMat4("view", view);
            plantShader.setMat4("projection", projection);
            plantShader.setVec3("lightPos", lightPos);
            plantShader.setVec3("lightColor", lightColor);
            plantShader.setVec3("viewPos", camera.Position);
            plantShader.setVec3("baseColor", type.color);
            for (const PlantInstance& p : plantInstances[t]) {
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, p.position);
                model = glm::rotate(model, p.yaw, glm::vec3(0.0f, 1.0f, 0.0f));
                if (type.vao == pinkVAO) model = glm::rotate(model, glm::radians(360.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                else model = glm::rotate(model, glm::radians(270.0f), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::scale(model, glm::vec3(p.scale));
                plantShader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, type.vertexCount);
            }
        }
        glBindVertexArray(0);

        //render ryb
        fishShader.Activate();
        fishShader.setMat4("projection", projection);
        fishShader.setMat4("view", view);
        fishShader.setVec3("viewPos", camera.Position);
        fishShader.setVec3("lightPos", lightPos);
        fishShader.setVec3("lightColor", lightColor);
        for (size_t t = 0; t < fishTypes.size(); ++t) {
            const FishType& type = fishTypes[t];
            glBindVertexArray(type.vao);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, type.texture);
            fishShader.setInt("texture_diffuse1", 0);
            auto& list = fishInstances[static_cast<int>(t)];
            for (FishInstance& fish : list) {
                fish.position += fish.velocity * fishGlobalSpeed * type.speed * deltaTime * 60.0f;
                if (fish.position.y > MAX_FISH_HEIGHT) {
                    fish.position.y = MAX_FISH_HEIGHT;
                    fish.velocity.y = std::min(fish.velocity.y, 0.0f);
                }
                if (fish.position.z < DESPAWN_Z) {
                    fish.position.x = (rand() / (float)RAND_MAX - 0.5f) * 2.0f * SPAWN_RADIUS_XZ;
                    fish.position.y = -9.0f + ((rand() / (float)RAND_MAX) * 6.0f);
                    fish.position.z = camera.Position.z - SPAWN_Z_OFFSET - ((rand() / (float)RAND_MAX) * 10.0f);
                }
                glm::vec2 uvScale(1.0f), uvOffset(0.0f);
                if (type.texture == fishTexture1) { uvScale = glm::vec2(1.0f, 0.5f); uvOffset = glm::vec2(0.0f, 0.5f); }
                else if (type.texture == fishTexture2) { uvScale = glm::vec2(1.0f, 1.0f); uvOffset = glm::vec2(0.0f, 0.0f); }
                fishShader.setVec2("uvScale", uvScale);
                fishShader.setVec2("uvOffset", uvOffset);
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, fish.position);
                model = glm::rotate(model, fish.yaw + type.yawOffset + glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::scale(model, glm::vec3(type.scale));
                fishShader.setMat4("model", model);
                glDrawArrays(GL_TRIANGLES, 0, type.vertexCount);
            }
        }
        glBindVertexArray(0);
        // glDisable(GL_BLEND);


        //render ramki do domyuslnego bufora
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        postProcessShader.use();
        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        postProcessShader.setInt("screenTexture", 0); 

        glDrawArrays(GL_TRIANGLES, 0, 6);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    glDeleteFramebuffers(1, &framebuffer);
    glDeleteTextures(1, &textureColorbuffer);
    glDeleteRenderbuffers(1, &rbo);
    glDeleteVertexArrays(1, &quadVAO);
    glDeleteBuffers(1, &quadVBO);

    glfwTerminate();
    return 0;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    camera.width = width;
    camera.height = height;
    if (textureColorbuffer) { 
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
        glBindTexture(GL_TEXTURE_2D, 0);

        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }
}

void mouse_callback_wrapper(GLFWwindow* window, double xpos, double ypos) {
    camera.MouseCallback(window, xpos, ypos);
}

unsigned int loadTexture(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    unsigned char* data = stbi_load(path, &width, &height, &nrComponents, 0);
    std::cout << "INFO: Texture " << path << " loaded with " << nrComponents << " channel(s)." << std::endl;
    if (data) {
        GLenum format = GL_RGB;
        if (nrComponents == 1) format = GL_RED;
        else if (nrComponents == 3) format = GL_RGB;
        else if (nrComponents == 4) format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
        std::cout << "INFO: Texture loaded: " << path << std::endl;
    }
    else {
        std::cerr << "ERROR: Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
        return 0;
    }
    return textureID;
}

unsigned int loadCubemap(std::vector<std::string> faces) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);
    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false);


    for (unsigned int i = 0; i < faces.size(); i++) {
        unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            GLenum format = GL_RGB;
            if (nrChannels == 4) format = GL_RGBA;
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
            std::cout << "INFO: Cubemap loaded: " << faces[i] << std::endl;
        }
        else {
            std::cerr << "ERROR: Cubemap texture failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data); return 0;
        }
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    stbi_set_flip_vertically_on_load(true);
    return textureID;
}

bool loadObj(const char* path, std::vector<float>& out_vertices, int& vertex_count, unsigned int& vao, unsigned int& vbo) {
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;

    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path)) {
        std::cerr << "TINYOBJLOADER_ERROR: " << warn << err << std::endl;
        return false;
    }

    out_vertices.clear();

    for (const auto& shape : shapes) {
        for (const auto& index : shape.mesh.indices) {
            out_vertices.push_back(attrib.vertices[3 * index.vertex_index + 0]);
            out_vertices.push_back(attrib.vertices[3 * index.vertex_index + 1]);
            out_vertices.push_back(attrib.vertices[3 * index.vertex_index + 2]);

            if (index.normal_index >= 0 && !attrib.normals.empty()) {
                out_vertices.push_back(attrib.normals[3 * index.normal_index + 0]);
                out_vertices.push_back(attrib.normals[3 * index.normal_index + 1]);
                out_vertices.push_back(attrib.normals[3 * index.normal_index + 2]);
            }
            else { out_vertices.push_back(0.0f); out_vertices.push_back(1.0f); out_vertices.push_back(0.0f); }

            if (index.texcoord_index >= 0 && !attrib.texcoords.empty()) {
                out_vertices.push_back(attrib.texcoords[2 * index.texcoord_index + 0]);
                out_vertices.push_back(1.0f - attrib.texcoords[2 * index.texcoord_index + 1]);
            }
            else { out_vertices.push_back(0.0f); out_vertices.push_back(0.0f); }
        }
    }
    vertex_count = static_cast<int>(out_vertices.size() / 8);

    if (vertex_count == 0) return false;

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, out_vertices.size() * sizeof(float), &out_vertices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float))); glEnableVertexAttribArray(2);
    glBindVertexArray(0);

    std::cout << "INFO: Model loaded: " << path << ", Vertices: " << vertex_count << std::endl;
    return true;
}

unsigned int createOceanMesh(int width, int depth, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    vertices.clear(); indices.clear();
    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            vertices.push_back(((float)x - (float)width / 2.0f) * 2.0f);
            vertices.push_back(0.0f);
            vertices.push_back(((float)z - (float)depth / 2.0f) * 2.0f);
            vertices.push_back(0.0f); vertices.push_back(1.0f); vertices.push_back(0.0f);
        }
    }
    for (int z = 0; z < depth - 1; ++z) {
        for (int x = 0; x < width - 1; ++x) {
            int tl = (z * width) + x; int tr = tl + 1; int bl = ((z + 1) * width) + x; int br = bl + 1;
            indices.push_back(tl); indices.push_back(bl); indices.push_back(tr);
            indices.push_back(tr); indices.push_back(bl); indices.push_back(br);
        }
    }
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO); glGenBuffers(1, &EBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO); glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO); glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float))); glEnableVertexAttribArray(1);
    glBindVertexArray(0);
    std::cout << "INFO: Ocean mesh created. Vertices: " << vertices.size() / 6 << ", Indices: " << indices.size() << std::endl;
    return VAO;
}
unsigned int createGroundMesh(int width, int depth, std::vector<float>& vertices, std::vector<unsigned int>& indices) {
    vertices.clear();
    indices.clear();

    for (int z = 0; z < depth; ++z) {
        for (int x = 0; x < width; ++x) {
            float worldX = ((float)x - (float)width / 2.0f) * 2.0f;
            float worldZ = ((float)z - (float)depth / 2.0f) * 2.0f;

            float u = (float)x / (width - 1);
            float v = (float)z / (depth - 1);
            if (x == 0 && z == 0) {
                std::cout << "Sample UV (0,0): " << u << ", " << v << std::endl;
            }

            vertices.push_back(worldX);
            vertices.push_back(0.0f);
            vertices.push_back(worldZ);

            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);

            vertices.push_back(u);
            vertices.push_back(v);
        }
    }

    for (int z = 0; z < depth - 1; ++z) {
        for (int x = 0; x < width - 1; ++x) {
            int tl = (z * width) + x;
            int tr = tl + 1;
            int bl = ((z + 1) * width) + x;
            int br = bl + 1;

            indices.push_back(tl);
            indices.push_back(bl);
            indices.push_back(tr);

            indices.push_back(tr);
            indices.push_back(bl);
            indices.push_back(br);
        }
    }

    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    std::cout << "INFO: Ground mesh created with UVs. Vertices: " << vertices.size() / 8 << ", Indices: " << indices.size() << std::endl;

    return VAO;
}