
#include "Dependencies/glew/glew.h"
#include "Dependencies/GLFW/glfw3.h"
#include "Dependencies/glm/glm.hpp"
#include "Dependencies/glm/gtc/matrix_transform.hpp"
#include "Dependencies/stb_image/stb_image.h"
#include "Shader.h"
#include "Texture.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <map>
#include <cstdlib>
#include <ctime>

//screen setting
const int SCR_WIDTH = 1600;
const int SCR_HEIGHT = 1200;

//define parameter for object
#define N 10
#define B 3
#define Amount 2000
#define F 6
int index = 0;
//vao,vbo and ebo
GLuint vaoID[N], vboID[N], eboID[N];

//set camera
glm::vec3 cameraposition = glm::vec3(15.0f, 0.0f, 0.0f);
glm::vec3 camerafrontvector = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraupvector = glm::vec3(0.0f, 1.0f, 0.0f);

//spotlight direction
glm::vec3 spotlightdir = camerafrontvector - cameraposition + glm::vec3(0.0f, 0.3f, 0.0f);

//set view control
float dtime = 0.0f;
float lframe = 0.0f;

//for zoom in and out
float fov = 45;
float speed = 0.2f;
float offset=0.0f;
//spacecraft
bool first = true;
float oldx = 0.0f;
float spacecraftangle = 0.0f;
float x1 = 0.0f;
float x2 = 5.0f;
float x3 = 10.0f;
//view & projection
glm::mat4 view;
glm::mat4 projection;

//three shader
Shader shading;
Shader lightshading;
Shader skyboxshading;

//texture load
Texture Rock[2];
Texture Craft[2];
Texture SpaceCraft[2];
Texture Food[4];
Texture Planet[2];
//to keep track of object texture
int planeindex = 0;
int alienvehiculeindex[B] = { 0,0,0 };
bool food[F];
int GoldNum = 0;
int FoodNum = F;
int RockNum[Amount];
//adjust environment light intensity
float dlightintensity = 1.2f;
float plightone = 0.8f;
float plighttwo = 0.8f;

//keep track skybox texture
unsigned int skyboxtex;

//for self rotation of objects
float selfrotating = 0.0f;

//for spacecraft parameter
glm::vec3 SCInitialPos = glm::vec3(15.0f, 3.0f, -3.0f);
glm::vec3 SCTranslation = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 SC_world_Front_Direction;
glm::vec3 SC_world_Right_Direction;

//for rock position
glm::mat4 rockmodel[Amount];

// struct for storing the obj file
struct Vertex {
	glm::vec3 position;
	glm::vec2 uv;
	glm::vec3 normal;
};

struct Model {
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
};

//object modal
Model obj[N];

