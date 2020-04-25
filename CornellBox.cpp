#include <ModelTriangle.h>
#include <CanvasTriangle.h>
#include <DrawingWindow.h>
#include <RayTriangleIntersection.h>
#include <Utils.h>
#include <glm/glm.hpp>
#include <fstream>
#include <vector>
#include <map>
#include <ModelTexture.h>

using namespace std;

using namespace glm;

#define WIDTH 640
#define HEIGHT 480
#define PI 3.14159265
#define SCALETING 0.5
#define SCALETING2 0.1
#define SCALETING3 0.002


void handleEvent(SDL_Event event);

DrawingWindow window = DrawingWindow(WIDTH, HEIGHT, false);
int mode = 1; //1 -> Wireframe    2 -> Rasteriser    3 -> Raytracer   4-> Anitaliasing   5->Culling
mat3 camOrien = mat3();
vec3 camera = vec3(0,0,6);
float angle = 0.1f;


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

vector<vec3> setwhitelights(vec3 from){
  vector<vec3> list;
  list.push_back(from);
  for(u_int i=1;i<7;i++){
    vec3 temp = from;
    float count = 0.02 * i;
    temp.x += count;
    list.push_back(temp);
  }
  for(u_int i=1;i<7;i++){
    vec3 temp = from;
    float count = 0.02 * i;
    temp.x -= count;
    list.push_back(temp);
  }
  return list;
}

// vec3 corner1 = vec3(-0.884011,5.018497,-3.567968)* (float)SCALETING;
// vec3 corner2 = vec3(0.415989,5.019334,-2.517968)* (float)SCALETING;
// vector<vec3> whitelights = interpolation3(corner1,corner2,13);

vec3 whitelight = vec3(-0.15 , 2.00463, -1.24199);

vector<vec3> whitelights = setwhitelights(whitelight);

// vec3 whitelight = vec3(-0.242005 , 2.00925, -0.683984) * (float)SCALETING;

