//=============================================================================================
// Mintaprogram: Z�ld h�romsz�g. Ervenyes 2019. osztol.
//
// A beadott program csak ebben a fajlban lehet, a fajl 1 byte-os ASCII karaktereket tartalmazhat, BOM kihuzando.
// Tilos:
// - mast "beincludolni", illetve mas konyvtarat hasznalni
// - faljmuveleteket vegezni a printf-et kiveve
// - Mashonnan atvett programresszleteket forrasmegjeloles nelkul felhasznalni es
// - felesleges programsorokat a beadott programban hagyni!!!!!!! 
// - felesleges kommenteket a beadott programba irni a forrasmegjelolest kommentjeit kiveve
// ---------------------------------------------------------------------------------------------
// A feladatot ANSI C++ nyelvu forditoprogrammal ellenorizzuk, a Visual Studio-hoz kepesti elteresekrol
// es a leggyakoribb hibakrol (pl. ideiglenes objektumot nem lehet referencia tipusnak ertekul adni)
// a hazibeado portal ad egy osszefoglalot.
// ---------------------------------------------------------------------------------------------
// A feladatmegoldasokban csak olyan OpenGL fuggvenyek hasznalhatok, amelyek az oran a feladatkiadasig elhangzottak 
// A keretben nem szereplo GLUT fuggvenyek tiltottak.
//
// NYILATKOZAT
// ---------------------------------------------------------------------------------------------
// Nev    : 
// Neptun : 
// ---------------------------------------------------------------------------------------------
// ezennel kijelentem, hogy a feladatot magam keszitettem, es ha barmilyen segitseget igenybe vettem vagy
// mas szellemi termeket felhasznaltam, akkor a forrast es az atvett reszt kommentekben egyertelmuen jeloltem.
// A forrasmegjeloles kotelme vonatkozik az eloadas foliakat es a targy oktatoi, illetve a
// grafhazi doktor tanacsait kiveve barmilyen csatornan (szoban, irasban, Interneten, stb.) erkezo minden egyeb
// informaciora (keplet, program, algoritmus, stb.). Kijelentem, hogy a forrasmegjelolessel atvett reszeket is ertem,
// azok helyessegere matematikai bizonyitast tudok adni. Tisztaban vagyok azzal, hogy az atvett reszek nem szamitanak
// a sajat kontribucioba, igy a feladat elfogadasarol a tobbi resz mennyisege es minosege alapjan szuletik dontes.
// Tudomasul veszem, hogy a forrasmegjeloles kotelmenek megsertese eseten a hazifeladatra adhato pontokat
// negativ elojellel szamoljak el es ezzel parhuzamosan eljaras is indul velem szemben.
//=============================================================================================
#include "framework.h"

// vertex shader in GLSL: It is a Raw string (C++11) since it contains new line characters
const char * const vertexSource = R"(
	#version 330

	uniform mat4 MVP;
	layout(location = 0) in vec2 vtxPos;
	layout(location = 1) in vec2 vtxUV;
	out vec2 texcoord;

	void main() {
		gl_Position = vec4(vtxPos, 0, 1) * MVP;
		texcoord = vtxUV;
	}
)";

// fragment shader in GLSL
const char * const fragmentSource = R"(
	#version 330
	
	uniform sampler2D samplerUnit;
	in vec2 texcoord;
	out vec4 fragmentColor;

	void main() {
		fragmentColor = texture(samplerUnit, texcoord); 
	}
)";

GPUProgram gpuProgram; // vertex and fragment shaders

class Camera2D {
	vec2 wCenter;
	vec2 wSize;
public:
	Camera2D(vec2 center, vec2 size) {
		this->wCenter = center;
		this->wSize = size;
	}
	vec2 GetCenter() { return wCenter; }
	vec2 GetSize() { return wSize; }
	mat4 V() { return TranslateMatrix(-wCenter); }
	mat4 P() {
		return ScaleMatrix(vec2(2 / wSize.x, 2 / wSize.y));
	}
	mat4 Vinv() {
		return TranslateMatrix(wCenter);
	}
	mat4 Pinv() {
		return ScaleMatrix(vec2(wSize.x / 2, wSize.y / 2));
	}
	void Zoom(float s) { wSize = wSize * s; }
	void Pan(vec2 t) { wCenter = wCenter + t; }

	vec2 ToWorldCoordinates(int pX, int pY) {
		float cX = 2.0f * pX / windowWidth - 1;
		float cY = 1.0f - 2.0f * pY / windowHeight;
		vec4 vec = vec4(cX, cY, 0, 1);
		vec = vec * this->Pinv() * this->Vinv();
		return vec2(vec.x, vec.y);
	}
};