Model loadOBJ(const char* objPath)
{
	// function to load the obj file
	// Note: this simple function cannot load all obj files.

	struct V {
		// struct for identify if a vertex has showed up
		unsigned int index_position, index_uv, index_normal;
		bool operator == (const V& v) const {
			return index_position == v.index_position && index_uv == v.index_uv && index_normal == v.index_normal;
		}
		bool operator < (const V& v) const {
			return (index_position < v.index_position) ||
				(index_position == v.index_position && index_uv < v.index_uv) ||
				(index_position == v.index_position && index_uv == v.index_uv && index_normal < v.index_normal);
		}
	};

	std::vector<glm::vec3> temp_positions;
	std::vector<glm::vec2> temp_uvs;
	std::vector<glm::vec3> temp_normals;

	std::map<V, unsigned int> temp_vertices;

	Model model;
	unsigned int num_vertices = 0;

	std::cout << "\nLoading OBJ file " << objPath << "..." << std::endl;

	std::ifstream file;
	file.open(objPath);

	// Check for Error
	if (file.fail()) {
		std::cerr << "Impossible to open the file! Do you use the right path? See Tutorial 6 for details" << std::endl;
		exit(1);
	}

	while (!file.eof()) {
		// process the object file
		char lineHeader[128];
		file >> lineHeader;

		if (strcmp(lineHeader, "v") == 0) {
			// geometric vertices
			glm::vec3 position;
			file >> position.x >> position.y >> position.z;
			temp_positions.push_back(position);
		}
		else if (strcmp(lineHeader, "vt") == 0) {
			// texture coordinates
			glm::vec2 uv;
			file >> uv.x >> uv.y;
			temp_uvs.push_back(uv);
		}
		else if (strcmp(lineHeader, "vn") == 0) {
			// vertex normals
			glm::vec3 normal;
			file >> normal.x >> normal.y >> normal.z;
			temp_normals.push_back(normal);
		}
		else if (strcmp(lineHeader, "f") == 0) {
			// Face elements
			V vertices[3];
			for (int i = 0; i < 3; i++) {
				char ch;
				file >> vertices[i].index_position >> ch >> vertices[i].index_uv >> ch >> vertices[i].index_normal;
			}

			// Check if there are more than three vertices in one face.
			std::string redundency;
			std::getline(file, redundency);
			if (redundency.length() >= 5) {
				std::cerr << "There may exist some errors while load the obj file. Error content: [" << redundency << " ]" << std::endl;
				std::cerr << "Please note that we only support the faces drawing with triangles. There are more than three vertices in one face." << std::endl;
				std::cerr << "Your obj file can't be read properly by our simple parser :-( Try exporting with other options." << std::endl;
				exit(1);
			}

			for (int i = 0; i < 3; i++) {
				if (temp_vertices.find(vertices[i]) == temp_vertices.end()) {
					// the vertex never shows before
					Vertex vertex;
					vertex.position = temp_positions[vertices[i].index_position - 1];
					vertex.uv = temp_uvs[vertices[i].index_uv - 1];
					vertex.normal = temp_normals[vertices[i].index_normal - 1];

					model.vertices.push_back(vertex);
					model.indices.push_back(num_vertices);
					temp_vertices[vertices[i]] = num_vertices;
					num_vertices += 1;
				}
				else {
					// reuse the existing vertex
					unsigned int index = temp_vertices[vertices[i]];
					model.indices.push_back(index);
				}
			} // for
		} // else if
		else {
			// it's not a vertex, texture coordinate, normal or face
			char stupidBuffer[1024];
			file.getline(stupidBuffer, 1024);
		}
	}
	file.close();

	std::cout << "There are " << num_vertices << " vertices in the obj file.\n" << std::endl;
	return model;
}

void get_OpenGL_info()
{
	// OpenGL information
	const GLubyte* name = glGetString(GL_VENDOR);
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* glversion = glGetString(GL_VERSION);
	std::cout << "OpenGL company: " << name << std::endl;
	std::cout << "Renderer name: " << renderer << std::endl;
	std::cout << "OpenGL version: " << glversion << std::endl;
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
	// load skymap texture
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);


	int width, height, nrchannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char* data = stbi_load(faces[i].c_str(), &width, &height, &nrchannels, 0);
		GLenum format = 3;
		switch (nrchannels) {
		case 1: format = GL_RED; break;
		case 3: format = GL_RGB; break;
		case 4: format = GL_RGBA; break;
		}
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Failed to load texture: " << std::endl;
			exit(1);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	std::cout << "Load " << "successfully!" << std::endl;
	return textureID;
}

