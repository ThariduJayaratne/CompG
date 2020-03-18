#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <RayTriangleIntersection.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>

using namespace std;

using namespace glm;

#define WIDTH 1000
#define HEIGHT 1000
#define PI 3.14159265
#define SCALETING 0.5

void draw();
void greyscale();
void update();
void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
mat3 camOrien = mat3();
vec3 camera = vec3(0,0,6);

vector<Colour> readmat(){
  ifstream myfile;
  myfile.open("cornell-box.mtl");
  vector<string> colors;
  vector<vec3> rgb;
  vector<Colour> result;
  std::string line;
  while (getline(myfile, line)) {
    if(line[0] == 'n'){
      string color = line.substr(7,line.size());
      colors.push_back(color);
    }
    if(line[0] == 'K'){
      string values = line.substr(2,line.size());
      string red = line.substr(2,10);
      string green = line.substr(11,19);
      string blue = line.substr(20,line.size());
      float r = stof(red) * 255;
      float g = stof(green) * 255;
      float b = stof(blue) * 255;
      rgb.push_back(vec3(r,g,b));
    }
  }
  for(u_int i = 0; i<colors.size(); i++){
    Colour test;
    test.name = colors[i];
    test.red = rgb[i].x;
    test.green = rgb[i].y;
    test.blue = rgb[i].z;
    result.push_back(test);
  }
  return result;
}

vector<vec3> readVertices(float scale){
  ifstream myfile;
  myfile.open("cornell-box.obj");
  std::string line;
  vector<vec3> vertices;
  while (getline(myfile, line)) {
    std::string* lineX = split(line,' ');
    if(line[0] == 'v'){
      float xc = stof(lineX[1])*scale;
      float yc = stof(lineX[2])*scale;
      float zc = stof(lineX[3])*scale;
      vertices.push_back(vec3(xc,yc,zc));
    }
  }
  return vertices;
}

vector<ModelTriangle> readtriangles(){
  fstream myfile;
  myfile.open("cornell-box.obj");
  std::string line;
  vector<ModelTriangle> triangles;
  vector<vec3> readvertices = readVertices(SCALETING);
  Colour colour;
  vector<Colour> colours = readmat();
  while (getline(myfile, line)) {
    string *splitLine = split(line, ' ');
    if(splitLine[0] == "usemtl") {
      string readColour = splitLine[1];
      for (u_int i = 0; i < colours.size(); i++) {
        if(readColour == colours[i].name) colour = colours[i];
      }
    }
    if(splitLine[0] == "f"){
      int slashpos1 =  splitLine[1].find('/');
      int slashpos2 =  splitLine[2].find('/');
      int slashpos3 =  splitLine[3].find('/');
      string point1 = splitLine[1].substr(0,slashpos1);
      string point2 = splitLine[2].substr(0,slashpos2);
      string point3 = splitLine[3].substr(0,slashpos3);
      float p1 = stof(point1) - 1;
      float p2 = stof(point2) - 1;
      float p3 = stof(point3) - 1;
      triangles.push_back(ModelTriangle(readvertices[p1], readvertices[p2], readvertices[p3], colour));
    }
  }
  return triangles;
}

vec3 rayDir(int x, int y){
  vec3 dir = normalize(vec3(x-WIDTH/2, -(y-HEIGHT/2),-500)-camera)*camOrien;
  return dir;
}
RayTriangleIntersection getClosestIntersection(vec3 cameraPosition, vec3 rayDirection, vector<ModelTriangle> triangles){
  RayTriangleIntersection intersectionP;
  intersectionP.distanceFromCamera = INFINITY;
  for(u_int i=0;i<triangles.size();i++){
    vec3 e0 = triangles[i].vertices[1] - triangles[i].vertices[0];
    vec3 e1 = triangles[i].vertices[2] - triangles[i].vertices[0];
    vec3 SPVector = cameraPosition - (triangles[i].vertices[0]);
    mat3 DEMatrix(-rayDirection, e0, e1);
    vec3 possibleSolution = inverse(DEMatrix) * SPVector;
    float t = possibleSolution.x;
    float u = possibleSolution.y;
    float v = possibleSolution.z;
    if ((u>= 0) & (u<=1) & (v>= 0) & (v<=1) & ((u+v)<=1)){
      if (t < intersectionP.distanceFromCamera){
        intersectionP.distanceFromCamera = t;
        intersectionP.intersectedTriangle = triangles[i];
        intersectionP.intersectionPoint = triangles[i].vertices[0] + (u*e0) + (v*e1);
      }
    }
  }
  if(intersectionP.distanceFromCamera == INFINITY){
    intersectionP.distanceFromCamera = -INFINITY;
  }
  return intersectionP;
}

