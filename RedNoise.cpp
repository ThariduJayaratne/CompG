#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>

using namespace std;

using namespace glm;

#define WIDTH 480
#define HEIGHT 395

void draw();
void greyscale();
void update();
void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);

vector<float> interpolation(float from, float to, int n) {
  vector<float> list;
  float step = (to - from)/(n-1);
  for(int i = 0; i < n; i++) {
    list.push_back(from + i * step);
  }
  return list;
}

vector<vec3> interpolation3(vec3 from, vec3 to, int n) {
  vector<vec3> list;
  float stepx = (to.x - from.x)/(n-1);
  float stepy = (to.y - from.y)/(n-1);
  float stepz = (to.z - from.z)/(n-1);
  for(int i = 0; i < n; i++) {
    list.push_back(vec3(from.x + i * stepx,from.y + i * stepy,from.z + i * stepz));
  }
  return list;
}

vector<CanvasPoint> interpolation2(CanvasPoint from, CanvasPoint to, int n) {
  vector<CanvasPoint> list;
  float stepx = (to.x - from.x)/(n-1);
  float stepy = (to.y - from.y)/(n-1);
  for(int i = 0; i < n; i++) {
    CanvasPoint pt;
    pt.x = from.x + i * stepx;
    pt.y = from.y + i * stepy;
    list.push_back(pt);
  }
  return list;
}

void colorscale(){
  window.clearPixels();
  vec3 r = vec3(255,0,0);
  vec3 g = vec3(0,255,0);
  vec3 b = vec3(0,0,255);
  vec3 y = vec3(255,255,0);
  vector<vec3> redtoblue = interpolation3(r, b, window.width);
  vector<vec3> yellowtogreen = interpolation3(y, g, window.width);
  for(int i=0;i<window.width;i++){
    vector<vec3> step = interpolation3(redtoblue[i],yellowtogreen[i],window.height);
    for (int y = 0; y < window.height; y++) {
      uint32_t colour = (255<<24) + (int(step.at(y).x)<<16) + (int(step.at(y).y)<<8) + int(step.at(y).z);
      window.setPixelColour(i, y, colour);
    }
  }
}

void line(CanvasPoint from, CanvasPoint to, uint32_t colour) {
  float xDiff = to.x - from.x;
  float yDiff = to.y - from.y;
  float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
  float xStepSize = xDiff/numberOfSteps;
  float yStepSize = yDiff/numberOfSteps;
  for(int i = 0; i<numberOfSteps; i++) {
    float x = from.x + (xStepSize*i);
    float y = from.y + (yStepSize*i);
    window.setPixelColour(round(x), round(y), colour);
  }
}

void triangle( CanvasPoint point1, CanvasPoint point2, CanvasPoint point3, uint32_t colour) {
  line(point1, point2, colour);
  line(point2, point3, colour);
  line(point1, point3, colour);
}

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