void sendDataToOpenGL()
{
	//Load objects and bind to VAO and VBO
	//Load textures
	for (int i = 0; i < Amount; i++)
	{
		RockNum[i] = rand() % 30;
		if (RockNum[i] <= 28)
		{
			RockNum[i] = 0;
		}
		else
		{
			GoldNum++;
			RockNum[i] = 1;
		}
	}
	const GLfloat cube[] =
	{
		-1.0f, -1.0f, -1.0f,  // 000

		-1.0f, -1.0f, +1.0f,  // 001

		-1.0f, +1.0f, -1.0f,  // 010

		-1.0f, +1.0f, +1.0f,  // 011

		+1.0f, -1.0f, -1.0f,  // 100

		+1.0f, -1.0f, +1.0f,  // 101

		+1.0f, +1.0f, -1.0f,  // 110

		+1.0f, +1.0f, +1.0f,  // 111
	};
	GLuint cubeindices[] = {
		0,4,6,
		6,2,0,
		1,5,7,
		7,3,1,
		3,2,0,
		0,1,3,
		7,6,4,
		4,5,7,
		0,4,5,
		5,1,0,
		2,6,7,
		7,3,2
	};

	glGenVertexArrays(N, vaoID);
	glGenBuffers(N, vboID);
	glGenBuffers(N, eboID);


	//alienvehicle
	obj[1] = loadOBJ("resource/craft.obj");

	glBindVertexArray(vaoID[1]);

	glBindBuffer(GL_ARRAY_BUFFER, vboID[1]);
	glBufferData(GL_ARRAY_BUFFER, obj[1].vertices.size() * sizeof(Vertex), &obj[1].vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID[1]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj[1].indices.size() * sizeof(unsigned int), &obj[1].indices[0], GL_STATIC_DRAW);

	//vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	//texture position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);

	//normal position
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	//alienvehicle texture
	Craft[0].setupTexture("resource/texture/zp.jpg");
	Craft[1].setupTexture("resource/texture/wzry.jfif");

	//chicken
	obj[2] = loadOBJ("resource/chicken.obj");

	glBindVertexArray(vaoID[2]);

	glBindBuffer(GL_ARRAY_BUFFER, vboID[2]);
	glBufferData(GL_ARRAY_BUFFER, obj[2].vertices.size() * sizeof(Vertex), &obj[2].vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID[2]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj[2].indices.size() * sizeof(unsigned int), &obj[2].indices[0], GL_STATIC_DRAW);

	// vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	//texture position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);

	//normal position
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	//chicken texture
	Food[0].setupTexture("resource/texture/1.jfif");

	//planet
	obj[3] = loadOBJ("resource/planet.obj");

	glBindVertexArray(vaoID[3]);

	glBindBuffer(GL_ARRAY_BUFFER, vboID[3]);
	glBufferData(GL_ARRAY_BUFFER, obj[3].vertices.size() * sizeof(Vertex), &obj[3].vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID[3]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj[3].indices.size() * sizeof(unsigned int), &obj[3].indices[0], GL_STATIC_DRAW);

	// vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	//texture position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);

	//normal position
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	//planet texture
	Planet[0].setupTexture("resource/texture/planetTexture.bmp");
	Planet[1].setupTexture("resource/texture/planetNormal.bmp");

	//rock
	obj[4] = loadOBJ("resource/rock.obj");

	glBindVertexArray(vaoID[4]);

	glBindBuffer(GL_ARRAY_BUFFER, vboID[4]);
	glBufferData(GL_ARRAY_BUFFER, obj[4].vertices.size() * sizeof(Vertex), &obj[4].vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID[4]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj[4].indices.size() * sizeof(unsigned int), &obj[4].indices[0], GL_STATIC_DRAW);

	// vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	//texture position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);

	//normal position
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	//rock texture
	Rock[0].setupTexture("resource/texture/rockTexture.bmp");
	Rock[1].setupTexture("resource/texture/gold.bmp");
	//spacecraft
	obj[5] = loadOBJ("resource/spacecraft.obj");

	glBindVertexArray(vaoID[5]);

	glBindBuffer(GL_ARRAY_BUFFER, vboID[5]);
	glBufferData(GL_ARRAY_BUFFER, obj[5].vertices.size() * sizeof(Vertex), &obj[5].vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID[5]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj[5].indices.size() * sizeof(unsigned int), &obj[5].indices[0], GL_STATIC_DRAW);

	// vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	//texture position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);

	//normal position
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	//spacecraft texture
	SpaceCraft[0].setupTexture("resource/texture/spacecraftTexture.bmp");
	SpaceCraft[1].setupTexture("resource/texture/gold.bmp");

	// skybox
	glBindVertexArray(vaoID[6]);

	glBindBuffer(GL_ARRAY_BUFFER, vboID[6]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cube), cube, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID[6]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeindices), cubeindices, GL_STATIC_DRAW);

	//vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
	glEnableVertexAttribArray(0);

	glBindVertexArray(0);

	//skybox texture
	std::vector<std::string> faces =
	{
		"resource/texture/skybox/right.bmp",
		"resource/texture/skybox/left.bmp",
		"resource/texture/skybox/bottom.bmp",
		"resource/texture/skybox/top.bmp",
		"resource/texture/skybox/back.bmp",
		"resource/texture/skybox/front.bmp"
	};
	skyboxtex = loadCubemap(faces);


	//pineapple
	obj[7] = loadOBJ("resource/10200_Pineapple_v1-L2.obj");

	glBindVertexArray(vaoID[7]);

	glBindBuffer(GL_ARRAY_BUFFER, vboID[7]);
	glBufferData(GL_ARRAY_BUFFER, obj[7].vertices.size() * sizeof(Vertex), &obj[7].vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID[7]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj[7].indices.size() * sizeof(unsigned int), &obj[7].indices[0], GL_STATIC_DRAW);

	// vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	//texture position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);

	//normal position
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	//pineapple texture
	Food[1].setupTexture("resource/texture/10200_Pineapple.jpg");


	//apple
	obj[8] = loadOBJ("resource/apple.obj");

	glBindVertexArray(vaoID[8]);

	glBindBuffer(GL_ARRAY_BUFFER, vboID[8]);
	glBufferData(GL_ARRAY_BUFFER, obj[8].vertices.size() * sizeof(Vertex), &obj[8].vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID[8]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj[8].indices.size() * sizeof(unsigned int), &obj[8].indices[0], GL_STATIC_DRAW);

	// vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	//texture position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);

	//normal position
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	//apple texture
	Food[2].setupTexture("resource/texture/PG.jfif");

	//banana
	obj[9] = loadOBJ("resource/banana.obj");

	glBindVertexArray(vaoID[9]);

	glBindBuffer(GL_ARRAY_BUFFER, vboID[9]);
	glBufferData(GL_ARRAY_BUFFER, obj[9].vertices.size() * sizeof(Vertex), &obj[9].vertices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboID[9]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, obj[9].indices.size() * sizeof(unsigned int), &obj[9].indices[0], GL_STATIC_DRAW);

	// vertex position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(0);

	//texture position
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
	glEnableVertexAttribArray(1);

	//normal position
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2);

	glBindVertexArray(0);

	//banana texture
	Food[3].setupTexture("resource/texture/1200px-ICS_Quebec.svg.png");

}