vector<pair<ModelTriangle,vector<vec3>>> triangleNormals;
vector<pair<ModelTriangle,ModelTexture>> texturePoints;



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
  float stepxt = (to.texturePoint.x - from.texturePoint.x)/n;
  float stepyt = (to.texturePoint.y - from.texturePoint.y)/n;
  for(int i = 0; i <= n; i++) {
    CanvasPoint pt;
    pt.x = from.x + i * stepx;
    pt.y = from.y + i * stepy;
    pt.depth = from.depth + i * stepDepth;
    pt.texturePoint.x =  from.texturePoint.x + i * stepxt;
    pt.texturePoint.y =  from.texturePoint.y + i * stepyt;
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

vector<vec3> readVertices2(float scale, string filename){
  ifstream myfile;
  myfile.open(filename);
  std::string line;
  vector<vec3> vertices;
  while (getline(myfile, line)) {
    std::string* lineX = split(line,' ');
    if(line[0] == 'v'){
      float xc = stof(lineX[1])*scale - 0.5;
      float yc = stof(lineX[2])*scale + 0.5;
      float zc = stof(lineX[3])*scale - 1;
      vertices.push_back(vec3(xc,yc,zc));
    }
  }
  return vertices;
}
vector<vec3> readVertices3(float scale, string filename){
  ifstream myfile;
  myfile.open(filename);
  std::string line;
  vector<vec3> vertices;
  while (getline(myfile, line)) {
    std::string* lineX = split(line,' ');
    if(lineX[0] == "v"){
      float xc = stof(lineX[1])*scale - 0.2;
      float yc = stof(lineX[2])*scale + 0.7;
      float zc = stof(lineX[3])*scale - 0.7;
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
      ModelTriangle toPush = ModelTriangle(readvertices[p1], readvertices[p2], readvertices[p3], colour);
      if(toPush.colour.name == "Mirror"){
        toPush.name = "mirror";
      }
      else{
        toPush.name = "cornell";
      }
      triangles.push_back(toPush);
    }
  }
  return triangles;
}

vector<ModelTriangle> readtriangles2(string filename){
  Colour c;
  c.red = 0;
  c.green = 255;
  c.blue = 255;
  ifstream myfile;
  myfile.open(filename);
  string line;
  vector<ModelTriangle> triangles;
  vector<vec3> readvertices = readVertices2(SCALETING2, filename);
  while(getline(myfile, line)) {
    string *splitLine = split(line, ' ');
    if(splitLine[0] == "f"){
      int slashpos1 =  splitLine[1].find('/');
      int slashpos2 =  splitLine[2].find('/');
      int slashpos3 =  splitLine[3].find('/');
      string point1 = splitLine[1].substr(0,slashpos1);
      string point2 = splitLine[2].substr(0,slashpos2);
      string point3 = splitLine[3].substr(0,slashpos3);
      int p1 = stoi(point1) - 1;
      int p2 = stoi(point2) - 1;
      int p3 = stoi(point3) - 1;
      ModelTriangle toPush = ModelTriangle(readvertices[p1], readvertices[p2], readvertices[p3], c);
      toPush.name = "sphere";
      triangles.push_back(toPush);
    }
  }
  myfile.close();
  return triangles;
}

vector<TexturePoint> readTexturePoints(float scale,string filename){
  fstream myfile;
  myfile.open(filename);
  std::string line;
  vector<TexturePoint> points;
  TexturePoint p;
  while (getline(myfile, line)) {
    string *splitLine = split(line, ' ');
    if(splitLine[0] == "vt"){
      p.x =  stof(splitLine[1]) * scale;
      p.y =  stof(splitLine[2]) * scale;
      points.push_back(p);
    }
  }
  return points;
}

vector<uint32_t> loadImg(string filename){
  string line1;
  string line2;
  string line3;
  ifstream myfile;
  myfile.open(filename);
  getline(myfile, line1);
  getline(myfile, line2);
  getline(myfile, line3);
  getline(myfile, line2);
  string width = line3.substr(0,3);
  string height = line3.substr(4,3);
  int w = stoi(width);
  int h = stoi(height);
  vector<uint32_t> pixeldata;
    for(int j =0;j<w*h;j++){
      int r,g,b;
      r = (myfile.get());
      g = (myfile.get());
      b = (myfile.get());
      uint32_t colour = (255<<24) + (r<<16) + (g<<8) + b;
      pixeldata.push_back(colour);
    }
  return pixeldata;
}

vector<ModelTexture> readTextureIndex(string filename){
  vector<ModelTexture> textureTriangles;
  Colour c;
  c.red = 255;
  c.green = 0;
  c.blue = 0;
  TexturePoint p1;
  p1.x =  -1;
  p1.y =  -1;
  if(filename == "sphere.obj" || filename == "cornell-box.obj"){
    ifstream myfile;
    myfile.open(filename);
    std::string line;
    vector<TexturePoint> textPoints = readTexturePoints(300.f,filename);
    while (getline(myfile, line)) {
      string *splitLine = split(line, ' ');
      if(splitLine[0] == "f"){
        ModelTexture toPush = ModelTexture(p1, p1, p1,c);
        toPush.name = "noTexture";
        textureTriangles.push_back(toPush);
      }
    }
  }
  else{
    ifstream myfile;
    myfile.open(filename);
    std::string line;
    vector<TexturePoint> textPoints = readTexturePoints(300.f,filename);
    while (getline(myfile, line)) {
      string *splitLine = split(line, ' ');
      if(splitLine[0] == "f"){
        int slashpos1 =  splitLine[1].find('/');
        int slashpos2 =  splitLine[2].find('/');
        int slashpos3 =  splitLine[3].find('/');
        string Tpoint1 = splitLine[1].substr(slashpos1 + 1,splitLine[1].length());
        string Tpoint2 = splitLine[2].substr(slashpos2 + 1,splitLine[2].length());
        string Tpoint3 = splitLine[3].substr(slashpos3 + 1,splitLine[3].length());
        int Tp1 = stoi(Tpoint1) - 1;
        int Tp2 = stoi(Tpoint2) - 1;
        int Tp3 = stoi(Tpoint3) - 1;
        ModelTexture toPush = ModelTexture(textPoints[Tp1], textPoints[Tp2], textPoints[Tp3],c);
        toPush.name = "logo";
        textureTriangles.push_back(toPush);
      }
    }
  }
  return textureTriangles;
}
vector<ModelTexture> readTextureIndexForRayTracer(string filename){
  Colour c;
  c.red = 255;
  c.green = 0;
  c.blue = 0;
  vector<ModelTexture> textureTriangles;
    ifstream myfile;
    myfile.open(filename);
    std::string line;
    vector<TexturePoint> textPoints = readTexturePoints(1.0f,filename);
    while (getline(myfile, line)) {
      string *splitLine = split(line, ' ');
      if(splitLine[0] == "f"){
        int slashpos1 =  splitLine[1].find('/');
        int slashpos2 =  splitLine[2].find('/');
        int slashpos3 =  splitLine[3].find('/');
        string Tpoint1 = splitLine[1].substr(slashpos1 + 1,splitLine[1].length());
        string Tpoint2 = splitLine[2].substr(slashpos2 + 1,splitLine[2].length());
        string Tpoint3 = splitLine[3].substr(slashpos3 + 1,splitLine[3].length());
        int Tp1 = stoi(Tpoint1) - 1;
        int Tp2 = stoi(Tpoint2) - 1;
        int Tp3 = stoi(Tpoint3) - 1;
        textureTriangles.push_back(ModelTexture(textPoints[Tp1], textPoints[Tp2], textPoints[Tp3],c));
    }
  }
  return textureTriangles;
}

vector<ModelTriangle> readtriangles3(string filename){
  Colour c;
  c.red = 255;
  c.green = 255;
  c.blue = 255;
  ifstream myfile;
  myfile.open(filename);
  string line;
  vector<ModelTriangle> triangles;
  vector<vec3> readvertices = readVertices3(SCALETING3, filename);
  while(getline(myfile, line)) {
    string *splitLine = split(line, ' ');
    if(splitLine[0] == "f"){
      int slashpos1 =  splitLine[1].find('/');
      int slashpos2 =  splitLine[2].find('/');
      int slashpos3 =  splitLine[3].find('/');
      string point1 = splitLine[1].substr(0,slashpos1);
      string point2 = splitLine[2].substr(0,slashpos2);
      string point3 = splitLine[3].substr(0,slashpos3);
      int p1 = stoi(point1) - 1;
      int p2 = stoi(point2) - 1;
      int p3 = stoi(point3) - 1;
      ModelTriangle toPush = ModelTriangle(readvertices[p1], readvertices[p2], readvertices[p3], c);
      toPush.name = "hackspace";
      triangles.push_back(toPush);
    }
  }
  myfile.close();
  return triangles;
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
void swap(int* a , int*b)
{
    int temp = *a;
    *a = *b;
    *b = temp;
}

float absolute(float x )
{
    if (x < 0) return -x;
    else return x;
}

//returns integer part of a floating point number
int iPartOfNumber(float x)
{
    return (int)x;
}

//rounds off a number
int roundNumber(float x)
{
    return iPartOfNumber(x + 0.5) ;
}

//returns fractional part of a number
float fPartOfNumber(float x)
{
    if (x>0) return x - iPartOfNumber(x);
    else return x - (iPartOfNumber(x)+1);

}

//returns 1 - fractional part of number
float rfPartOfNumber(float x)
{
    return 1 - fPartOfNumber(x);
}

void drawPixel( int x , int y ,uint32_t colour, float brightness)
{
    int r = (colour >> 16) & 255;
    int g = (colour >> 8) & 255;
    int b = (colour) & 255;
    uint32_t newcolour = (255<<24) + (int(r * brightness)<<16) + (int(g * brightness)<<8) + int(b * brightness);
    if(x<WIDTH && y<HEIGHT && x>=0 && y>=0){
      window.setPixelColour(x,y,newcolour);
    }
}

void drawWuLines(CanvasPoint from, CanvasPoint to, uint32_t colour) {
  float fromX = from.x;
  float fromY = from.y;
  float toX = to.x;
  float toY = to.y;

  int steep = absolute(toY - fromY) > absolute(toX - fromX);

   if (steep)
   {
       swap(fromX, fromY);
       swap(toX, toY);
   }
   if (fromX > toX)
   {
       swap( fromX , toX);
       swap( fromY , toY);
   }
   float dx =  toX - fromX;
   float dy =  toY - fromY;
   float gradient = dy/dx;
   if (dx == 0.0)
       gradient = 1;

   int xend1 = round(fromX);
   int xend2 = round(toX);
   float yend1 = fromY + gradient*(xend1  - fromX);
   float yend2 = toY + gradient*(xend2  - toX);

   int xpxl1 = xend1;
   int xpxl2 = xend2;

   int ypxl1 = iPartOfNumber(yend1);
   int ypxl2 = iPartOfNumber(yend2);

   float intersectY = yend1 + gradient;

    if(steep){
       drawPixel(ypxl1, xpxl1, colour, rfPartOfNumber(yend1) * fPartOfNumber(fromX+0.5));
       drawPixel(ypxl1+1, xpxl1, colour,fPartOfNumber(yend1) * fPartOfNumber(fromX+0.5));
       drawPixel(ypxl2, xpxl2, colour, rfPartOfNumber(yend2) * fPartOfNumber(toX+0.5));
       drawPixel(ypxl2+1, xpxl2, colour,fPartOfNumber(yend2) * fPartOfNumber(toX+0.5));
   }
  else{
    drawPixel(xpxl1,ypxl1,colour, rfPartOfNumber(yend1) * fPartOfNumber(fromX+0.5));
    drawPixel(xpxl1,ypxl1+1,colour,fPartOfNumber(yend1) * fPartOfNumber(fromX+0.5));
    drawPixel( xpxl2,ypxl2, colour, rfPartOfNumber(yend2) * fPartOfNumber(toX+0.5));
    drawPixel(xpxl2,ypxl2+1,  colour,fPartOfNumber(yend2) * fPartOfNumber(toX+0.5));
  }
   if (steep)
   {
       int x;
       for (x = xpxl1+1 ; x <xpxl2-1 ; x++)
       {
           drawPixel(iPartOfNumber(intersectY), x, colour, rfPartOfNumber(intersectY));
           drawPixel(iPartOfNumber(intersectY)+1, x,colour, fPartOfNumber(intersectY));
           intersectY += gradient;
       }
   }
   else
   {
       int x;
       for (x = xpxl1+1 ; x <xpxl2-1 ; x++)
       {
           drawPixel(x, iPartOfNumber(intersectY),colour,
                       rfPartOfNumber(intersectY));
           drawPixel(x, iPartOfNumber(intersectY)+1,colour,
                         fPartOfNumber(intersectY));
           intersectY += gradient;
       }
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


void draw3DRasterisedTriangles(vector<CanvasTriangle> canvastriangles, vector<uint32_t> colours) {

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

    point4.texturePoint.x = point3.texturePoint.x - ratio * (point3.texturePoint.x - point1.texturePoint.x);
    point4.texturePoint.y = point3.texturePoint.y - ratio * (point3.texturePoint.y - point1.texturePoint.y);

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
            if(current.texturePoint.x == -1 && current.texturePoint.y == -1 ){
              window.setPixelColour((int)current.x, (int)current.y, colour);
            }
            else{
              window.setPixelColour((int)current.x, (int)current.y, colours[round(current.texturePoint.x) + round(current.texturePoint.y) * 300 ]);
            }
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
            if(current.texturePoint.x == -1 && current.texturePoint.y == -1 ){
              window.setPixelColour((int)current.x, (int)current.y, colour);
            }
            else{
              window.setPixelColour((int)current.x, (int)current.y, colours[round(current.texturePoint.x) + round(current.texturePoint.y) * 300 ]);
            }
          }
        }
      }
    }

  }
}
void drawTextureTriangles(vector<CanvasTriangle> canvastriangles) {
   vector<uint32_t> colours = loadImg("texture1.ppm");
   for(u_int i = 0; i < canvastriangles.size(); i++){

     CanvasPoint point1 = canvastriangles[i].vertices[0];
     CanvasPoint point2 = canvastriangles[i].vertices[1];
     CanvasPoint point3 = canvastriangles[i].vertices[2];

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

     point4.texturePoint.x = point3.texturePoint.x - ratio * (point3.texturePoint.x - point1.texturePoint.x);
     point4.texturePoint.y = point3.texturePoint.y - ratio * (point3.texturePoint.y - point1.texturePoint.y);

     vector<CanvasPoint> points1to4coord = interpolation2(point1,point4,(point2.y-point1.y)+1);
     vector<CanvasPoint> points1to2coord = interpolation2(point1,point2,(point2.y-point1.y)+1);
     vector<CanvasPoint> points4to3coord = interpolation2(point4,point3,(point3.y-point2.y)+1);
     vector<CanvasPoint> points2to3coord = interpolation2(point2,point3,(point3.y-point2.y)+1);

     for (u_int i = 0; i < points1to4coord.size(); i++) {
       vector<CanvasPoint> pointstofillcoord = interpolation2(points1to4coord[i],points1to2coord[i],abs(points1to4coord[i].x-points1to2coord[i].x)+1);
       for(u_int k = 0; k < pointstofillcoord.size(); k++) {
         CanvasPoint current = pointstofillcoord[k];
         if(((int)current.x >= 0) && ((int)current.x < WIDTH) && ((int)current.y >= 0) && ((int)current.y < HEIGHT)) {
            window.setPixelColour((int)current.x, (int)current.y, colours[round(current.texturePoint.x) + round(current.texturePoint.y) * 300 ]);
       }
     }
   }

     for (u_int i = 0; i < points4to3coord.size(); i++) {
       vector<CanvasPoint> pointstofillcoord = interpolation2(points4to3coord[i],points2to3coord[i],abs(points4to3coord[i].x-points2to3coord[i].x)+1);
       for(u_int k = 0; k < pointstofillcoord.size(); k++) {
         CanvasPoint current = pointstofillcoord[k];
         if((int)(current.x >= 0) && ((int)current.x < WIDTH) && ((int)current.y >= 0) && ((int)current.y < HEIGHT)) {
             window.setPixelColour((int)current.x, (int)current.y, colours[round(current.texturePoint.x) + round(current.texturePoint.y) * 300 ]);
         }
       }
     }
   }
}