vector<vec3> readVertices(){
  ifstream myfile;
  myfile.open("cornell-box.obj");
  std::string line;
  vector<vec3> vertices;
  while (getline(myfile, line)) {
    std::string* lineX = split(line,' ');
    if(line[0] == 'v'){
      float xc = stof(lineX[1]);
      float yc = stof(lineX[2]);
      float zc = stof(lineX[3]);
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
  vector<vec3> readvertices = readVertices();
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

void filltriangle(CanvasPoint point1, CanvasPoint point2, CanvasPoint point3, uint32_t colour) {
  for (int i = 0; i < 2; i++) {
    if (point3.y < point2.y) {
      CanvasPoint temp = point2;
      point2 = point3;
      point3 = temp;
    }
    if (point2.y < point1.y) {
      CanvasPoint temp = point1;
      point1 = point2;
      point2 = temp;
    }
    if (point3.y < point1.y) {
      CanvasPoint temp = point1;
      point1 = point3;
      point3 = temp;
    }
  }
  float x = ((point3.x - point1.x)*(point2.y - point1.y))/(point3.y - point1.y);
  CanvasPoint point4 = CanvasPoint(round(point1.x + x), point2.y);

  vector<CanvasPoint> points1to4 = interpolation2(point1,point4,(point4.y-point1.y));
  vector<CanvasPoint> points1to2 = interpolation2(point1,point2,(point2.y-point1.y));
  vector<CanvasPoint> points4to3 = interpolation2(point4,point3,(point3.y-point4.y));
  vector<CanvasPoint> points2to3 = interpolation2(point2,point3,(point3.y-point2.y));
  vector<CanvasPoint> points4to2 = interpolation2(point4,point2,abs(point2.x-point4.x));
  vector<CanvasPoint> pointstofill;

  for (u_int i = 0; i < points1to4.size(); i++) {
    for (u_int j = 0; j < points1to2.size(); j++) {
      line(points1to4[i],points1to2[j],colour);
    }
  }
  for (u_int i = 0; i < points4to3.size(); i++) {
    for (u_int j = 0; j < points2to3.size(); j++) {
      line(points4to3[i],points2to3[j],colour);
    }
 }
}

void drawWireframes(){
  vec3 camera = vec3(0,0,6);
  float f = 250;
  vector<ModelTriangle> triangles = readtriangles();
  vector<CanvasTriangle> canvastriangles;
  for(u_int i=0;i<triangles.size();i++){
    CanvasPoint test1,test2,test3;
    CanvasPoint projP1,projP2,projP3;
    float cameraP1x,cameraP1y,cameraP1z,cameraP2x,cameraP2y,cameraP2z,cameraP3x,cameraP3y,cameraP3z;
    test1.x = triangles[i].vertices[0].x;
    test2.x = triangles[i].vertices[1].x;
    test3.x = triangles[i].vertices[2].x;
    test1.y = triangles[i].vertices[0].y;
    test2.y = triangles[i].vertices[1].y;
    test3.y = triangles[i].vertices[2].y;
    test1.depth = triangles[i].vertices[0].z;
    test2.depth = triangles[i].vertices[1].z;
    test3.depth = triangles[i].vertices[2].z;

    cameraP1x = test1.x - camera.x;
    cameraP1y = test1.y - camera.y;
    cameraP1z = test1.depth - camera.z;
    float p1Screen = f/-cameraP1z;
    int P1XProj = (cameraP1x * p1Screen) + WIDTH/2;
    int P1YProj = ((1 - cameraP1y) * p1Screen) + HEIGHT/2;
    projP1.x = P1XProj;
    projP1.y = P1YProj;

    cameraP2x = test2.x - camera.x;
    cameraP2y = test2.y - camera.y;
    cameraP2z = test2.depth - camera.z;
    float p2Screen = f/-cameraP2z;
    int P2XProj = (cameraP2x * p2Screen) + WIDTH/2;
    int P2YProj = ((1 - cameraP2y) * p2Screen) + HEIGHT/2;
    projP2.x = P2XProj;
    projP2.y = P2YProj;

    cameraP3x = test3.x - camera.x;
    cameraP3y = test3.y - camera.y;
    cameraP3z = test3.depth - camera.z;
    float p3Screen = f/-cameraP3z;
    int P3XProj = (cameraP3x * p3Screen) + WIDTH/2;
    int P3YProj = ((1 - cameraP3y) * p3Screen) + HEIGHT/2;
    projP3.x = P3XProj;
    projP3.y = P3YProj;

    canvastriangles.push_back(CanvasTriangle(projP1,projP2,projP3));
  }
  for(u_int i =0;i<triangles.size();i++){
    Colour c = triangles[i].colour;
    float red = c.red;
    float green = c.green;
    float blue = c.blue;
    uint32_t colour = (255<<24) + (int(red)<<16) + (int(green)<<8) + int(blue);
    // triangle(canvastriangles[i].vertices[0],canvastriangles[i].vertices[1],canvastriangles[i].vertices[2],colour);
    filltriangle(canvastriangles[i].vertices[0],canvastriangles[i].vertices[1],canvastriangles[i].vertices[2],colour);
  }
}

vector<uint32_t> loadImg(){
  string line1;
  string line2;
  string line3;
  ifstream myfile;
  myfile.open("texture.ppm");
  getline(myfile, line1);
  getline(myfile, line2);
  getline(myfile, line3);
  string width = line3.substr(0,3);
  string height = line3.substr(4,3);
  int w = stoi(width);
  int h = stoi(height);
  vector<uint32_t> pixeldata;
  for(int i =0;i<h;i++){
    for(int j =0;j<w;j++){
      int r,g,b;
      b = myfile.get();
      g = myfile.get();
      r = myfile.get();
      uint32_t colour = (255<<24) + (r<<16) + (g<<8) + b;
      // window.setPixelColour(i, j, colour);
      pixeldata.push_back(colour);
    }
  }
  return pixeldata;
}

int main(int argc, char* argv[])
{
  SDL_Event event;
  drawWireframes();
  while(true)
  {
    if(window.pollForInputEvents(&event)) handleEvent(event);
    window.renderFrame();
  }
}

void texturetriangle(CanvasPoint point1, CanvasPoint point2, CanvasPoint point3){
  vector<uint32_t> imgdata = loadImg();
  for (int i = 0; i < 2; i++) {
    if (point3.y < point2.y) {
      CanvasPoint temp = point2;
      point2 = point3;
      point3 = temp;
    }
    if (point2.y < point1.y) {
      CanvasPoint temp = point1;
      point1 = point2;
      point2 = temp;
    }
    if (point3.y < point1.y) {
      CanvasPoint temp = point1;
      point1 = point3;
      point3 = temp;
    }
  }
  float x = ((point3.x - point1.x)*(point2.y - point1.y))/(point3.y - point1.y);
  CanvasPoint point4 = CanvasPoint(round(point1.x + x), point2.y);
  vector<CanvasPoint> points1to4 = interpolation2(point1,point4,(point4.y-point1.y));
  vector<CanvasPoint> points1to2 = interpolation2(point1,point2,(point2.y-point1.y));
  vector<CanvasPoint> points4to3 = interpolation2(point4,point3,(point3.y-point4.y));
  vector<CanvasPoint> points2to3 = interpolation2(point2,point3,(point3.y-point2.y));
  vector<CanvasPoint> points4to2 = interpolation2(point4,point2,abs(point2.x-point4.x));
  vector<CanvasPoint> pointstofill;
  for (unsigned int i = 0; i < points1to4.size(); i++) {
    for (unsigned int j = 0; j < points1to2.size(); j++) {
      pointstofill = interpolation2(points1to4[i],points1to2[j],abs(points1to4[i].x-points1to2[j].x));
      for(unsigned int k=0;k<pointstofill.size();k++){
        window.setPixelColour(round(pointstofill[k].x), round(pointstofill[k].y), imgdata[round(pointstofill[k].x) + round(pointstofill[k].y)* WIDTH]);
      }
    }
  }
  for (unsigned int i = 0; i < points4to3.size(); i++) {
    for (unsigned int j = 0; j < points2to3.size(); j++) {
      pointstofill = interpolation2(points4to3[i],points2to3[j],abs(points4to3[i].x-points2to3[j].x));
      for(unsigned int k=0;k<pointstofill.size();k++){
        window.setPixelColour(round(pointstofill[k].x), round(pointstofill[k].y), imgdata[round(pointstofill[k].x) + round(pointstofill[k].y)* WIDTH]);
      }
    }
  }
}

void draw()
{
  window.clearPixels();
  for(int y=0; y<window.height ;y++) {
    for(int x=0; x<window.width ;x++) {
      float red = rand() % 255;
      float green = 0.0;
      float blue = 0.0;
      uint32_t colour = (255<<24) + (int(red)<<16) + (int(green)<<8) + int(blue);
      window.setPixelColour(x, y, colour);
    }
  }
}

void greyscale() {
    window.clearPixels();
    vector<float> values = interpolation(0, 255, window.width);
    for (int x = 0; x < window.width; x++) {
      uint32_t colour = (255<<24) + (int(255 - values[x])<<16) + (int(255 - values[x])<<8) + int(255 - values[x]);
      for (int y = 0; y < window.height; y++) {
        window.setPixelColour(x, y, colour);
      }
    }
}

void update()
{
  // Function for performing animation (shifting artifacts or moving the camera)
}

void handleEvent(SDL_Event event)
{
  if(event.type == SDL_KEYDOWN) {
    if(event.key.keysym.sym == SDLK_LEFT) cout << "LEFT" << endl;
    else if(event.key.keysym.sym == SDLK_RIGHT) cout << "RIGHT" << endl;
    else if(event.key.keysym.sym == SDLK_UP) cout << "UP" << endl;
    else if(event.key.keysym.sym == SDLK_DOWN) cout << "DOWN" << endl;
    else if(event.key.keysym.sym == SDLK_u) {
      CanvasPoint p1 = CanvasPoint(rand() % 480, rand() % 395);
      CanvasPoint p2 = CanvasPoint(rand() % 480, rand() % 395);
      CanvasPoint p3 = CanvasPoint(rand() % 480, rand() % 395);

      uint32_t colour = (255<<24) + ((rand() % 255)<<16) + ((rand() % 255)<<8) + (rand() % 255);
      triangle(p1,p2,p3,colour);
    }
    else if(event.key.keysym.sym == SDLK_f) {
      CanvasPoint p1 = CanvasPoint(rand() % 480, rand() % 395);
      CanvasPoint p2 = CanvasPoint(rand() % 480, rand() % 395);
      CanvasPoint p3 = CanvasPoint(rand() % 480, rand() % 395);
      uint32_t colour = (255<<24) + ((rand() % 256)<<16) + ((rand() % 256)<<8) + (rand() % 256);
      filltriangle(p1,p2,p3,colour);
    }
    else if(event.key.keysym.sym == SDLK_c) {
      window.clearPixels();
    }
    else if(event.key.keysym.sym == SDLK_t) {
      CanvasPoint p1 = CanvasPoint(rand() % 480, rand() % 395);
      CanvasPoint p2 = CanvasPoint(rand() % 480, rand() % 395);
      CanvasPoint p3 = CanvasPoint(rand() % 480, rand() % 395);
      texturetriangle(p1,p2,p3);
    }

  }
  else if(event.type == SDL_MOUSEBUTTONDOWN) cout << "MOUSE CLICKED" << endl;
}