void createrockmodel() {
	// create rock model
	srand(time(NULL));
	float radius = 12.0f;
	float offset = 0.8f;
	float displacement;
	for (int i = 0; i < Amount; i++) {
		glm::mat4 model = glm::mat4(1.0f);
		float angle = (float)i / (float)Amount * 360.0f;
		displacement = (rand() % (int)(2 * offset * 200)) / 100.0f - offset;
		float x = sin(angle) * radius + displacement;
		displacement = (rand() % (int)(2 * offset * 200)) / 100.0f - offset;
		float y = displacement * offset + 1;
		displacement = (rand() % (int)(2 * offset * 200)) / 100.0f - offset;
		float z = cos(angle) * radius + displacement;
		//std::cout << x << " " << y << " " << z << std::endl;
		model = glm::translate(model, glm::vec3(x, y, z));
		float scale = (rand() % 10) / 50.0f + 0.1;
		model = glm::scale(model, glm::vec3(scale));
		float rotateangle = (rand() % 360);
		model = glm::rotate(model, glm::radians(rotateangle), glm::vec3(0.4f, 0.6f, 0.8f));
		rockmodel[i] = model;
	}
}

void setfood() {
	for (int i = 0; i < F; i++)
		food[i] = true;
}

void initializedGL(void) //run only once
{
	if (glewInit() != GLEW_OK) {
		std::cout << "GLEW not OK." << std::endl;
	}

	get_OpenGL_info();

	sendDataToOpenGL();

	createrockmodel();

	setfood();

	//set up the vertex shader and fragment shader
	shading.setupShader("VertexShaderCode.glsl", "FragmentShaderCode.glsl");
	lightshading.setupShader("LightVertexShader.glsl", "LightFragmentShader.glsl");
	skyboxshading.setupShader("SkyboxVertexShader.glsl", "SkyboxFragmentShader.glsl");


	glEnable(GL_DEPTH_TEST);
}