vector<CanvasTriangle> modelToCanvas(vec3 camera, vector<ModelTriangle> triangles, vector<ModelTexture> textureTriangles, float focalDistance, float canvasScale){

  vector<CanvasTriangle> canvastriangles;

  for(u_int i=0;i<triangles.size();i++){

    CanvasPoint test1,test2,test3;
    CanvasPoint projP1,projP2,projP3;
    projP1.brightness = 0;
    projP2.brightness = 0;
    projP3.brightness = 0;
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


    TexturePoint p1 = textureTriangles[i].vertices[0];
    TexturePoint p2 = textureTriangles[i].vertices[1];
    TexturePoint p3 = textureTriangles[i].vertices[2];

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
    projP1.texturePoint = p1;


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
    projP2.texturePoint = p2;


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
    projP3.texturePoint = p3;


    canvastriangles.push_back(CanvasTriangle(projP1,projP2,projP3,triangles[i].colour));
  }

  return canvastriangles;
}

vec3 rayDir(int x, int y) {
  vec3 dir = normalize(vec3(x-WIDTH/2, -(y-HEIGHT/2),-500)-camera)* glm::inverse(camOrien);
  return dir;
}

bool inShadow(vec3 surfacePoint, vec3 lightSource, vector<ModelTriangle> triangles, int index){
  bool shadow = false;
  vec3 dir = (lightSource - surfacePoint);
  float distance = length(dir);
  for(u_int i=0;i<triangles.size();i++){
    vec3 e0 = triangles[i].vertices[1] - triangles[i].vertices[0];
    vec3 e1 = triangles[i].vertices[2] - triangles[i].vertices[0];
    vec3 SPVector = surfacePoint - (triangles[i].vertices[0]);
    mat3 DEMatrix(-normalize(dir), e0, e1);
    vec3 possibleSolution = inverse(DEMatrix) * SPVector;
    float t = possibleSolution.x;
    float u = possibleSolution.y;
    float v = possibleSolution.z;
    if ((u>= 0) & (u<=1) & (v>= 0) & (v<=1) & ((u+v)<=1) & ((int)i != index)){
      if (t < distance && t > 0.0f){
        shadow = true;
        break;
      }
    }
  }
  return shadow;
}