Camera2D* camera;


class Circle {
	vec3 center;
	float radius;
public:
	Circle() {}
	Circle(vec3 center, float rad) {
		this->center = center;
		this->radius = radius;
	}
	void CalcCircle(vec3 firstPoint) {
		vec3 secondPoint = firstPoint / dot(firstPoint, firstPoint);
		center = (firstPoint + secondPoint) / 2;
		radius = length(firstPoint - secondPoint) / 2;
	}

	bool Contains(vec3 point) {
		
		return length(point - center) < radius;
	}
};

class CircleCollection {
	std::vector<Circle> circles;
public:
	CircleCollection() {}
	
	void Add(Circle c) {
		circles.push_back(c);
	}

	int Size() {
		return circles.size();
	}

	std::vector<Circle> GetCircles() {
		return circles;
	}

	Circle At(int idx) {
		return circles.at(idx);
	}

};

CircleCollection* circleCollection;


class PoincareTexture {
public:
	unsigned int textureId;
	std::vector<vec4> image;

	PoincareTexture(int width, int height, std::vector<vec4>& image) {
		textureId = 0;
		this->image = image;
		UploadTexture(width, height, image);
	}
	void UploadTexture(int width, int height, std::vector<vec4>& image) {
		glGenTextures(1, &textureId);
		glBindTexture(GL_TEXTURE_2D, textureId); 
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_FLOAT, &image[0]); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	void CreateImage() {
		vec3 startingPoint(0, 0, 1);
		float degree = 0;
		vec3 v0(cos(degree), sin(degree), 0);
		float t[] = { 0.5, 1.5, 2.5, 3.5, 4.5, 5.5 };
		std::vector<vec3> hyperbolic;
		for (int j = 0; j < 9; j++) {
			degree = j * 40 * M_PI / 180;
			v0 = vec3(cos(degree), sin(degree), 0);
			for (int i = 0; i < 6; i++) {
				vec3 r = startingPoint * cosh(t[i]) + v0 * sinh(t[i]);
				hyperbolic.push_back(r);
			}
		}

		
		for (int i = 0; i < hyperbolic.size(); i++) {
			vec3 r = vec3(hyperbolic[i].x / (hyperbolic[i].z + 1), hyperbolic[i].y / (hyperbolic[i].z + 1), 0);
			Circle circle;
			circle.CalcCircle(r);
			circleCollection->Add(circle);
		}
	}
};




class Star {
public:
	std::vector<vec2> vtxs, uvs;
	unsigned int vao, vbo[2];
	PoincareTexture* tex;
	int s;

	Star(PoincareTexture* texture) {
		this->tex = texture;
		s = 40;
		Create();
	}

	void Increment() {
		s += 10;
	}

	void Decrement() {
		s -= 10;
	}

	void Create() {

		glGenVertexArrays(1, &vao);
		glBindVertexArray(vao);
		glGenBuffers(2, vbo);

		MakeVTX();
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, NULL);

		MakeUVS();
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, NULL);


	}

	void MakeVTX() {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
		vtxs.push_back(vec2(50, 30));
		vtxs.push_back(vec2(50 + s, 30));
		vtxs.push_back(vec2(50 + 40, 30 - 40));
		vtxs.push_back(vec2(50, 30 - s));
		vtxs.push_back(vec2(50 - 40, 30 - 40));
		vtxs.push_back(vec2(50 - s, 30));
		vtxs.push_back(vec2(50 - 40, 30 + 40));
		vtxs.push_back(vec2(50, 30 + s));
		vtxs.push_back(vec2(50 + 40, 30 + 40));
		vtxs.push_back(vec2(50 + s, 30));
		glBufferData(GL_ARRAY_BUFFER, vtxs.size() * sizeof(vec3), &vtxs[0], GL_STATIC_DRAW);
	}

	void MakeUVS() {
		glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
		uvs.push_back(vec2(0.5, 0.5));
		uvs.push_back(vec2(0.5, 1));
		uvs.push_back(vec2(1, 1));
		uvs.push_back(vec2(1, 0.5));
		uvs.push_back(vec2(1, 0));
		uvs.push_back(vec2(0.5, 0));
		uvs.push_back(vec2(0, 0));
		uvs.push_back(vec2(0, 0.5));
		uvs.push_back(vec2(0, 1));
		uvs.push_back(vec2(0.5, 1));
		glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0], GL_STATIC_DRAW);
	}

	void ClearVTX() {
		vtxs.clear();
	}

	void ClearUVS() {
		uvs.clear();
	}

	void Draw() {
		glBindVertexArray(vao);
		mat4 MVP = camera->V() * camera->P();
		gpuProgram.setUniform(MVP, "MVP");

		int sampler = 0; // which sampler unit should be used
		int location = glGetUniformLocation(gpuProgram.getId(),
			"samplerUnit");
		glUniform1i(location, sampler);
		glActiveTexture(GL_TEXTURE0 + sampler); // = GL_TEXTURE0
		glBindTexture(GL_TEXTURE_2D, tex->textureId);
		glDrawArrays(GL_TRIANGLE_FAN, 0, vtxs.size());
	}
};