void Renderthescene(Shader& tempshader) {
	//render the scene

	//spacecraft
	glm::vec3 SC_local_pos(0.0f, 0.0f, 0.0f);
	glm::vec3 SC_local_front(0.0f, 0.0f, -1.0f);
	glm::vec3 SC_local_right(1.0f, 0.0f, 0.0f);
	float scale = 0.0005;
	glm::mat4 SC_scale_M = glm::scale(glm::mat4(1.0f), glm::vec3(scale));
	glm::mat4 SC_Rot_M = glm::rotate(glm::mat4(1.0f), glm::radians(spacecraftangle), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 SC_trans_M = glm::translate(glm::mat4(1.0f), glm::vec3(SCInitialPos[0] + SCTranslation[0], SCInitialPos[1] + SCTranslation[1], SCInitialPos[2] + SCTranslation[2]));
	glm::mat4 tempmodel = SC_trans_M * SC_Rot_M;
	glm::mat4 model = SC_trans_M * SC_Rot_M * SC_scale_M * glm::rotate(glm::mat4(1.0f), glm::radians(180.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::vec3 SC_world_pos = model * glm::vec4(SC_local_pos, 1.0f);
	SC_world_Front_Direction = SC_Rot_M * glm::vec4(SC_local_front, 1.0f);
	SC_world_Right_Direction = SC_Rot_M * glm::vec4(SC_local_right, 1.0f);
	SC_world_Front_Direction = glm::normalize(SC_world_Front_Direction);
	SC_world_Right_Direction = glm::normalize(SC_world_Right_Direction);

	//view and projection
	cameraposition = tempmodel * glm::vec4(glm::vec3(0.0f, 0.5f, 0.8f), 1.0f);
	camerafrontvector = tempmodel * glm::vec4(glm::vec3(0.0f, 0.0f, -0.8f), 1.0f);
	view = glm::lookAt(cameraposition, camerafrontvector, cameraupvector);
	tempshader.setMat4("view", view);
	projection = glm::mat4(1.0f);
	projection = glm::perspective(glm::radians(fov), (float)800 / (float)600, 0.1f, 1000.0f);
	tempshader.setMat4("projection", projection);
	tempshader.setMat4("model", model);

	//spotlight
	spotlightdir = camerafrontvector - cameraposition + glm::vec3(0.0f, 0.3f, 0.0f);
	tempshader.setVec3("slight.position", cameraposition);
	tempshader.setVec3("slight.direction", spotlightdir);

	SpaceCraft[planeindex].bind(0);
	glBindVertexArray(vaoID[5]);
	glDrawElements(GL_TRIANGLES, obj[5].indices.size(), GL_UNSIGNED_INT, 0);
	SpaceCraft[planeindex].unbind();


	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(x1+offset, 0.0f, -50.0f));
	model = glm::rotate(model, glm::radians(selfrotating), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.2f));
	tempshader.setMat4("model", model);
	Craft[alienvehiculeindex[0]].bind(0);
	glBindVertexArray(vaoID[1]);
	glDrawElements(GL_TRIANGLES, obj[1].indices.size(), GL_UNSIGNED_INT, 0);
	Craft[alienvehiculeindex[0]].unbind();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(x2+offset, 0.0f, -100.0f));
	model = glm::rotate(model, glm::radians(selfrotating), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.2f));
	tempshader.setMat4("model", model);
	Craft[alienvehiculeindex[1]].bind(0);
	glBindVertexArray(vaoID[1]);
	glDrawElements(GL_TRIANGLES, obj[1].indices.size(), GL_UNSIGNED_INT, 0);
	Craft[alienvehiculeindex[1]].unbind();

	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(x3+offset, 0.0f, -150.0f));
	model = glm::rotate(model, glm::radians(selfrotating), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(1.2f, 1.2f, 1.2f));
	tempshader.setMat4("model", model);
	Craft[alienvehiculeindex[2]].bind(0);
	glBindVertexArray(vaoID[1]);
	glDrawElements(GL_TRIANGLES, obj[1].indices.size(), GL_UNSIGNED_INT, 0);
	Craft[alienvehiculeindex[2]].unbind();

	//planet
	model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(20.0f, 0.0f, -200.0f));
	model = glm::rotate(model, glm::radians(selfrotating), glm::vec3(0.0f, 1.0f, 0.0f));
	model = glm::scale(model, glm::vec3(2.0f, 2.0f, 2.0f));
	tempshader.setMat4("model", model);
	tempshader.setInt("normalmap_flag",1);
	Planet[0].bind(0);
    Planet[1].bind(1);
	glBindVertexArray(vaoID[3]);
	glDrawElements(GL_TRIANGLES, obj[3].indices.size(), GL_UNSIGNED_INT, 0);
	Planet[0].unbind();
	Planet[1].unbind();
	tempshader.setInt("normalmap_flag", 0);

	//rock
	glm::mat4 temp;
	temp = glm::mat4(1.0f);
	temp = glm::translate(temp, glm::vec3(20.0f, 0.0f, -200.0f));
	temp = glm::rotate(temp, glm::radians(selfrotating), glm::vec3(0.0f, 1.0f, 0.0f));
	for (int i = 0; i < Amount; i++) {
		model = glm::mat4(1.0f);
		model = temp * rockmodel[i];
		tempshader.setMat4("model", model);
		glm::vec3 dis = glm::vec3(model * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f))-cameraposition;
		if (glm::sqrt(glm::dot(dis, dis)) < 5.0f)
		{
			if (RockNum[i] == 1)
			{
				GoldNum--;
			}
			RockNum[i] = 0;
		}
		Rock[RockNum[i]].bind(0);
		glBindVertexArray(vaoID[4]);
		glDrawElements(GL_TRIANGLES, obj[4].indices.size(), GL_UNSIGNED_INT, 0);
		Rock[1].unbind();
	}

	//food1
	if (food[0]) {
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(8.0f, 3.0f, -50.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.008f, 0.008f, 0.008f));
		tempshader.setMat4("model", model);
		Food[0].bind(0);
		glBindVertexArray(vaoID[2]);
		glDrawElements(GL_TRIANGLES, obj[2].indices.size(), GL_UNSIGNED_INT, 0);
		Food[0].unbind();
	}
	//food2
	if (food[1]) {
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(13.0f, 3.0f, -100.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.008f, 0.008f, 0.008f));
		tempshader.setMat4("model", model);
		Food[0].bind(0);
		glBindVertexArray(vaoID[2]);
		glDrawElements(GL_TRIANGLES, obj[2].indices.size(), GL_UNSIGNED_INT, 0);
		Food[0].unbind();
	}
	//food3
	if (food[2]) {
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(18.0f, 3.0f, -150.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.008f, 0.008f, 0.008f));
		tempshader.setMat4("model", model);
		Food[0].bind(0);
		glBindVertexArray(vaoID[2]);
		glDrawElements(GL_TRIANGLES, obj[2].indices.size(), GL_UNSIGNED_INT, 0);
		Food[0].unbind();
	}

	//food4
	if (food[3]) {
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(5.0f, 2.0f, -40.0f));
		model = glm::rotate(model, glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.1f, 0.1f, 0.1f));
		tempshader.setMat4("model", model);
		Food[1].bind(0);
		glBindVertexArray(vaoID[7]);
		glDrawElements(GL_TRIANGLES, obj[7].indices.size(), GL_UNSIGNED_INT, 0);
		Food[1].unbind();
	}
	//food5
	if (food[4]) {
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(10.0f, 2.0f, -90.0f));
		model = glm::scale(model, glm::vec3(0.008f, 0.008f, 0.008f));
		tempshader.setMat4("model", model);
		Food[2].bind(0);
		glBindVertexArray(vaoID[8]);
		glDrawElements(GL_TRIANGLES, obj[8].indices.size(), GL_UNSIGNED_INT, 0);
		Food[2].unbind();
	}
	//food6
	if (food[5]) {
		model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(15.0f, 2.0f, -140.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(0.2f, 0.2f, 0.2f));
		tempshader.setMat4("model", model);
		Food[3].bind(0);
		glBindVertexArray(vaoID[9]);
		glDrawElements(GL_TRIANGLES, obj[9].indices.size(), GL_UNSIGNED_INT, 0);
		Food[3].unbind();
	}
}