vec3 normaltoVertices(ModelTriangle trianglex){
  vec3 normaltOvertices = normalize(cross(trianglex.vertices[1]-trianglex.vertices[0],trianglex.vertices[2]-trianglex.vertices[0]));
  return normaltOvertices;
}

int getIndex(ModelTriangle triangle,vector<ModelTriangle> triangles){
  int index = 0;
  for(u_int i=0;i<triangles.size();i++){
    if((triangles[i].vertices[0] == triangle.vertices[0]) & (triangles[i].vertices[1] == triangle.vertices[1]) & (triangles[i].vertices[2] == triangle.vertices[2])){
      index = i;
      break;
    }
  }
  return index;
}


float compBrightness( vec3 vectorX, vec3 normaltovertices){
  vec3 pToL = whitelight - vectorX;
  float dotProd = (dot(normaltovertices,normalize(pToL)));
  if(dotProd < 0.0f) dotProd = 0.0f;
  float myDistance = length(pToL);
  float brightness = 5 * (dotProd)/(0.5*M_PI* myDistance * myDistance);
  if(brightness > 1.0f) brightness = 1.0f;
  if(brightness < 0.2f) brightness = 0.2f;
  return brightness;
}

pair<ModelTriangle,vector<vec3>> compare(ModelTriangle triangle){
  pair<ModelTriangle,vector<vec3>> rightOne;
  for(u_int i=0;i<triangleNormals.size();i++){
    if((triangleNormals[i].first.vertices[0] == triangle.vertices[0]) & (triangleNormals[i].first.vertices[1] == triangle.vertices[1]) & (triangleNormals[i].first.vertices[2] == triangle.vertices[2])){
      rightOne = triangleNormals[i];
      break;
    }
  }
  return rightOne;
}
pair<ModelTriangle,ModelTexture> compareTP(ModelTriangle triangle){
  pair<ModelTriangle,ModelTexture> rightOne;
  for(u_int i=0;i<texturePoints.size();i++){
    if((texturePoints[i].first.vertices[0] == triangle.vertices[0]) & (texturePoints[i].first.vertices[1] == triangle.vertices[1]) & (texturePoints[i].first.vertices[2] == triangle.vertices[2])){
      rightOne = texturePoints[i];
      break;
    }
  }
  return rightOne;
}