void computeRayT(){
  vector<ModelTriangle> triangles = readtriangles();
  for(int x=0;x<WIDTH;x++){
    for(int y=0;y<HEIGHT;y++){
      vec3 dir = rayDir(x,y);
      RayTriangleIntersection intersectP = getClosestIntersection(camera,dir, triangles);
      Colour c = intersectP.intersectedTriangle.colour;
      uint32_t colour = (255<<24) + (int(c.red)<<16) + (int(c.green)<<8) + int(c.blue);
      if(intersectP.distanceFromCamera != -INFINITY){
        window.setPixelColour(x, y, colour);
      }
    }
  }
  std::cout << "RAYTRACING FINISHED" << '\n';
}

void rotateX(vec3 camera, float angle){
  mat3 rotationMatrix(vec3(1.0,0.0,0.0),
  vec3(0,cos(angle),-sin(angle)),
  vec3(0,sin(angle),cos(angle)));
  camOrien = camOrien * rotationMatrix;
}
void rotateY(vec3 camera, float angle){
  mat3 rotationMatrix(vec3(cos(angle),0,sin(angle)),
  vec3(0,1,0),
  vec3(-sin(angle),0,cos(angle)));
  camOrien = camOrien * rotationMatrix;
}
void rotateZ(vec3 camera, float angle){
  mat3 rotationMatrix(vec3(cos(angle),-sin(angle),0),
  vec3(sin(angle),cos(angle),0),
  vec3(0,0,1));
  camOrien = camOrien * rotationMatrix;
}

int main(int argc, char* argv[])
{
  SDL_Event event;

  while(true)
  {
    if(window.pollForInputEvents(&event)){
      handleEvent(event);
      // bufferting(camera);
    }
    window.renderFrame();
  }
}

float angle = 0.1f;
void handleEvent(SDL_Event event)
{
  if(event.type == SDL_KEYDOWN) {
    window.clearPixels();
    if(event.key.keysym.sym == SDLK_LEFT){
      cout << "LEFT" << endl;
      camera.x += 1;
    }
    else if(event.key.keysym.sym == SDLK_RIGHT){
      cout << "RIGHT" << endl;
      camera.x -= 1;
    }
    else if(event.key.keysym.sym == SDLK_UP) {
      cout << "UP" << endl;
      camera.y -= 1;
    }
    else if(event.key.keysym.sym == SDLK_DOWN){
      cout << "DOWN" << endl;
      camera.y += 1;
    }
    else if(event.key.keysym.sym == SDLK_w){
      cout << "FORWARD" << endl;
      camera.z -= 1;
    }
    else if(event.key.keysym.sym == SDLK_s){
      cout << "BACKWARD" << endl;
      camera.z += 1;
    }
    else if(event.key.keysym.sym == SDLK_c) {
      // window.clearPixels();
      computeRayT();
    }
    else if(event.key.keysym.sym == SDLK_x) {
      rotateX(camera,angle);
    }
    else if(event.key.keysym.sym == SDLK_y) {
      rotateY(camera,angle);
    }
    else if(event.key.keysym.sym == SDLK_z) {
      rotateZ(camera,angle);
    }
    else if(event.key.keysym.sym == SDLK_r) {
      camera = vec3(0,0,6);
      camOrien = mat3();
    }
  }
  else if(event.type == SDL_MOUSEBUTTONDOWN) cout << "MOUSE CLICKED" << endl;
}