void checkalienvehicle() {
	glm::vec3 first = glm::vec3(x1+offset, 0.0f, -50.0f) - cameraposition;
	if (glm::sqrt(glm::dot(first, first)) <= 10.0f)
		alienvehiculeindex[0] = 1;
	glm::vec3 second = glm::vec3(x2+offset, 0.0f, -100.0f) - cameraposition;
	if (glm::sqrt(glm::dot(second, second)) <= 10.0f)
		alienvehiculeindex[1] = 1;
	glm::vec3 third = glm::vec3(x3+offset, 0.0f, -150.0f) - cameraposition;
	if (glm::sqrt(glm::dot(third, third)) <= 10.0f)
		alienvehiculeindex[2] = 1;
}
void checkfood() {
	glm::vec3 first = glm::vec3(8.0f, 3.0f, -50.0f) - cameraposition;
	if (glm::sqrt(glm::dot(first, first)) <= 5.0f)
	{
		food[0] = false;
		FoodNum--;
	}
	glm::vec3 second = glm::vec3(13.0f, 3.0f, -100.0f) - cameraposition;
	if (glm::sqrt(glm::dot(second, second)) <= 5.0f)
	{
		food[1] = false;FoodNum--;
	}
	glm::vec3 third = glm::vec3(18.0f, 3.0f, -150.0f) - cameraposition;
	if (glm::sqrt(glm::dot(third, third)) <= 5.0f)
	{
		food[2] = false; 
		FoodNum--;
	}
	glm::vec3 four = glm::vec3(5.0f, 2.0f, -40.0f) - cameraposition;
	if (glm::sqrt(glm::dot(four, four)) <= 5.0f)
	{
		food[3] = false;
		FoodNum--;
	}
	glm::vec3 five = glm::vec3(10.0f, 2.0f, -90.0f) - cameraposition;
	if (glm::sqrt(glm::dot(five, five)) <= 5.0f)
	{
		food[4] = false;
		FoodNum--;
	}
	glm::vec3 six = glm::vec3(15.0f, 2.0f, -140.0f) - cameraposition;
	if (glm::sqrt(glm::dot(six, six)) <= 5.0f)
	{
		food[5] = false;
		FoodNum--;
	}
}