vector<ModelTriangle> removeTriangle(ModelTriangle triangle,vector<ModelTriangle> triangles){
  vector<ModelTriangle> newSet;
  for(u_int i=0;i<triangles.size();i++){
    if(!((triangles[i].vertices[0] == triangle.vertices[0]) & (triangles[i].vertices[1] == triangle.vertices[1]) & (triangles[i].vertices[2] == triangle.vertices[2]))){
      newSet.push_back(triangles[i]);
    }
  }
  return newSet;
}

vec3 returnTUandV(vec3 cameraPosition,ModelTriangle triX,vec3 rayDirection){
  vec3 e0 = triX.vertices[1] - triX.vertices[0];
  vec3 e1 = triX.vertices[2] - triX.vertices[0];
  vec3 SPVector = cameraPosition - (triX.vertices[0]);
  mat3 DEMatrix(-rayDirection, e0, e1);
  vec3 possibleSolution = inverse(DEMatrix) * SPVector;
  return possibleSolution;
}
RayTriangleIntersection getClosestIntersection(vec3 cameraPosition, vec3 rayDirection, vector<ModelTriangle> triangles,vector<ModelTexture> textureTriangles,vector<uint32_t> colours) {
  RayTriangleIntersection intersectionP;
  intersectionP.distanceFromCamera = INFINITY;
  for(u_int i = 0; i < triangles.size(); i++){
    ModelTriangle triX = triangles[i];
    vec3 possibleSolution = returnTUandV(cameraPosition,triX,rayDirection);
    float t = possibleSolution.x;
    float u = possibleSolution.y;
    float v = possibleSolution.z;
    vec3 e0 = triX.vertices[1] - triX.vertices[0];
    vec3 e1 = triX.vertices[2] - triX.vertices[0];
    if ((u>= 0) & (u<=1) & (v>= 0) & (v<=1) & ((u+v)<=1)){
      if (t < intersectionP.distanceFromCamera){
        if((intersectionP.distanceFromCamera - t) > 0.05){
        intersectionP.distanceFromCamera = t;
        intersectionP.intersectedTriangle = triangles[i];
        intersectionP.intersectionPoint = triangles[i].vertices[0] + (u*e0) + (v*e1);
        }
      }
    }
  }
  if(intersectionP.distanceFromCamera == INFINITY)
  {
    intersectionP.distanceFromCamera = -INFINITY;
  }
  vec3 solution = returnTUandV(cameraPosition,intersectionP.intersectedTriangle,rayDirection);
  float u = solution.y;
  float v = solution.z;
  vec3 normaltovertices = normalize(cross(intersectionP.intersectedTriangle.vertices[1]-intersectionP.intersectedTriangle.vertices[0],intersectionP.intersectedTriangle.vertices[2]-intersectionP.intersectedTriangle.vertices[0]));

  if(intersectionP.distanceFromCamera != -INFINITY){
    if(intersectionP.intersectedTriangle.name == "cornell"){
      int ind = getIndex(intersectionP.intersectedTriangle,triangles);
      float brightness = compBrightness(intersectionP.intersectionPoint, normaltovertices);
      if(brightness < 0.2f) brightness = 0.2f;
      float count = 0;
      for(u_int i = 0;i<whitelights.size();i++){
        if(inShadow(intersectionP.intersectionPoint, whitelights[i],triangles,ind)){
          count++;
        }
      }
      float ratio = count/13;
      brightness *= 1 - ratio;
      if(brightness<0.15f){
        brightness = 0.15f;
      }
      if(brightness > 1.0f) brightness = 1.0f;
      Colour c = intersectionP.intersectedTriangle.colour;
      c.red = c.red * brightness;
      c.green = c.green * brightness;
      c.blue = c.blue * brightness;
      intersectionP.intersectedTriangle.colour = c;
    }
    else if(intersectionP.intersectedTriangle.name == "sphere"){
      int ind = getIndex(intersectionP.intersectedTriangle,triangles);
      pair<ModelTriangle,vector<vec3>> rightOne = compare(intersectionP.intersectedTriangle);
      vector<vec3> normals = rightOne.second;
      // GOUROUD
      // float brightness1 = compBrightness(trianglex.vertices[0],normals[0]);
      // float brightness2 = compBrightness(trianglex.vertices[1],normals[1]);
      // float brightness3 = compBrightness(trianglex.vertices[2],normals[2]);
      // float brightness = brightness1 + (u*(brightness2-brightness1)) + (v*(brightness3-brightness1));
      //PHONG
      vec3 newNormal = normals[0] + (u*(normals[1]-normals[0])) + (v*(normals[2]-normals[0]));
      float brightness = compBrightness(intersectionP.intersectionPoint,newNormal);
      if(brightness < 0.2f) brightness = 0.2f;
      float count = 0;
      for(u_int i = 0;i<whitelights.size();i++){
        if(inShadow(intersectionP.intersectionPoint, whitelights[i],triangles,ind)){
          count++;
        }
      }
      float ratio = count/13;
      brightness *= 1 - ratio;
      if(brightness<0.15f){
        brightness = 0.15f;
      }
      if(brightness > 1.0f) brightness = 1.0f;
      Colour c = intersectionP.intersectedTriangle.colour;
      c.red = c.red * brightness;
      c.green = c.green * brightness;
      c.blue = c.blue * brightness;
      intersectionP.intersectedTriangle.colour = c;
    }
    else if(intersectionP.intersectedTriangle.name == "hackspace"){
      int ind = getIndex(intersectionP.intersectedTriangle,triangles);
      pair<ModelTriangle,ModelTexture> rightTextureP = compareTP(intersectionP.intersectedTriangle);
      ModelTexture textureTriangle = rightTextureP.second;
      TexturePoint p1 = textureTriangle.vertices[0];
      TexturePoint p2 = textureTriangle.vertices[1];
      TexturePoint p3 = textureTriangle.vertices[2];

      vec2 tp1 = vec2(p1.x,p1.y) * 300.0f;
      vec2 tp2 = vec2(p2.x,p2.y) * 300.0f;
      vec2 tp3 = vec2(p3.x,p3.y) * 300.0f;
      vec2 texPoint = tp1 + (u*(tp2-tp1)) + (v*(tp3-tp1));
      uint32_t tempC = colours[round(texPoint.x) + round(texPoint.y) * 300 ];
      int red = (tempC >> 16) & 255;
      int green = (tempC >> 8) & 255;
      int blue = (tempC) & 255;
      float brightness = compBrightness(intersectionP.intersectionPoint, normaltovertices);
      if(brightness < 0.2f) brightness = 0.2f;
      float count = 0;
      for(u_int i = 0;i<whitelights.size();i++){
        if(inShadow(intersectionP.intersectionPoint, whitelights[i],triangles,ind)){
          count++;
        }
      }
      float ratio = count/13;
      brightness *= 1 - ratio;
      if(brightness<0.15f){
        brightness = 0.15f;
      }
      if(brightness > 1.0f) brightness = 1.0f;
      Colour c = intersectionP.intersectedTriangle.colour;
      c.red = red * brightness;
      c.green = green * brightness;
      c.blue = blue * brightness;
      intersectionP.intersectedTriangle.colour = c;
    }
     if(intersectionP.intersectedTriangle.name == "mirror"){
      vec3 reflectionNormal = normaltoVertices(intersectionP.intersectedTriangle);
      vec3 reflectionRay = rayDirection - (2.0f * reflectionNormal * (dot(rayDirection,reflectionNormal)));
      vector<ModelTriangle> newSet = removeTriangle(intersectionP.intersectedTriangle,triangles);
      RayTriangleIntersection intersectR = getClosestIntersection(intersectionP.intersectionPoint,reflectionRay,newSet,textureTriangles,colours);
      if(intersectR.distanceFromCamera == -INFINITY){
        Colour c = intersectionP.intersectedTriangle.colour;
        c.red = 0;
        c.green = 0;
        c.blue = 0;
        intersectionP.intersectedTriangle.colour = c;
      }
      else{
        Colour reflectedColour = intersectR.intersectedTriangle.colour;
        intersectionP.intersectedTriangle.colour = reflectedColour;
      }
    }
  }

  return intersectionP;
}