int GetCircleCount(CircleCollection* circles, int x, int y) {
	int circleCount = 0;

	for (int i = 0; i < circles->Size(); i++) {
		vec3 kurva = vec3(((x / 100.0) - 0.5) * 2, ((y / 100.0) - 0.5) * 2, 0);
			
		if (circles->At(i).Contains(kurva)) {
			circleCount++;
		}
		
	}
	return circleCount;
}


PoincareTexture* tex;
Star* star;
// Initialization, create an OpenGL context
void onInitialization() {
	
	glViewport(0, 0, windowWidth, windowHeight);
	glPointSize(10);

	circleCollection = new CircleCollection;
	tex->CreateImage();
	std::vector<vec4> image;
	//circleCollection->Add(Circle(vec3(0.5, 0.5, 0), 0.5));
	Circle base(vec3(0, 0, 0), 0.5);
	vec3 kurva;
	for (int i = 0; i < 100; i++) {
		for (int j = 0; j < 100; j++) {
			kurva = vec3(((i / 100.0) - 0.5) * 2, ((j / 100.0) - 0.5) * 2, 0);
			if (base.Contains(kurva)) {
				image.push_back(vec4(0, 0, 0, 0));
				printf("asd");
			}
			else {

				int circleCount = GetCircleCount(circleCollection, i, j);
				if (circleCount % 2 == 0) image.push_back(vec4(1, 1, 0, 0));
				else image.push_back(vec4(0, 0, 1, 0));
			}
			
		}
	}

	tex = new PoincareTexture(100, 100, image);
	camera = new Camera2D(vec2(20, 30), vec2(150, 150));
	star = new Star(tex);

	// create program for the GPU
	gpuProgram.create(vertexSource, fragmentSource, "outColor");

}

// Window has become invalid: Redraw
void onDisplay() {
	glClearColor(0, 0, 0, 0);     // background color
	glClear(GL_COLOR_BUFFER_BIT); // clear frame buffer


	star->Draw();
	

	glutSwapBuffers(); // exchange buffers for double buffering
}

// Key of ASCII code pressed
void onKeyboard(unsigned char key, int pX, int pY) {
	if (key == 'd') glutPostRedisplay();         // if d, invalidate display, i.e. redraw
	switch (key) {
	case 'h':
		star->Decrement();
		star->ClearVTX();
		star->MakeVTX();
		glutPostRedisplay();
		break;
	case 'H':
		star->Increment();
		star->ClearVTX();
		star->MakeVTX();
		glutPostRedisplay();
		break;
	}
}

// Key of ASCII code released
void onKeyboardUp(unsigned char key, int pX, int pY) {
}

// Move mouse with key pressed
void onMouseMotion(int pX, int pY) {	// pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;
	printf("Mouse moved to (%3.2f, %3.2f)\n", cX, cY);
}

// Mouse click event
void onMouse(int button, int state, int pX, int pY) { // pX, pY are the pixel coordinates of the cursor in the coordinate system of the operation system
	// Convert to normalized device space
	float cX = 2.0f * pX / windowWidth - 1;	// flip y axis
	float cY = 1.0f - 2.0f * pY / windowHeight;

	char * buttonStat;
	switch (state) {
	case GLUT_DOWN: buttonStat = "pressed"; break;
	case GLUT_UP:   buttonStat = "released"; break;
	}

	switch (button) {
	case GLUT_LEFT_BUTTON:   printf("Left button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);   break;
	case GLUT_MIDDLE_BUTTON: printf("Middle button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY); break;
	case GLUT_RIGHT_BUTTON:  printf("Right button %s at (%3.2f, %3.2f)\n", buttonStat, cX, cY);  break;
	}
}

// Idle event indicating that some time elapsed: do animation here
void onIdle() {
	long time = glutGet(GLUT_ELAPSED_TIME); // elapsed time since the start of the program
}