void checkplane() {
	if (GoldNum<=0 && (FoodNum<=0))
	{
		planeindex = 1;
	}
}

void paintGL(void)  //always run
{
	glViewport(0, 0, 1600, 1200);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDepthMask(GL_FALSE);

	skyboxshading.use();
	if (offset >= 30.0f || offset <= -30.0f) {
		speed = -speed; 
	}
	offset += speed;
	//view
	glm::mat4 fview;
	fview = glm::mat4(1.0f);
	fview = glm::lookAt(cameraposition, camerafrontvector, cameraupvector);
	view = glm::mat4(glm::mat3(fview));
	skyboxshading.setMat4("view", view);

	//projection
	projection = glm::mat4(1.0f);
	projection = glm::perspective(glm::radians(fov), (float)800 / (float)600, 0.1f, 1000.0f);
	skyboxshading.setMat4("projection", projection);

	glBindVertexArray(vaoID[6]);
	glBindTexture(GL_TEXTURE_CUBE_MAP, skyboxtex);
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);

	glDepthMask(GL_TRUE);

	//normal scene render

	// glEnable(GL_BLEND);
	// glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
	// glBlendColor(1.0, 1.0, 1.0, 0.5);
	lightshading.use();

	//material for all objects
	lightshading.setVec3("m.specular", 1.0, 1.0, 1.0);
	lightshading.setVec3("viewpos", cameraposition);
	lightshading.setFloat("m.shiny", 32.0f);

	//direction light
	lightshading.setVec3("dlight.direction", 0.1f, -1.0f, -0.5f);
	lightshading.setVec3("dlight.ambient", 0.05f, 0.05f, 0.05f);
	lightshading.setVec3("dlight.diffuse", 0.6f, 0.6f, 0.6f);
	lightshading.setVec3("dlight.specular", 0.3f, 0.3f, 0.3f);
	lightshading.setFloat("dlight.intensity", dlightintensity);

	//point light
	lightshading.setVec3("plight[0].position", 0.0f, 8.0f, -45.0f);
	lightshading.setVec3("plight[0].ambient", 0.05f, 0.05f, 0.05f);
	lightshading.setVec3("plight[0].diffuse", plightone, plightone, plightone);
	lightshading.setVec3("plight[0].specular", 0.15f, 0.15f, 0.15f);
	lightshading.setFloat("plight[0].constant", 0.8f);
	lightshading.setFloat("plight[0].linear", 0.1f);
	lightshading.setFloat("plight[0].quadratic", 0.07f);

	lightshading.setVec3("plight[1].position", 5.0f, 8.0f, -95.0f);
	lightshading.setVec3("plight[1].ambient", 0.05f, 0.05f, 0.05f);
	lightshading.setVec3("plight[1].diffuse", plighttwo, plighttwo, plighttwo);
	lightshading.setVec3("plight[1].specular", 0.1f, 0.1f, 0.1f);
	lightshading.setFloat("plight[1].constant", 0.8f);
	lightshading.setFloat("plight[1].linear", 0.1f);
	lightshading.setFloat("plight[1].quadratic", 0.07f);

	lightshading.setVec3("plight[2].position", 10.0f, 8.0f, -145.0f);
	lightshading.setVec3("plight[2].ambient", 0.05f, 0.05f, 0.05f);
	lightshading.setVec3("plight[2].diffuse", 0.7f, 0.7f, 0.7f);
	lightshading.setVec3("plight[2].specular", 0.25f, 0.25f, 0.25f);
	lightshading.setFloat("plight[2].constant", 0.8f);
	lightshading.setFloat("plight[2].linear", 0.1f);
	lightshading.setFloat("plight[2].quadratic", 0.07f);

	lightshading.setVec3("plight[3].position", 20.0f, 5.0f, -195.0f);
	lightshading.setVec3("plight[3].ambient", 0.15f, 0.15f, 0.15f);
	lightshading.setVec3("plight[3].diffuse", 0.9f, 0.9f, 0.9f);
	lightshading.setVec3("plight[3].specular", 0.15f, 0.15f, 0.15f);
	lightshading.setFloat("plight[3].constant", 0.2f);
	lightshading.setFloat("plight[3].linear", 0.1f);
	lightshading.setFloat("plight[3].quadratic", 0.07f);

	//spot light
	lightshading.setVec3("slight.position", cameraposition);
	lightshading.setVec3("slight.direction", spotlightdir);
	lightshading.setVec3("slight.ambient", 0.0f, 0.0f, 0.0f);
	lightshading.setVec3("slight.diffuse", 1.0f, 1.0f, 1.0f);
	lightshading.setVec3("slight.specular", 1.0f, 1.0f, 1.0f);
	lightshading.setFloat("slight.constant", 0.8f);
	lightshading.setFloat("slight.linear", 0.09f);
	lightshading.setFloat("slight.quadratic", 0.032f);
	lightshading.setFloat("slight.cutoff", glm::cos(glm::radians(12.5f)));
	lightshading.setFloat("slight.outercutoff", glm::cos(glm::radians(15.0f)));

	lightshading.setInt("normalmap_flag", 0);

	//view
	view = glm::mat4(1.0f);
	view = glm::lookAt(cameraposition, camerafrontvector, cameraupvector);
	lightshading.setMat4("view", view);

	//projection
	projection = glm::mat4(1.0f);
	projection = glm::perspective(glm::radians(fov), (float)800 / (float)600, 0.1f, 1000.0f);
	lightshading.setMat4("projection", projection);

	Renderthescene(lightshading);

}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, width, height);
}