void antialiasing(vector<ModelTriangle> triangles,vector<ModelTexture> textureTriangles,vector<uint32_t> colours){
  for(int x=0;x<WIDTH;x++){
    for(int y=0;y<HEIGHT;y++){
      vec3 dir = rayDir(x,y);
      RayTriangleIntersection intersectP = getClosestIntersection(camera, dir, triangles,textureTriangles,colours);
      vec3 dir1 = rayDir(x + 0.5 ,y + 0.5);
      RayTriangleIntersection intersectP1 = getClosestIntersection(camera, dir1, triangles,textureTriangles,colours);
      vec3 dir2 = rayDir(x + 0.5 ,y - 0.5);
      RayTriangleIntersection intersectP2 = getClosestIntersection(camera, dir2, triangles,textureTriangles,colours);
      vec3 dir3 = rayDir(x - 0.5 ,y + 0.5);
      RayTriangleIntersection intersectP3 = getClosestIntersection(camera, dir3, triangles,textureTriangles,colours);
      vec3 dir4 = rayDir(x - 0.5 ,y - 0.5);
      RayTriangleIntersection intersectP4 = getClosestIntersection(camera, dir4, triangles,textureTriangles,colours);

      ModelTriangle trianglex = intersectP.intersectedTriangle;
      ModelTriangle triangle1 = intersectP1.intersectedTriangle;
      ModelTriangle triangle2 = intersectP2.intersectedTriangle;
      ModelTriangle triangle3 = intersectP3.intersectedTriangle;
      ModelTriangle triangle4 = intersectP4.intersectedTriangle;

      Colour c = intersectP.intersectedTriangle.colour;
      Colour c1 = intersectP1.intersectedTriangle.colour;
      Colour c2 = intersectP2.intersectedTriangle.colour;
      Colour c3 = intersectP3.intersectedTriangle.colour;
      Colour c4 = intersectP4.intersectedTriangle.colour;

      float avgred = (c.red + c1.red + c2.red + c3.red + c4.red)/5;
      float avggreen = (c.green + c1.green + c2.green + c3.green + c4.green)/5;
      float avgblue = (c.blue + c1.blue + c2.blue + c3.blue + c4.blue)/5;


      vec3 normaltoVerticesx = normaltoVertices(trianglex);
      vec3 normaltoVertices1 = normaltoVertices(triangle1);
      vec3 normaltoVertices2 = normaltoVertices(triangle2);
      vec3 normaltoVertices3 = normaltoVertices(triangle3);
      vec3 normaltoVertices4 = normaltoVertices(triangle4);

      float brightnessx = compBrightness(intersectP.intersectionPoint, normaltoVerticesx);
      float brightness1 = compBrightness(intersectP1.intersectionPoint, normaltoVertices1);
      float brightness2 = compBrightness(intersectP2.intersectionPoint, normaltoVertices2);
      float brightness3 = compBrightness(intersectP3.intersectionPoint, normaltoVertices3);
      float brightness4 = compBrightness(intersectP4.intersectionPoint, normaltoVertices4);


      float avgB = (brightnessx + brightness1 + brightness2 + brightness3 + brightness4)/5;

      uint32_t avg = (255<<24) + (int(avgred * avgB)<<16) + (int(avggreen * avgB)<<8) + int(avgblue * avgB);

      if(intersectP.distanceFromCamera != -INFINITY){
        window.setPixelColour(x, y, avg);
      }
    }
  }
}
vector<ModelTriangle> culledTriangles(vector<ModelTriangle> triangles){
  vector<ModelTriangle> culledTriangles;
  for (u_int i = 0; i < triangles.size(); i++) {
    vec3 test1 = triangles[i].vertices[0];
    vec3 test2 = triangles[i].vertices[1];
    vec3 test3 = triangles[i].vertices[2];
    vec3 normaltovertices = normalize(cross(test2-test1,test3-test1));
    vec3 centroid = vec3((test1.x + test2.x + test3.x)/3,(test1.y + test2.y + test3.y)/3,(test1.z + test2.z + test3.z)/3);
    vec3 camtoCentroid = normalize(camera - centroid);
    float dotProd = (dot(normaltovertices,camtoCentroid));
      if(dotProd >= 0){
        culledTriangles.push_back(triangles[i]);
      }
    }
    return culledTriangles;
}

