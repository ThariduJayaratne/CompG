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

#define WIDTH 640
#define HEIGHT 480
#define PI 3.14159265
#define SCALETING 0.5

void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
int mode = 1; //1 -> Wireframe    2 -> Rasteriser    3 -> Raytracer
mat3 camOrien = mat3();
vec3 camera = vec3(0,0,6);
float angle = 0.1f;

vec3 whitelight = vec3(-0.242005 , 2.00925, -0.683984) * (float)SCALETING;

void savePPM(){
  std::ofstream file("frame.ppm", std::ofstream::out | std::ofstream::trunc);
  file << "P6\n";
  file << "640 480\n";
  file << "255\n";

  for(int i = 0; i<HEIGHT;i++){
    for(int j = 0; j<WIDTH;j++){
      uint32_t colour = window.getPixelColour(j,i);
      int r = (colour >> 16) & 255;
      int g = (colour >> 8) & 255;
      int b = (colour) & 255;
      file << (u8) r << (u8) g << (u8) b;
    }
  }
  file.close();
}
double **malloc2dArray(int dim1, int dim2) {
    double **array = (double **) malloc(dim1 * sizeof(double *));
    for (int i = 0; i < dim1; i++) {
        array[i] = (double *) malloc(dim2 * sizeof(double));
    }
    return array;
}

vector<float> interpolation(float from, float to, int n) {
  vector<float> list;
  float step = (to - from)/(n-1);
  for(int i = 0; i < n; i++) {
    list.push_back(from + i * step);
  }
  return list;
}

vector<CanvasPoint> interpolation2(CanvasPoint from, CanvasPoint to, int n) {
  vector<CanvasPoint> list;
  float stepx = (to.x - from.x)/n;
  float stepy = (to.y - from.y)/n;
  double stepDepth = (to.depth - from.depth)/n;
  for(int i = 0; i <= n; i++) {
    CanvasPoint pt;
    pt.x = from.x + i * stepx;
    pt.y = from.y + i * stepy;
    pt.depth = from.depth + i * stepDepth;
    list.push_back(pt);
  }
  return list;
}

vector<Colour> readmat(string filename){
  ifstream myfile;
  myfile.open(filename);
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
    Colour thisColour;
    thisColour.name = colors[i];
    thisColour.red = rgb[i].x;
    thisColour.green = rgb[i].y;
    thisColour.blue = rgb[i].z;
    result.push_back(thisColour);
  }
  return result;
}