void cursor_position_callback(GLFWwindow* window, double xpos, double ypos)
{
	//adjust camera position and direction follow with the spacecraft
	if (first) {
		oldx = xpos;
		first = false;
	}
	else {
		if (xpos < oldx)
			spacecraftangle += 2.0f;
		if (xpos > oldx)
			spacecraftangle -= 2.0f;
		oldx = xpos;
	}
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	// Sets the scoll callback for the current window.
	//zoom in and zoom out
	fov -= (float)yoffset;
	if (fov < 1.0f)
		fov = 1.0f;
	if (fov > 45.0f)
		fov = 45.0f;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
		//increase environment light intensity
		if (dlightintensity < 2.5f)
			dlightintensity += 0.1f;
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
		//decrease enviroment light intensity
		if (dlightintensity > 0.0f)
			dlightintensity -= 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
		//increase point light 1 intensity
		if (plightone < 2.5f)
			plightone += 0.1f;
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
		//decrease point light 1 intensity
		if (plightone > 0.0f)
			plightone -= 0.1f;
	}

	if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
		//increase point light 1 intensity
		if (plighttwo < 2.5f)
			plighttwo += 0.1f;
	}
	if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
		//decrease point light 1 intensity
		if (plighttwo > 0.0f)
			plighttwo -= 0.1f;
	}

	//adjust the position of spacecraft
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
		SCTranslation[0] = SCTranslation[0] + 0.5 * SC_world_Front_Direction[0];
		SCTranslation[2] = SCTranslation[2] + 0.5 * SC_world_Front_Direction[2];
	}

	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
		SCTranslation[0] = SCTranslation[0] - 0.5 * SC_world_Front_Direction[0];
		SCTranslation[2] = SCTranslation[2] - 0.5 * SC_world_Front_Direction[2];
	}

	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
		SCTranslation[0] = SCTranslation[0] - 0.5 * SC_world_Right_Direction[0];
		SCTranslation[2] = SCTranslation[2] - 0.5 * SC_world_Right_Direction[2];
	}

	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
		SCTranslation[0] = SCTranslation[0] + 0.5 * SC_world_Right_Direction[0];
		SCTranslation[2] = SCTranslation[2] + 0.5 * SC_world_Right_Direction[2];
	}

	checkalienvehicle();
	checkfood();
	checkplane();
}


int main(int argc, char* argv[])
{
	GLFWwindow* window;

	/* Initialize the glfw */
	if (!glfwInit()) {
		std::cout << "Failed to initialize GLFW" << std::endl;
		return -1;
	}
	/* glfw: configure; necessary for MAC */
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Assignment 2", NULL, NULL);
	if (!window) {
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return -1;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	/*register callback functions*/
	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetKeyCallback(window, key_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	glEnable(GL_DEPTH_TEST);

	if (GLEW_OK != glewInit()) {
		std::cout << "Failed to initialize GLEW" << std::endl;
		return -1;
	}

	initializedGL();

	while (!glfwWindowShouldClose(window)) {

		//time & angle changing
		float currentFrame = (float)glfwGetTime();
		dtime = currentFrame - lframe;
		lframe = currentFrame;

		//self rotation
		selfrotating += 0.1f;
		if (selfrotating >= 360)
			selfrotating -= 360;

		/* Render here */
		paintGL();

		/* Swap front and back buffers */
		glfwSwapBuffers(window);

		/* Poll for and process events */
		glfwPollEvents();
	}

	glfwTerminate();

	return 0;
}