void culling(vector<CanvasTriangle> triangles) {

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
            window.setPixelColour((int)points1to2[i].x,(int)points1to2[i].y, colour);
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

void computeRayT(vector<ModelTriangle> triangles,vector<ModelTexture> textureTriangles,vector<vec3> whitelights,vector<uint32_t> colours, vec3 camera){
  for(int x=0;x<WIDTH;x++){
    for(int y=0;y<HEIGHT;y++){
      vec3 dir = rayDir(x,y);
      RayTriangleIntersection intersectP = getClosestIntersection(camera, dir, triangles,textureTriangles,colours);
      ModelTriangle trianglex = intersectP.intersectedTriangle;
      Colour c = trianglex.colour;
      uint32_t colour = (255<<24) + (int(c.red )<<16) + (int(c.green)<<8) + int(c.blue);
        if(intersectP.distanceFromCamera != -INFINITY){
          window.setPixelColour(x, y, colour);
        }
    }
  }
}


void initializePair(vector<ModelTriangle> triangles){
  for(u_int i = 0;i<triangles.size();i++){
    vector<vec3> normals;
    for(u_int j = 0;j<3;j++){
      vec3 test1 = triangles[i].vertices[j];
      vec3 avgNormal = vec3(0,0,0);
      int count = 0;
      for(u_int k = 0;k<triangles.size();k++){
        for(u_int l = 0;l<3;l++){
          vec3 test2 = triangles[k].vertices[l];
          if(test1 == test2){
            vec3 normaltoVertices2 = normaltoVertices(triangles[k]);
            avgNormal += normaltoVertices2;
            count++;
          }
        }
      }
      avgNormal = avgNormal / ((float) count);
      normals.push_back(avgNormal);
    }
    triangleNormals.push_back(std::make_pair(triangles[i], normals));
  }
}
void initializeTexturePoints(vector<ModelTriangle> triangles,vector<ModelTexture> Texturetriangles){
  for(u_int i = 0;i<triangles.size();i++){
    texturePoints.push_back(std::make_pair(triangles[i], Texturetriangles[i]));
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
  vector<ModelTriangle> triangles2 = readtriangles2("sphere.obj");
  vector<ModelTriangle> triangles3 = readtriangles3("logo.obj");
  vector<ModelTexture> textureTriangles = readTextureIndex("logo.obj");
  vector<ModelTexture> textureTriangles2 = readTextureIndex("sphere.obj");
  vector<ModelTexture> textureTriangles3 = readTextureIndex("cornell-box.obj");
  vector<ModelTexture> textureTrianglesRay = readTextureIndexForRayTracer("logo.obj");
  vector<uint32_t> coloursTextures = loadImg("texture1.ppm");


  vector<ModelTexture> completeTextures = textureTriangles3;
  for(u_int i = 0;i<textureTriangles2.size();i++){
    completeTextures.push_back(textureTriangles2[i]);
  }
  for(u_int i = 0;i<textureTriangles.size();i++){
    completeTextures.push_back(textureTriangles[i]);
  }
  initializeTexturePoints(triangles3,textureTrianglesRay);
  initializePair(triangles2);

  vector<ModelTriangle> complete = triangles;
  for(u_int i = 0;i<triangles2.size();i++){
    complete.push_back(triangles2[i]);
  }
  for(u_int i = 0;i<triangles3.size();i++){
    complete.push_back(triangles3[i]);
  }
  SDL_Event event;
  while(true)
  {
    if(window.pollForInputEvents(&event)){
      handleEvent(event);
      window.clearPixels();
      if (mode == 1) {
        vector<CanvasTriangle> canvastriangles = modelToCanvas(camera, triangles,textureTriangles3, 5, 100);
        vector<CanvasTriangle> canvastriangles2 = modelToCanvas(camera, triangles2,textureTriangles2, 5, 100);
        vector<CanvasTriangle> canvastriangles3 = modelToCanvas(camera, triangles3,textureTriangles, 5, 100);
        vector<CanvasTriangle> complete = canvastriangles;
        for(u_int i = 0;i<canvastriangles2.size();i++){
          complete.push_back(canvastriangles2[i]);
        }
        for(u_int i = 0;i<canvastriangles3.size();i++){
          complete.push_back(canvastriangles3[i]);
        }
        draw3DWireframes(complete);
      }

      else if (mode == 2) {
        vector<CanvasTriangle> canvastriangles = modelToCanvas(camera, triangles,textureTriangles3, 5, 100);
        vector<CanvasTriangle> canvastriangles2 = modelToCanvas(camera, triangles2,textureTriangles2,  5, 100);
        vector<CanvasTriangle> canvastriangles3 = modelToCanvas(camera, triangles3,textureTriangles, 5, 100);

        vector<CanvasTriangle> complete = canvastriangles;
        for(u_int i = 0;i<canvastriangles2.size();i++){
          complete.push_back(canvastriangles2[i]);
        }
        for(u_int i = 0;i<canvastriangles3.size();i++){
          complete.push_back(canvastriangles3[i]);
        }
        vector<uint32_t> colours = loadImg("texture1.ppm");

        draw3DRasterisedTriangles(complete,colours);
      }

      else if (mode == 3) {
        computeRayT(complete,textureTrianglesRay,whitelights,coloursTextures,camera);
      }
      else if (mode == 4) {
        antialiasing(triangles,textureTrianglesRay,coloursTextures);
      }
      else if (mode == 5){
        vector<ModelTriangle> complete2 = culledTriangles(complete);
        vector<CanvasTriangle> culledTrianglesx = modelToCanvas(camera, complete2,completeTextures, 5, 100);
        culling(culledTrianglesx);
      }
      else if (mode == 6){
        vector<CanvasTriangle> canvastriangles = modelToCanvas(camera, triangles,textureTriangles3, 5, 100);
        vector<CanvasTriangle> canvastriangles2 = modelToCanvas(camera, triangles2,textureTriangles2,  5, 100);
        vector<CanvasTriangle> canvastriangles3 = modelToCanvas(camera, triangles3,textureTriangles, 5, 100);
        vector<CanvasTriangle> complete = canvastriangles;
        for(u_int i = 0;i<canvastriangles2.size();i++){
          complete.push_back(canvastriangles2[i]);
        }
        for(u_int i = 0;i<canvastriangles3.size();i++){
          complete.push_back(canvastriangles3[i]);
        }
        for(u_int i = 0;i<complete.size();i++){
          CanvasPoint point1 = complete[i].vertices[0];
          CanvasPoint point2 = complete[i].vertices[1];
          CanvasPoint point3 = complete[i].vertices[2];
          Colour c = complete[i].colour;
          float red = c.red;
          float green = c.green;
          float blue = c.blue;
          uint32_t colour = (255<<24) + (int(red)<<16) + (int(green)<<8) + int(blue);
          drawWuLines(point1, point2, colour);
          drawWuLines(point2, point3, colour);
          drawWuLines(point1, point3, colour);
        }
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

    else if(event.key.keysym.sym == SDLK_g){
      whitelight.x -= 0.1;
      cout << "LightLeft" << endl;

    }
    else if(event.key.keysym.sym == SDLK_j){
      whitelight.x += 0.1;
      cout << "LightRight" << endl;

    }
    else if(event.key.keysym.sym == SDLK_u) {
      whitelight.y += 0.1;
      cout << "LightUp" << endl;

    }
    else if(event.key.keysym.sym == SDLK_h){
      whitelight.y -= 0.1;
      cout << "LightDown" << endl;

    }
    else if(event.key.keysym.sym == SDLK_n){
      whitelight.z -= 0.1;
      cout << "LightForward" << endl;

    }
    else if(event.key.keysym.sym == SDLK_b){
      whitelight.z += 0.1;
      cout << "LightBackward" << endl;

    }
    else if(event.key.keysym.sym == SDLK_p){
      cout << whitelight.x << endl;
      cout << whitelight.y << endl;
      cout << whitelight.z << endl;
    }
    else if(event.key.keysym.sym == SDLK_v){
      cout << "SaveImage" << endl;
      savePPM();
    }

    else if(event.key.keysym.sym == SDLK_1) {
      mode = 1;
      cout << "WireFrame Mode" << endl;
    }
    else if(event.key.keysym.sym == SDLK_2) {
      mode = 2;
      cout << "Rasterizer Mode" << endl;
    }
    else if(event.key.keysym.sym == SDLK_3) {
      mode = 3;
      cout << "Raytracer Mode" << endl;
    }
    else if(event.key.keysym.sym == SDLK_4) {
      mode = 4;
      cout << "Antialiasing Mode" << endl;
    }
    else if(event.key.keysym.sym == SDLK_5) {
      mode = 5;
      cout << "Culling Mode" << endl;
    }
    else if(event.key.keysym.sym == SDLK_6) {
      mode = 6;
      cout << "Wu Lines Mode" << endl;
    }
    else if(event.key.keysym.sym == SDLK_c) {
      window.clearPixels();
    }
    else if(event.key.keysym.sym == SDLK_x) {
      rotateX(angle);
      cout << "ROTATE-X" << endl;
    }
    else if(event.key.keysym.sym == SDLK_y) {
      rotateY(-angle);
      cout << "ROTATE-Y" << endl;
    }
    else if(event.key.keysym.sym == SDLK_z) {
      rotateZ(angle);
      cout << "ROTATE-Z" << endl;
    }
    else if(event.key.keysym.sym == SDLK_l) {
      lookAt(camera, whitelight);
    }
    else if(event.key.keysym.sym == SDLK_r) {
      camera = vec3(0,0,6);
      camOrien = mat3();
      cout << "RESET" << endl;
    }
  }
  else if(event.type == SDL_MOUSEBUTTONDOWN) cout << "MOUSE CLICKED" << endl;
}