vector<vec3> readVertices(float scale, string filename){
  ifstream myfile;
  myfile.open(filename);
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

vector<ModelTriangle> readtriangles(string filename, vector<Colour> colours){
  fstream myfile;
  myfile.open(filename);
  std::string line;
  vector<ModelTriangle> triangles;
  vector<vec3> readvertices = readVertices(SCALETING, filename);
  Colour colour;
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

vector<uint32_t> loadImg(){
  string line1;  // double **buffer = malloc2dArray(WIDTH,HEIGHT);
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

void drawLine(CanvasPoint from, CanvasPoint to, uint32_t colour) {

  float xDiff = to.x - from.x;
  float yDiff = to.y - from.y;
  float numberOfSteps = std::max(abs(xDiff), abs(yDiff));
  float xStepSize = xDiff/numberOfSteps;
  float yStepSize = yDiff/numberOfSteps;

  vector<float> depths = interpolation(from.depth, to.depth, numberOfSteps);

  for(int i = 0; i<numberOfSteps; i++) {
    float x = from.x + (xStepSize*i);
    float y = from.y + (yStepSize*i);
    window.setPixelColour(round(x), round(y), colour);
  }
}

void drawTriangle( CanvasPoint point1, CanvasPoint point2, CanvasPoint point3, uint32_t colour) {
  drawLine(point1, point2, colour);
  drawLine(point2, point3, colour);
  drawLine(point1, point3, colour);
}

void drawFillTriangle(CanvasPoint point1, CanvasPoint point2, CanvasPoint point3, uint32_t colour) {

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

  vector<CanvasPoint> points1to4 = interpolation2(point1,point4,(point4.y-point1.y)+1);
  vector<CanvasPoint> points1to2 = interpolation2(point1,point2,(point2.y-point1.y)+1);
  vector<CanvasPoint> points4to3 = interpolation2(point4,point3,(point3.y-point4.y)+1);
  vector<CanvasPoint> points2to3 = interpolation2(point2,point3,(point3.y-point2.y)+1);

  for (u_int i = 0; i < points1to4.size(); i++) {
    drawLine(points1to4[i],points1to2[i],colour);
    }

  for (u_int i = 0; i < points4to3.size(); i++) {
    drawLine(points4to3[i],points2to3[i],colour);
  }
}

void draw3DWireframes(vector<CanvasTriangle> triangles) {

  double **buffer = malloc2dArray(WIDTH, HEIGHT);
  for(u_int y = 0; y < HEIGHT; y++) {
    for(u_int x = 0; x < WIDTH; x++) {
      buffer[x][y] = -INFINITY;
    }
  }

  for (u_int i = 0; i < triangles.size(); i++) {
    Colour c = triangles[i].colour;
    uint32_t colour = (255<<24) + (int(c.red)<<16) + (int(c.green)<<8) + int(c.blue);

    float numberOfSteps = std::max(abs(triangles[i].vertices[1].x - triangles[i].vertices[0].x), abs(triangles[i].vertices[1].y - triangles[i].vertices[0].y));
    vector<CanvasPoint> points1to2 = interpolation2(triangles[i].vertices[0], triangles[i].vertices[1], (int)numberOfSteps);
    numberOfSteps = std::max(abs(triangles[i].vertices[2].x - triangles[i].vertices[0].x), abs(triangles[i].vertices[2].y - triangles[i].vertices[0].y));
    vector<CanvasPoint> points1to3 = interpolation2(triangles[i].vertices[0], triangles[i].vertices[2], (int)numberOfSteps);
    numberOfSteps = std::max(abs(triangles[i].vertices[2].x - triangles[i].vertices[1].x), abs(triangles[i].vertices[2].y - triangles[i].vertices[1].y));
    vector<CanvasPoint> points2to3 = interpolation2(triangles[i].vertices[1], triangles[i].vertices[2], (int)numberOfSteps);

    for (u_int i = 0; i < points1to2.size(); i++) {
      if ((int)points1to2[i].x >= 0 && (int)points1to2[i].x < WIDTH && (int)points1to2[i].y >= 0 && (int)points1to2[i].y < HEIGHT) {
        if (points1to2[i].depth > buffer[(int)points1to2[i].x][(int)points1to2[i].y]) {
          buffer[(int)points1to2[i].x][(int)points1to2[i].y] = points1to2[i].depth;
          window.setPixelColour((int)points1to2[i].x, (int)points1to2[i].y, colour);
        }
      }
    }

    for (u_int i = 0; i < points1to3.size(); i++) {
      if ((int)points1to3[i].x >= 0 && (int)points1to3[i].x < WIDTH && (int)points1to3[i].y >= 0 && (int)points1to3[i].y < HEIGHT) {
        if (points1to3[i].depth > buffer[(int)points1to3[i].x][(int)points1to3[i].y]) {
          buffer[(int)points1to3[i].x][(int)points1to3[i].y] = points1to3[i].depth;
          window.setPixelColour((int)points1to3[i].x, (int)points1to3[i].y, colour);
        }
      }
    }

    for (u_int i = 0; i < points2to3.size(); i++) {
      if ((int)points2to3[i].x >= 0 && (int)points2to3[i].x < WIDTH && (int)points2to3[i].y >= 0 && (int)points2to3[i].y < HEIGHT) {
        if (points2to3[i].depth > buffer[(int)points2to3[i].x][(int)points2to3[i].y]) {
          buffer[(int)points2to3[i].x][(int)points2to3[i].y] = points2to3[i].depth;
          window.setPixelColour((int)points2to3[i].x, (int)points2to3[i].y, colour);
        }
      }
    }
  }
}

void draw3DRasterisedTriangles(vector<CanvasTriangle> canvastriangles) {

  double **buffer = malloc2dArray(WIDTH, HEIGHT);
  for(u_int y = 0; y < HEIGHT; y++) {
    for(u_int x = 0; x < WIDTH; x++) {
      buffer[x][y] = -INFINITY;
    }
  }

  for(u_int i = 0; i < canvastriangles.size(); i++){

    CanvasPoint point1 = canvastriangles[i].vertices[0];
    CanvasPoint point2 = canvastriangles[i].vertices[1];
    CanvasPoint point3 = canvastriangles[i].vertices[2];
    Colour c = canvastriangles[i].colour;
    float red = c.red;
    float green = c.green;
    float blue = c.blue;
    uint32_t colour = (255<<24) + (int(red)<<16) + (int(green)<<8) + int(blue);

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

    double ratio = (point3.y - point2.y)/(point3.y - point1.y);
    double point4Depth = point3.depth - ratio * (point3.depth - point1.depth);
    CanvasPoint point4 = CanvasPoint(point3.x - ratio * (point3.x - point1.x), point3.y - ratio * (point3.y - point1.y), point4Depth);
    vector<CanvasPoint> points1to4coord = interpolation2(point1,point4,(point2.y-point1.y)+1);
    vector<CanvasPoint> points1to2coord = interpolation2(point1,point2,(point2.y-point1.y)+1);
    vector<CanvasPoint> points4to3coord = interpolation2(point4,point3,(point3.y-point2.y)+1);
    vector<CanvasPoint> points2to3coord = interpolation2(point2,point3,(point3.y-point2.y)+1);

    for (u_int i = 0; i < points1to4coord.size(); i++) {
      vector<CanvasPoint> pointstofillcoord = interpolation2(points1to4coord[i],points1to2coord[i],abs(points1to4coord[i].x-points1to2coord[i].x)+1);
      for(u_int k = 0; k < pointstofillcoord.size(); k++) {
        CanvasPoint current = pointstofillcoord[k];
        if(((int)current.x >= 0) && ((int)current.x < WIDTH) && ((int)current.y >= 0) && ((int)current.y < HEIGHT)) {
          if(current.depth > buffer[(int)current.x][(int)current.y]) {
            buffer[(int)current.x][(int)current.y] = current.depth;
            window.setPixelColour((int)current.x, (int)current.y, colour);
          }
        }
      }
    }

    for (u_int i = 0; i < points4to3coord.size(); i++) {
      vector<CanvasPoint> pointstofillcoord = interpolation2(points4to3coord[i],points2to3coord[i],abs(points4to3coord[i].x-points2to3coord[i].x)+1);
      for(u_int k = 0; k < pointstofillcoord.size(); k++) {
        CanvasPoint current = pointstofillcoord[k];
        if((current.x >= 0) && ((int)current.x < WIDTH) && ((int)current.y >= 0) && ((int)current.y < HEIGHT)) {
          if(current.depth > buffer[(int)current.x][(int)current.y]) {
            buffer[(int)current.x][(int)current.y] = current.depth;
            window.setPixelColour((int)current.x, (int)current.y, colour);
          }
        }
      }
    }

  }
}

vector<CanvasTriangle> modelToCanvas(vec3 camera, vector<ModelTriangle> triangles, float focalDistance, float canvasScale){

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

    //PERSPECTIVE
    cameraP1x = test1.x - camera.x;
    cameraP1y = test1.y - camera.y;
    cameraP1z = test1.depth - camera.z;

    vec3 camPer1 = vec3(cameraP1x,cameraP1y,cameraP1z);
    vec3 adjustedPos1 = camPer1 * camOrien;

    //PROJECTION
    float p1Screen = focalDistance/-adjustedPos1.z;
    int P1XProj = (adjustedPos1.x * p1Screen * canvasScale) + WIDTH/2;
    int P1YProj = ((1 - adjustedPos1.y) * p1Screen * canvasScale) + HEIGHT/2;
    projP1.x = P1XProj;
    projP1.y = P1YProj;
    projP1.depth = 1/-(adjustedPos1.z);


    cameraP2x = test2.x - camera.x;
    cameraP2y = test2.y - camera.y;
    cameraP2z = test2.depth - camera.z;

    vec3 camPer2 = vec3(cameraP2x,cameraP2y,cameraP2z);
    vec3 adjustedPos2 = camPer2 * camOrien;

    float p2Screen = focalDistance/-adjustedPos2.z;
    int P2XProj = (adjustedPos2.x * p2Screen * canvasScale) + WIDTH/2;
    int P2YProj = ((1 - adjustedPos2.y) * p2Screen  * canvasScale) + HEIGHT/2;
    projP2.x = P2XProj;
    projP2.y = P2YProj;
    projP2.depth = 1/-(adjustedPos2.z);


    cameraP3x = test3.x - camera.x;
    cameraP3y = test3.y - camera.y;
    cameraP3z = test3.depth - camera.z;

    vec3 camPer3 = vec3(cameraP3x,cameraP3y,cameraP3z);
    vec3 adjustedPos3 = camPer3 * camOrien;

    float p3Screen = focalDistance/-adjustedPos3.z;
    int P3XProj = (adjustedPos3.x * p3Screen  * canvasScale) + WIDTH/2;
    int P3YProj = ((1 - adjustedPos3.y) * p3Screen  * canvasScale) + HEIGHT/2;
    projP3.x = P3XProj;
    projP3.y = P3YProj;
    projP3.depth = 1/-(adjustedPos3.z);

    // Colour c = triangles[i].colour;
    // uint32_t colour = (255<<24) + (int(c.red)<<16) + (int(c.green)<<8) + int(c.blue);

    canvastriangles.push_back(CanvasTriangle(projP1,projP2,projP3,triangles[i].colour));
  }

  return canvastriangles;
}

vec3 rayDir(int x, int y) {
  vec3 dir = normalize(vec3(x-WIDTH/2, -(y-HEIGHT/2),-500)-camera)*camOrien;
  return dir;
}

bool inShadow(vec3 surfacePoint, vec3 lightSource, vector<ModelTriangle> triangles, int index){
  bool shadow = false;
  vec3 dir = normalize(lightSource - surfacePoint);
  float distance = length(dir);
  for(u_int i=0;i<triangles.size();i++){
    vec3 e0 = triangles[i].vertices[1] - triangles[i].vertices[0];
    vec3 e1 = triangles[i].vertices[2] - triangles[i].vertices[0];
    vec3 SPVector = surfacePoint - (triangles[i].vertices[0]);
    mat3 DEMatrix(-dir, e0, e1);
    vec3 possibleSolution = inverse(DEMatrix) * SPVector;
    float t = possibleSolution.x;
    float u = possibleSolution.y;
    float v = possibleSolution.z;
    if ((u>= 0) & (u<=1) & (v>= 0) & (v<=1) & ((u+v)<=1) & i != index){
      if (t < distance){
        shadow = true;
        break;
      }
    }
  }
  return shadow;
}

RayTriangleIntersection getClosestIntersection(vec3 cameraPosition, vec3 rayDirection, vector<ModelTriangle> triangles) {
  RayTriangleIntersection intersectionP;
  intersectionP.distanceFromCamera = INFINITY;
  for(u_int i=0;i<triangles.size();i++){
    ModelTriangle triX = triangles[i];
    vec3 e0 = triX.vertices[1] - triX.vertices[0];
    vec3 e1 = triX.vertices[2] - triX.vertices[0];
    vec3 SPVector = cameraPosition - (triX.vertices[0]);
    mat3 DEMatrix(-rayDirection, e0, e1);
    vec3 possibleSolution = inverse(DEMatrix) * SPVector;
    float t = possibleSolution.x;
    float u = possibleSolution.y;
    float v = possibleSolution.z;
    if ((u>= 0) & (u<=1) & (v>= 0) & (v<=1) & ((u+v)<=1)){
      if (t < intersectionP.distanceFromCamera){
        if((intersectionP.distanceFromCamera - t) > 0.05){
        intersectionP.distanceFromCamera = t;
        intersectionP.intersectedTriangle = triangles[i];
        intersectionP.intersectionPoint = triangles[i].vertices[0] + (u*e0) + (v*e1);
        vec3 normaltovertices = normalize(cross(triX.vertices[1]-triX.vertices[0],triX.vertices[2]-triX.vertices[0]));
        vec3 pToL = whitelight - intersectionP.intersectionPoint;
        float dotProd = normalize(dot(normaltovertices,pToL));
        if(dotProd < 0.0f) dotProd = 0.0f;
        float myDistance = length(pToL);
        float brightness = (dotProd)/(0.5*M_PI* myDistance * myDistance);
        Colour c = intersectionP.intersectedTriangle.colour;
        if(brightness > 1.0f) brightness = 1.0f;
        if(brightness < 0.2f) brightness = 0.2f;
        if(inShadow(intersectionP.intersectionPoint, whitelight,triangles,i)){
          brightness = 0.15f;
        }
        intersectionP.intersectedTriangle.colour.red = c.red * brightness;
        intersectionP.intersectedTriangle.colour.green = c.green * brightness;
        intersectionP.intersectedTriangle.colour.blue = c.blue * brightness;
        }
      }
    }
  }
  return intersectionP;
}


void computeRayT(vector<ModelTriangle> triangles,vec3 whitelight){
  for(int x=0;x<WIDTH;x++){
    for(int y=0;y<HEIGHT;y++){
      vec3 dir = rayDir(x,y);
      RayTriangleIntersection intersectP = getClosestIntersection(camera, dir, triangles);
      ModelTriangle trianglex = intersectP.intersectedTriangle;
      vec3 normaltovertices = normalize(cross(trianglex.vertices[1]-trianglex.vertices[0],trianglex.vertices[2]-trianglex.vertices[0]));
      Colour c = intersectP.intersectedTriangle.colour;
      vec3 pToL = whitelight - intersectP.intersectionPoint;
      float dotProd = normalize(dot(normaltovertices,pToL));
      if(dotProd < 0.0f) dotProd = 0.0f;
      float myDistance = length(pToL);
      float brightness = 2 * (dotProd)/(0.5*M_PI* myDistance * myDistance);
      if(brightness > 1.0f) brightness = 1.0f;
      if(brightness < 0.2f) brightness = 0.2f;
      uint32_t colour = (255<<24) + (int(c.red * brightness)<<16) + (int(c.green * brightness)<<8) + int(c.blue * brightness);
      if(intersectP.distanceFromCamera != INFINITY){
        window.setPixelColour(x, y, colour);
      }
    }
  }
}

void lookAt (vec3 camera, vec3 point) {
  vec3 vertical = vec3(0, 1, 0);
  vec3 forward = normalize(camera - point);
  vec3 right = normalize(cross(vertical, forward));
  vec3 up = normalize(cross(forward, right));
  mat3 newCamOrien = mat3(right, up,forward);
  camOrien = inverse(transpose(newCamOrien));
}

int main(int argc, char* argv[])
{

  vector<Colour> colours = readmat("cornell-box.mtl");
  vector<ModelTriangle> triangles = readtriangles("cornell-box.obj", colours);
  SDL_Event event;
  while(true)
  {
    if(window.pollForInputEvents(&event)){
      handleEvent(event);
      window.clearPixels();
      if (mode == 1) {
        vector<CanvasTriangle> canvastriangles = modelToCanvas(camera, triangles, 5, 100);
        draw3DWireframes(canvastriangles);
        // lookAt(camera,vec3(0,0,0));
        // vector<CanvasTriangle> canvastriangles = modelToCanvas(camera, triangles, 5, 100);
        // draw3DRasterisedTriangles(canvastriangles);
      }

      else if (mode == 2) {
        vector<CanvasTriangle> canvastriangles = modelToCanvas(camera, triangles, 5, 100);
        draw3DRasterisedTriangles(canvastriangles);
      }

      else if (mode == 3) {
        computeRayT(triangles, whitelight);
      }
    }
    window.renderFrame();
  }
}

void rotateX(float angle){
  mat3 rotationMatrix(vec3(1.0,0.0,0.0),
  vec3(0,cos(angle),-sin(angle)),
  vec3(0,sin(angle),cos(angle)));
  camOrien = camOrien * rotationMatrix;
}

void rotateY(float angle){
  mat3 rotationMatrix(vec3(cos(angle),0,sin(angle)),
  vec3(0,1,0),
  vec3(-sin(angle),0,cos(angle)));
  camOrien = camOrien * rotationMatrix;
}

void rotateZ(float angle){
  mat3 rotationMatrix(vec3(cos(angle),-sin(angle),0),
  vec3(sin(angle),cos(angle),0),
  vec3(0,0,1));
  camOrien = camOrien * rotationMatrix;
}


void handleEvent(SDL_Event event)
{
  if(event.type == SDL_KEYDOWN) {
    if(event.key.keysym.sym == SDLK_LEFT){
      cout << "LEFT" << endl;
      camera.x -= 0.1;
    }
    else if(event.key.keysym.sym == SDLK_RIGHT){
      cout << "RIGHT" << endl;
      camera.x += 0.1;
    }
    else if(event.key.keysym.sym == SDLK_UP) {
      cout << "UP" << endl;
      camera.y += 0.1;
    }
    else if(event.key.keysym.sym == SDLK_DOWN){
      cout << "DOWN" << endl;
      camera.y -= 0.1;
    }
    else if(event.key.keysym.sym == SDLK_w){
      cout << "FORWARD" << endl;
      camera.z -= 0.1;
    }
    else if(event.key.keysym.sym == SDLK_s){
      cout << "BACKWARD" << endl;
      camera.z += 0.1;
    }


    if(event.key.keysym.sym == SDLK_g){
      whitelight.x -= 0.1;
    }
    else if(event.key.keysym.sym == SDLK_j){
      whitelight.x += 0.1;
    }
    else if(event.key.keysym.sym == SDLK_y) {
      whitelight.y += 0.1;
    }
    else if(event.key.keysym.sym == SDLK_h){
      whitelight.y -= 0.1;
    }
    else if(event.key.keysym.sym == SDLK_n){
      whitelight.z -= 0.1;
    }
    else if(event.key.keysym.sym == SDLK_b){
      whitelight.z += 0.1;
    }
    else if(event.key.keysym.sym == SDLK_p){
      cout << whitelight.x << ' ';
      cout << whitelight.y << ' ';
      cout << whitelight.z << ' ';
    }
    else if(event.key.keysym.sym == SDLK_v){
      savePPM();
    }

    else if(event.key.keysym.sym == SDLK_1) {
      mode = 1;
    }
    else if(event.key.keysym.sym == SDLK_2) {
      mode = 2;
    }
    else if(event.key.keysym.sym == SDLK_3) {
      mode = 3;
    }
    else if(event.key.keysym.sym == SDLK_c) {
      window.clearPixels();
    }
    else if(event.key.keysym.sym == SDLK_x) {
      rotateX(angle);
    }
    else if(event.key.keysym.sym == SDLK_y) {
      rotateY(angle);
    }
    else if(event.key.keysym.sym == SDLK_z) {
      rotateZ(angle);
    }
    else if(event.key.keysym.sym == SDLK_r) {
      camera = vec3(0,0,6);
      camOrien = mat3();
    }
  }
  else if(event.type == SDL_MOUSEBUTTONDOWN) cout << "MOUSE CLICKED" << endl;
}
